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

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <sstream>
#include <thread>
#include <vector>

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
    // are destroyed.
    worker_.stop();
    if (kiwiClient_) {
        kiwiClient_->disconnect();
        kiwiClient_.reset();
    }
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
    if (kiwiClient_) {
        kiwiClient_->disconnect();
        kiwiClient_.reset();
    }
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
// IConnectionPoint — receives the setStation message from the controller
// ---------------------------------------------------------------------------

tresult PLUGIN_API PluginProcessor::notify(IMessage* message) {
    if (message == nullptr) {
        return kResultFalse;
    }
    const char* msgId = message->getMessageID();
    if (msgId == nullptr) {
        return kResultFalse;
    }

    if (std::strcmp(msgId, "setStation") == 0) {
        // Read UTF-16 TChar buffer from message attributes.
        std::array<TChar, 256> buf{};
        if (message->getAttributes()->getString(
                "station", buf.data(),
                static_cast<uint32>(buf.size() * sizeof(TChar))) != kResultOk) {
            return kResultFalse;
        }
        std::string stationStr;
        for (TChar c : buf) {
            if (c == 0) break;
            stationStr += static_cast<char>(c);
        }
        connectToStation(stationStr);
        return kResultOk;
    }

    return AudioEffect::notify(message);
}

// ---------------------------------------------------------------------------
// IAudioProcessor
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

tresult PLUGIN_API PluginProcessor::process(ProcessData& data) {
    // (1) Apply host parameter changes for this block.
    if (IParameterChanges* paramChanges = data.inputParameterChanges) {
        const int32 numParams = paramChanges->getParameterCount();
        for (int32 i = 0; i < numParams; ++i) {
            if (IParamValueQueue* queue = paramChanges->getParameterData(i)) {
                ParamValue value    = 0.0;
                int32 sampleOffset  = 0;
                const int32 numPoints = queue->getPointCount();
                if (numPoints > 0 &&
                    queue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                    applyParamValue(queue->getParameterId(), value);
                }
            }
        }
    }

    // (2) Handle pipeline reset request (posted after reconnect).
    if (resetPipelineFlag_.exchange(false)) {
        if (jitterBuffer_) jitterBuffer_->reset();
        if (adpcmDecoder_) adpcmDecoder_->reset();
        telemetry_.reset();
        lastSequence_ = 0;
        NETSDR_LOG_DEBUG("Pipeline reset");
    }

    if (data.numOutputs == 0 || data.numSamples == 0) {
        return kResultOk;
    }

    // (3) Render KiwiSDR pipeline to the first output bus.
    const int32 numChannels = data.outputs[0].numChannels;
    void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);

    if (data.symbolicSampleSize == kSample32) {
        auto** out32 = reinterpret_cast<float**>(out);
        if (numChannels > 0) {
            renderPipeline(out32[0], static_cast<std::size_t>(data.numSamples));
            for (int32 c = 1; c < numChannels; ++c) {
                std::memcpy(out32[c], out32[0],
                            static_cast<std::size_t>(data.numSamples) * sizeof(float));
            }
        }
    } else {
        // 64-bit: render via stack scratch buffer.
        auto** out64 = reinterpret_cast<double**>(out);
        if (numChannels > 0) {
            std::array<float, 4096> scratch{};
            std::size_t remaining = static_cast<std::size_t>(data.numSamples);
            std::size_t offset    = 0;
            while (remaining > 0) {
                const std::size_t chunk = (std::min)(remaining, scratch.size());
                renderPipeline(scratch.data(), chunk);
                for (std::size_t i = 0; i < chunk; ++i) {
                    out64[0][offset + i] = static_cast<double>(scratch[i]);
                }
                offset    += chunk;
                remaining -= chunk;
            }
            for (int32 c = 1; c < numChannels; ++c) {
                std::memcpy(out64[c], out64[0],
                            static_cast<std::size_t>(data.numSamples) * sizeof(double));
            }
        }
    }

    // Apply mute + silence flags.
    if (mute_.load() && numChannels > 0 && numChannels < 64) {
        void** outBuf = getChannelBuffersPointer(processSetup, data.outputs[0]);
        if (data.symbolicSampleSize == kSample32) {
            auto** o = reinterpret_cast<float**>(outBuf);
            for (int32 c = 0; c < numChannels; ++c) {
                std::memset(o[c], 0,
                            static_cast<std::size_t>(data.numSamples) * sizeof(float));
            }
        }
        data.outputs[0].silenceFlags = (1ull << numChannels) - 1;
    } else {
        data.outputs[0].silenceFlags = 0;
    }

    return kResultOk;
}

