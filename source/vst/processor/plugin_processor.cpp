#include "plugin_processor.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "util/file_logger.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/pluginids.h"
#include "vst/common/processor_state.h"
#include "version.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "base/source/fstreamer.h"
#include "base/source/fstring.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

PluginProcessor::PluginProcessor()
    : registry_(createParameterDefinitions()) {
    setControllerClass(kControllerUID);
}

PluginProcessor::~PluginProcessor() {
    // Stop worker first so no pending callbacks can access members after they
    // are destroyed. Destroying the client (instead of an explicit disconnect)
    // sets `destroying_` first, so the auto-reconnect/onClose path never fires.
    worker_.stop();
    kiwiClient_.reset();
}

// ---------------------------------------------------------------------------
// IPluginBase
// ---------------------------------------------------------------------------

tresult PLUGIN_API PluginProcessor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 1);

    // Initialise pipeline components. The real DAW rate arrives in
    // setupProcessing(); 48000 Hz is a safe placeholder until then.
    constexpr double kDefaultDawRate    = 48000.0;
    constexpr double kDefaultServerRate = 12000.0;
    resampler_    = std::make_unique<Resampler>(kDefaultServerRate, kDefaultDawRate);
    smoothedRatio_ = resampler_->currentRatio();
    // 500 ms target prefill, 2000 ms max capacity (covers network jitter).
    jitterBuffer_  = std::make_unique<JitterBuffer>(kDefaultDawRate, 500.0, 2000.0);
    adpcmDecoder_  = std::make_unique<ImaAdpcmDecoder>();

    worker_.start();
    NETSDR_LOG_INFO("PluginProcessor initialized (M3)");
    return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate() {
    worker_.stop();
    // Destroy the client directly (KiwiClient's destructor sets `destroying_`
    // first, so the onClose/reconnect path does not fire a spurious disconnect).
    kiwiClient_.reset();
    return AudioEffect::terminate();
}

// ---------------------------------------------------------------------------
// State persistence
// ---------------------------------------------------------------------------

tresult PLUGIN_API PluginProcessor::setState(IBStream* state) {
    if (state == nullptr) {
        return kResultFalse;
    }

    // Read up to 4096 bytes; ProcessorState::deserialize validates internally.
    constexpr int kMaxStateBytes = 4096;
    std::string bytes(kMaxStateBytes, '\0');
    IBStreamer streamer(state, kLittleEndian);
    const TSize bytesRead = streamer.readRaw(bytes.data(),
                                              static_cast<TSize>(kMaxStateBytes));
    if (bytesRead < 8) { // minimum: version(4) + station-len(4)
        return kResultFalse;
    }
    bytes.resize(static_cast<std::size_t>(bytesRead));

    ProcessorState s;
    if (!s.deserialize(bytes)) {
        return kResultFalse;
    }
    applyState(s);
    return kResultOk;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream* state) {
    if (state == nullptr) {
        return kResultFalse;
    }

    ProcessorState s;
    s.station    = station();
    s.freqKhz    = freqKhz_.load();
    s.mode       = static_cast<double>(mode_.load());
    s.lowCut     = static_cast<double>(lowCut_.load());
    s.highCut    = static_cast<double>(highCut_.load());
    s.agcOn      = agcOn_.load()  ? 1.0 : 0.0;
    s.agcHang    = static_cast<double>(agcHang_.load());
    s.agcThresh  = static_cast<double>(agcThresh_.load());
    s.agcSlope   = static_cast<double>(agcSlope_.load());
    s.agcDecay   = static_cast<double>(agcDecay_.load());
    s.agcManGain = static_cast<double>(agcManGain_.load());
    s.volume     = volume_.load();
    s.mute       = mute_.load() ? 1.0 : 0.0;
    s.squelchOn  = squelchOn_.load()  ? 1.0 : 0.0;
    s.squelchThr = squelchThr_.load();
    s.nbOn       = nbOn_.load()   ? 1.0 : 0.0;
    s.nbThresh   = nbThresh_.load();
    s.nrOn       = nrOn_.load()   ? 1.0 : 0.0;
    s.deempOn    = deempOn_.load() ? 1.0 : 0.0;
    s.compOn     = compOn_.load()  ? 1.0 : 0.0;

    const std::string bytes = s.serialize();
    IBStreamer streamer(state, kLittleEndian);
    if (streamer.writeRaw(bytes.data(), static_cast<TSize>(bytes.size())) !=
        static_cast<TSize>(bytes.size())) {
        return kResultFalse;
    }
    return kResultOk;
}

// ---------------------------------------------------------------------------
// IConnectionPoint — receives the setStation/disconnect messages
// ---------------------------------------------------------------------------