// ---------------------------------------------------------------------------
// applyParamValue — non-audio thread (host automation / UI messages)
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
}

// ---------------------------------------------------------------------------
// sendPendingParams — worker thread
// ---------------------------------------------------------------------------

void PluginProcessor::sendPendingParams() {
    if (!kiwiClient_ || !kiwiClient_->isConnected()) {
        return;
    }
    if (!paramsDirty_.exchange(false)) {
        return;
    }

    const int modeIdx = mode_.load();
    const std::string modeStr =
        (modeIdx >= 0 && modeIdx < kNumKiwiModes) ? kKiwiModeNames[modeIdx] : kKiwiModeNames[0];

    kiwiClient_->setTuning(modeStr, lowCut_.load(), highCut_.load(), freqKhz_.load());
    kiwiClient_->setAgc(agcOn_.load(), agcHang_.load() > 0, agcThresh_.load(),
                        agcSlope_.load(), agcDecay_.load(), agcManGain_.load());
    kiwiClient_->setSquelch(squelchOn_.load(), squelchThr_.load());
    kiwiClient_->setNb(nbOn_.load(), nbThresh_.load());
    kiwiClient_->setNr(nrOn_.load());
    kiwiClient_->setDeemp(deempOn_.load());
    kiwiClient_->setComp(compOn_.load());
}

// ---------------------------------------------------------------------------
// decodeAndQueue — network thread (KiwiClient binary callback)
// Reconstructed from RAG cache [netsdr_405d31d2c80a], lines 468-576
// ---------------------------------------------------------------------------

void PluginProcessor::decodeAndQueue(const std::string& data) {
    static std::atomic<int> frameCount{0};
    const int currentFrame = frameCount.fetch_add(1);
    const bool shouldLog   = (currentFrame % 100) == 0;

    if (data.empty()) {
        NETSDR_LOG_INFO("decodeAndQueue: empty data received (frame %d)", currentFrame);
        return;
    }
    if (!adpcmDecoder_) {
        NETSDR_LOG_INFO("decodeAndQueue: CRITICAL - no adpcmDecoder (frame %d)", currentFrame);
        return;
    }

    const auto* bytes  = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t numBytes = data.size();

    // KiwiSDR SND frame header: 10 bytes (id[3] + flags[1] + seq[4] + smeter[2]).
    // Audio payload starts at offset 10.
    static constexpr std::size_t kSndHeaderSize   = 10;
    static constexpr std::uint8_t kSndFlagCompressed = 0x10;
    static constexpr std::uint8_t kSndFlagIq         = 0x08;

    const std::uint8_t flags  = numBytes > 3 ? bytes[3] : 0;
    const std::size_t  offset = kSndHeaderSize;

    if (shouldLog) {
        NETSDR_LOG_DEBUG("decodeAndQueue: frame %d, numBytes=%zu, flags=0x%02X",
                         currentFrame, numBytes, flags);
    }

    // IQ mode carries raw I/Q samples, not ADPCM — skip.
    if ((flags & kSndFlagIq) != 0) {
        if (currentFrame == 0) {
            NETSDR_LOG_INFO("decodeAndQueue: IQ mode detected, audio pipeline disabled");
        }
        return;
    }
    // Uncompressed PCM is unexpected; the handshake always requests ADPCM.
    if ((flags & kSndFlagCompressed) == 0) {
        if (currentFrame == 0) {
            NETSDR_LOG_INFO("decodeAndQueue: uncompressed mode, expected ADPCM");
        }
        return;
    }

    // Decode the ADPCM payload into AudioSampleBlocks and push to the queue.
    AudioSampleBlock block;
    std::size_t pos = offset;
    int blocksDecoded = 0;
    int blocksPushed  = 0;

    while (pos < numBytes) {
        const std::size_t chunk = (std::min)(
            numBytes - pos,
            AudioSampleBlock::kMaxSamples / 2);  // 2 nibbles per byte → 2 samples

        adpcmDecoder_->decode(bytes + pos, chunk, block.samples.data());
        block.sampleCount = chunk * 2;
        block.sequence    = sequence_.fetch_add(1);
        blocksDecoded++;

        // Sequence gap detection.
        const std::uint32_t expectedSeq = lastSequence_ + 1;
        if (lastSequence_ != 0 && block.sequence != expectedSeq) {
            telemetry_.sequenceGaps.fetch_add(1);
        }
        lastSequence_ = block.sequence;

        // Bounded push: drop if queue is full (real-time safe, no blocking).
        const std::size_t queueSizeBefore = audioQueue_.sizeApprox();
        audioQueue_.tryPush(block);
        const std::size_t queueSizeAfter  = audioQueue_.sizeApprox();
        if (queueSizeAfter == queueSizeBefore) {
            const int overflow = static_cast<int>(telemetry_.overflows.fetch_add(1)) + 1;
            telemetry_.droppedBlocks.fetch_add(1);
            if (overflow == 1) {
                NETSDR_LOG_INFO("decodeAndQueue: queue FULL, dropping blocks (queueSize=%zu)",
                                queueSizeBefore);
            } else if (shouldLog) {
                NETSDR_LOG_DEBUG("decodeAndQueue: queue FULL, dropped block (overflow=%d)", overflow);
            }
        } else {
            blocksPushed++;
        }
        pos += chunk;
    }

    telemetry_.queueDepth.store(static_cast<std::int32_t>(audioQueue_.sizeApprox()));

    if (shouldLog) {
        NETSDR_LOG_DEBUG("decodeAndQueue: frame %d decoded %d blocks, pushed %d",
                         currentFrame, blocksDecoded, blocksPushed);
    }
    if (blocksDecoded > 0 && blocksPushed == 0 && currentFrame < 10) {
        NETSDR_LOG_INFO("decodeAndQueue: CRITICAL - decoded %d blocks but pushed 0 (frame %d)",
                        blocksDecoded, currentFrame);
    }
}

// ---------------------------------------------------------------------------
// renderPipeline — audio thread (called from process())
// Reconstructed from RAG cache [netsdr_4b1b71a22b59], lines 781-952
// Pipeline: audioQueue_ → Resampler → JitterBuffer push → JitterBuffer pull
// ---------------------------------------------------------------------------