tresult PLUGIN_API PluginProcessor::notify(IMessage* message) {
    if (message == nullptr) {
        return kResultFalse;
    }
    const char* msgId = message->getMessageID();
    if (msgId == nullptr) {
        return kResultFalse;
    }

    if (std::strcmp(msgId, "NetSDRStation:SetStation") == 0) {
        // Read UTF-16 TChar buffer from message attributes.
        std::array<TChar, 256> buf{};
        if (message->getAttributes()->getString(
                "HostPort", buf.data(),
                static_cast<uint32>(buf.size() * sizeof(TChar))) != kResultOk) {
            return kResultFalse;
        }
        std::string stationStr;
        for (TChar c : buf) {
            if (c == 0) break;
            stationStr += static_cast<char>(c);
        }
        // connectToStation performs network I/O; run it on the worker thread
        // (notify() is invoked on the host's message/UI thread).
        worker_.post([this, stationStr]() { connectToStation(stationStr); });
        return kResultOk;
    }

    if (std::strcmp(msgId, "NetSDRStation:Disconnect") == 0) {
        worker_.post([this]() { disconnectStation(); });
        return kResultOk;
    }

    return AudioEffect::notify(message);
}

// ---------------------------------------------------------------------------
// IAudioProcessor (bus / sample-size / setup)
// ---------------------------------------------------------------------------

tresult PLUGIN_API PluginProcessor::setBusArrangements(SpeakerArrangement* inputs,
                                                       int32 numIns,
                                                       SpeakerArrangement* outputs,
                                                       int32 numOuts) {
    if (numOuts == 1 && SpeakerArr::getChannelCount(outputs[0]) > 0) {
        if (auto* bus = FCast<AudioBus>(audioOutputs.at(0))) {
            bus->setArrangement(outputs[0]);
            return kResultOk;
        }
    }
    return kResultFalse;
}

tresult PLUGIN_API PluginProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
        ? kResultTrue
        : kResultFalse;
}

tresult PLUGIN_API PluginProcessor::setupProcessing(ProcessSetup& newSetup) {
    const double dawRate    = newSetup.sampleRate;
    const double serverRate = serverSampleRate_.load();

    resampler_    = std::make_unique<Resampler>(serverRate, dawRate);
    smoothedRatio_ = resampler_->currentRatio();
    // Keep 500 ms / 2000 ms prefill/capacity at the new DAW rate.
    jitterBuffer_  = std::make_unique<JitterBuffer>(dawRate, 500.0, 2000.0);
    lastRatioAdjust_ = std::chrono::steady_clock::now();

    return AudioEffect::setupProcessing(newSetup);
}

// ---------------------------------------------------------------------------
// applyParamValue — host automation / UI messages
// ---------------------------------------------------------------------------

void PluginProcessor::applyParamValue(ParamID tag, ParamValue value) {
    registry_.setValue(tag, value);
    switch (tag) {
        case kParamFreqKhz:   freqKhz_.store(registry_.toPlain(tag, value)); break;
        case kParamMode:      mode_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamLowCut:    lowCut_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamHighCut:   highCut_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamAgcOn:     agcOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamAgcHang:   agcHang_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamAgcThresh: agcThresh_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamAgcSlope:  agcSlope_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamAgcDecay:  agcDecay_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamAgcManGain: agcManGain_.store(static_cast<int>(registry_.toPlain(tag, value))); break;
        case kParamVolume:    volume_.store(registry_.toPlain(tag, value)); break;
        case kParamMute:      mute_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamSquelchOn: squelchOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamSquelchThr: squelchThr_.store(registry_.toPlain(tag, value)); break;
        case kParamNbOn:      nbOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamNbThresh:  nbThresh_.store(registry_.toPlain(tag, value)); break;
        case kParamNrOn:      nrOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamDeempOn:   deempOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        case kParamCompOn:    compOn_.store(registry_.toPlain(tag, value) > 0.5); break;
        default: break;
    }
    paramsDirty_.store(true);
}

// ---------------------------------------------------------------------------
// applyState — worker or host thread
// ---------------------------------------------------------------------------