void PluginProcessor::renderPipeline(float* out, std::size_t numSamples) {
    if (out == nullptr || numSamples == 0) {
        return;
    }

    static std::atomic<int> callCount{0};
    const int  currentCall = callCount.fetch_add(1);
    const bool shouldLog   = (currentCall % 1000) == 0;

    // (a) Drain audioQueue_ into Resampler → JitterBuffer.
    if (resampler_ && jitterBuffer_) {
        AudioSampleBlock block;
        std::array<float, 1024> inScratch{};
        std::array<float, 4096> outScratch{};
        std::size_t inputSamplesThisCall = 0;
        int         blocksPopped         = 0;

        while (audioQueue_.pop(block)) {
            blocksPopped++;
            const std::size_t count = block.sampleCount;
            inputSamplesThisCall += count;

            for (std::size_t off = 0; off < count; off += 1024) {
                const std::size_t chunk =
                    (std::min)(static_cast<std::size_t>(1024), count - off);
                for (std::size_t i = 0; i < chunk; ++i) {
                    inScratch[i] =
                        static_cast<float>(block.samples[off + i]) / 32768.0f;
                }
                const std::size_t produced =
                    resampler_->process(inScratch.data(), chunk,
                                        outScratch.data(), outScratch.size());
                if (produced > 0) {
                    jitterBuffer_->push(outScratch.data(), produced);
                }
            }
        }

        if (shouldLog) {
            NETSDR_LOG_DEBUG(
                "renderPipeline: blocksPopped=%d inputSamples=%d queueDepth=%d bufferMs=%.1f",
                blocksPopped, static_cast<int>(inputSamplesThisCall),
                static_cast<int>(audioQueue_.sizeApprox()),
                jitterBuffer_->bufferedMs());
        }

        // Clock-drift compensation: adjust resampler ratio based on buffer level.
        //
        // BUG-2 fix: guard against running when disconnected. Without a live
        // stream bufferMs stays at 0.0, which makes the controller diverge
        // the ratio far above nominal. We still update lastRatioAdjust_ so
        // elapsed does not accumulate while idle.
        const auto now       = std::chrono::steady_clock::now();
        const auto elapsed   = std::chrono::duration<double>(now - lastRatioAdjust_).count();
        const bool isConnected = kiwiClient_ && kiwiClient_->isConnected();

        if (elapsed >= 0.05) {
            lastRatioAdjust_ = now;

            if (isConnected) {
                const double serverRate    = serverSampleRate_.load();
                const double dawRate       = resampler_->outputRate();
                const double nominalRatio  = dawRate / serverRate;
                const double bufferedMs    = jitterBuffer_->bufferedMs();
                const double targetMs      = 300.0;

                const double bufferError        = (targetMs - bufferedMs) / targetMs;
                const double desiredAdjustment  = 1.0 + (bufferError * 0.05);
                const double maxChangePerSecond = 0.01;
                const double maxChange          = maxChangePerSecond * elapsed;
                const double currentAdjustment  = smoothedRatio_ / nominalRatio;
                const double adjustmentDiff     = desiredAdjustment - currentAdjustment;
                const double clampedDiff        =
                    std::clamp(adjustmentDiff, -maxChange, maxChange);
                const double newAdjustment      = currentAdjustment + clampedDiff;

                const double oldRatio = smoothedRatio_;
                smoothedRatio_        = nominalRatio * newAdjustment;
                resampler_->setRatio(smoothedRatio_);

                // BUG-3 fix: log CRITICAL only for genuine overflow/underflow
                // (>300 ms from target). Normal network jitter (±200 ms) must
                // not flood the log.
                if (std::fabs(bufferedMs - targetMs) > 300.0) {
                    criticalDriftCount_.fetch_add(1);
                    NETSDR_LOG_INFO(
                        "Clock-drift CRITICAL: bufferMs=%.1f target=%.1f ratio=%.6f",
                        bufferedMs, targetMs, smoothedRatio_);
                } else if (shouldLog) {
                    NETSDR_LOG_DEBUG(
                        "Clock-drift: bufferMs=%.1f target=%.1f ratio=%.6f->%.6f",
                        bufferedMs, targetMs, oldRatio, smoothedRatio_);
                }
            }
        }

        totalInputSamples_.fetch_add(static_cast<std::int64_t>(inputSamplesThisCall));
        totalOutputSamples_.fetch_add(static_cast<std::int64_t>(numSamples));
    }

    // (b) Pull DAW-rate samples from the jitter buffer.
    std::size_t got = 0;
    if (jitterBuffer_) {
        got = jitterBuffer_->pull(out, numSamples);
        telemetry_.jitterBufferMs.store(
            static_cast<std::int32_t>(jitterBuffer_->bufferedMs()));
    }

    // (c) Underflow concealment: repeat-last-sample fade then silence.
    if (got < numSamples && got > 0) {
        const int   underrunCount = static_cast<int>(telemetry_.underruns.fetch_add(1)) + 1;
        const float lastSample    = out[got - 1];
        constexpr std::size_t kFadeLength = 512;
        const std::size_t fadeSamples =
            (std::min)(kFadeLength, numSamples - got);

        for (std::size_t i = 0; i < fadeSamples; ++i) {
            const float fade = 1.0f - static_cast<float>(i)
                                    / static_cast<float>(fadeSamples);
            out[got + i] = lastSample * fade;
        }
        for (std::size_t i = got + fadeSamples; i < numSamples; ++i) {
            out[i] = 0.0f;
        }

        if (underrunCount <= 5) {
            NETSDR_LOG_INFO("UNDERRUN: got %zu/%zu samples, bufferMs=%.1f (count=%d)",
                            got, numSamples,
                            jitterBuffer_ ? jitterBuffer_->bufferedMs() : 0.0,
                            underrunCount);
        } else {
            NETSDR_LOG_DEBUG("UNDERRUN: got %zu/%zu samples, bufferMs=%.1f (count=%d)",
                             got, numSamples,
                             jitterBuffer_ ? jitterBuffer_->bufferedMs() : 0.0,
                             underrunCount);
        }
    } else if (got == 0) {
        const int underrunCount = static_cast<int>(telemetry_.underruns.fetch_add(1)) + 1;
        std::fill(out, out + numSamples, 0.0f);
        if (underrunCount <= 5) {
            NETSDR_LOG_INFO(
                "UNDERRUN (COMPLETE): requested %zu samples, bufferMs=%.1f (count=%d)",
                numSamples,
                jitterBuffer_ ? jitterBuffer_->bufferedMs() : 0.0,
                underrunCount);
        } else {
            NETSDR_LOG_DEBUG(
                "UNDERRUN (COMPLETE): requested %zu samples, bufferMs=%.1f (count=%d)",
                numSamples,
                jitterBuffer_ ? jitterBuffer_->bufferedMs() : 0.0,
                underrunCount);
        }
    }

    // (d) Denormal protection: flush sub-normal values to zero.
    for (std::size_t i = 0; i < numSamples; ++i) {
        if (std::fabs(out[i]) < 1e-30f) {
            out[i] = 0.0f;
        }
    }
}

// ---------------------------------------------------------------------------
// connectToStation — worker thread
// ---------------------------------------------------------------------------

void PluginProcessor::connectToStation(const std::string& hostPort) {
    if (hostPort.empty()) {
        return;
    }

    // Parse "host:port".
    std::string host;
    std::uint16_t port = kiwiDefaultPort;
    const auto colon = hostPort.rfind(':');
    if (colon != std::string::npos) {
        host = hostPort.substr(0, colon);
        try {
            port = static_cast<std::uint16_t>(std::stoul(hostPort.substr(colon + 1)));
        } catch (...) {
            port = kiwiDefaultPort;
        }
    } else {
        host = hostPort;
    }

    {
        std::lock_guard<std::mutex> lock(stationMutex_);
        station_ = hostPort;
    }

    NETSDR_LOG_INFO("Connecting to station: %s", hostPort.c_str());
    emitStatus("Connecting");

    // Disconnect any existing client first.
    if (kiwiClient_) {
        kiwiClient_->disconnect();
        kiwiClient_.reset();
    }

    KiwiClientConfig cfg;
    cfg.host      = host;
    cfg.port      = port;
    cfg.freqKhz   = freqKhz_.load();
    const int modeIdx = mode_.load();
    cfg.mode = (modeIdx >= 0 && modeIdx < kNumKiwiModes)
               ? kKiwiModeNames[modeIdx]
               : kKiwiModeNames[0];
    cfg.lowCut    = lowCut_.load();
    cfg.highCut   = highCut_.load();
    cfg.agcOn     = agcOn_.load();
    cfg.sampleRate = serverSampleRate_.load();

    kiwiClient_ = std::make_unique<KiwiClient>();

    kiwiClient_->setOnBinaryMessage([this](const std::string& data) {
        decodeAndQueue(data);
    });

    kiwiClient_->setOnOpen([this]() {
        NETSDR_LOG_INFO("Station connected");
        emitStatus("Connected");
        resetPipelineFlag_.store(true);
        paramsDirty_.store(true);
        worker_.post([this]() { sendPendingParams(); });
    });

    kiwiClient_->setOnError([this]() {
        NETSDR_LOG_INFO("Station connection error");
        emitStatus("Error");
    });

    kiwiClient_->setOnClose([this]() {
        NETSDR_LOG_INFO("Station disconnected");
        emitStatus("Disconnected");
    });

    if (!kiwiClient_->connect(cfg)) {
        NETSDR_LOG_INFO("Failed to start connection to %s", hostPort.c_str());
        emitStatus("Error");
        kiwiClient_.reset();
    }
}

} // namespace netsdr