void PluginProcessor::applyState(const ProcessorState& state) {
    // Map every registry param from its plain value to normalized.
    registry_.setValue(kParamMode,     registry_.toNormalized(kParamMode,     state.mode));
    registry_.setValue(kParamFreqKhz,  registry_.toNormalized(kParamFreqKhz,  state.freqKhz));
    registry_.setValue(kParamLowCut,   registry_.toNormalized(kParamLowCut,   state.lowCut));
    registry_.setValue(kParamHighCut,  registry_.toNormalized(kParamHighCut,  state.highCut));
    registry_.setValue(kParamAgcOn,    registry_.toNormalized(kParamAgcOn,    state.agcOn));
    registry_.setValue(kParamAgcHang,  registry_.toNormalized(kParamAgcHang,  state.agcHang));
    registry_.setValue(kParamAgcThresh, registry_.toNormalized(kParamAgcThresh, state.agcThresh));
    registry_.setValue(kParamAgcSlope, registry_.toNormalized(kParamAgcSlope, state.agcSlope));
    registry_.setValue(kParamAgcDecay, registry_.toNormalized(kParamAgcDecay, state.agcDecay));
    registry_.setValue(kParamAgcManGain, registry_.toNormalized(kParamAgcManGain, state.agcManGain));
    registry_.setValue(kParamVolume,   registry_.toNormalized(kParamVolume,   state.volume));
    registry_.setValue(kParamMute,     registry_.toNormalized(kParamMute,     state.mute));
    registry_.setValue(kParamSquelchOn, registry_.toNormalized(kParamSquelchOn, state.squelchOn));
    registry_.setValue(kParamSquelchThr, registry_.toNormalized(kParamSquelchThr, state.squelchThr));
    registry_.setValue(kParamNbOn,     registry_.toNormalized(kParamNbOn,     state.nbOn));
    registry_.setValue(kParamNbThresh, registry_.toNormalized(kParamNbThresh, state.nbThresh));
    registry_.setValue(kParamNrOn,     registry_.toNormalized(kParamNrOn,     state.nrOn));
    registry_.setValue(kParamDeempOn,  registry_.toNormalized(kParamDeempOn,  state.deempOn));
    registry_.setValue(kParamCompOn,   registry_.toNormalized(kParamCompOn,   state.compOn));
    registry_.setValue(kParamWfOn,     registry_.toNormalized(kParamWfOn,     state.wfOn));
    registry_.setValue(kParamWfSpeed,  registry_.toNormalized(kParamWfSpeed,  state.wfSpeed));
    registry_.setValue(kParamWfZoom,   registry_.toNormalized(kParamWfZoom,   state.wfZoom));
    registry_.setValue(kParamWfMaxDb,  registry_.toNormalized(kParamWfMaxDb,  state.wfMaxDb));
    registry_.setValue(kParamWfMinDb,  registry_.toNormalized(kParamWfMinDb,  state.wfMinDb));
    registry_.setValue(kParamWfComp,   registry_.toNormalized(kParamWfComp,   state.wfComp));
    registry_.setValue(kParamArOn,     registry_.toNormalized(kParamArOn,     state.arOn));
    registry_.setValue(kParamOvOn,     registry_.toNormalized(kParamOvOn,     state.ovOn));

    // Mirror plain values into audio-thread atomics.
    mode_.store(static_cast<int>(state.mode));
    freqKhz_.store(state.freqKhz);
    lowCut_.store(static_cast<int>(state.lowCut));
    highCut_.store(static_cast<int>(state.highCut));
    agcOn_.store(state.agcOn > 0.5);
    agcHang_.store(static_cast<int>(state.agcHang));
    agcThresh_.store(static_cast<int>(state.agcThresh));
    agcSlope_.store(static_cast<int>(state.agcSlope));
    agcDecay_.store(static_cast<int>(state.agcDecay));
    agcManGain_.store(static_cast<int>(state.agcManGain));
    volume_.store(state.volume);
    mute_.store(state.mute > 0.5);
    squelchOn_.store(state.squelchOn > 0.5);
    squelchThr_.store(state.squelchThr);
    nbOn_.store(state.nbOn > 0.5);
    nbThresh_.store(state.nbThresh);
    nrOn_.store(state.nrOn > 0.5);
    deempOn_.store(state.deempOn > 0.5);
    compOn_.store(state.compOn > 0.5);

    // Connect to the saved station (if any) on the worker thread.
    if (!state.station.empty()) {
        worker_.post([this, station = state.station]() {
            connectToStation(station);
        });
    }
}

// ---------------------------------------------------------------------------
// station() — thread-safe snapshot
// ---------------------------------------------------------------------------

std::string PluginProcessor::station() const {
    std::lock_guard<std::mutex> lock(stationMutex_);
    return station_;
}

// ---------------------------------------------------------------------------
// setOnStatus
// ---------------------------------------------------------------------------

void PluginProcessor::setOnStatus(StatusCallback cb) {
    onStatus_ = std::move(cb);
}

void PluginProcessor::setOnLevel(LevelCallback cb) {
    onLevel_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// emitStatus — any thread
// ---------------------------------------------------------------------------

void PluginProcessor::emitStatus(const std::string& status) {
    if (onStatus_) {
        worker_.post([this, status]() {
            if (onStatus_) {
                onStatus_(status);
            }
        });
    }

    // Forward the status to the controller peer via IConnectionPoint so the
    // editor UI reflects "Connecting"/"Connected"/"Error"/"Disconnected".
    // (No-op when the host has not connected controller<->processor.)
    if (auto msg = IPtr<IMessage>(allocateMessage())) {
        msg->setMessageID("NetSDRStation:Status");
        Steinberg::String tmp(status.c_str(), Steinberg::kCP_Utf8);
        msg->getAttributes()->setString("Status", tmp.text16());
        sendMessage(msg);
    }
}

// ---------------------------------------------------------------------------
// sendLevel — worker thread only (posted from the audio thread at ~10 Hz)
// Reads the S-meter atomic (written by the audio thread) and forwards it to
// the local callback AND the controller peer (IMessage "NetSDRStation:Level").
// Never calls eval() from here; the editor does that on the UI thread.
// ---------------------------------------------------------------------------

void PluginProcessor::sendLevel() {
    const float dbm = signalLevelDbM_.load(std::memory_order_relaxed);
    if (onLevel_) {
        onLevel_(dbm);
    }
    if (auto msg = IPtr<IMessage>(allocateMessage())) {
        msg->setMessageID("NetSDRStation:Level");
        msg->getAttributes()->setFloat("Level", dbm);
        sendMessage(msg);
    }
}

} // namespace netsdr
