#include "plugin_processor.h"

#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/pluginids.h"
#include "version.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "base/source/fstreamer.h"

#include <array>
#include <cmath>
#include <cstring>

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

PluginProcessor::PluginProcessor()
    : registry_(createParameterDefinitions())
    , oscillator_(registry_.toPlain(kParamFreq, registry_.value(kParamFreq)), 48000.0) {
    setControllerClass(kControllerUID);
}

tresult PLUGIN_API PluginProcessor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 1);

    // Parameters are registered by the edit controller; the processor only
    // consumes their values via inputParameterChanges in process().

    oscillator_.setFrequency(registry_.toPlain(kParamFreq, registry_.value(kParamFreq)));
    oscillator_.setVolume(registry_.toPlain(kParamVolume, registry_.value(kParamVolume)));
    oscillator_.setMute(registry_.toPlain(kParamMute, registry_.value(kParamMute)) > 0.5);

    freqHz_.store(oscillator_.frequency());
    volume_.store(oscillator_.volume());
    mute_.store(oscillator_.mute());

    worker_.start();
    return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate() {
    worker_.stop();
    return AudioEffect::terminate();
}

tresult PLUGIN_API PluginProcessor::setState(IBStream* state) {
    IBStreamer streamer(state, kLittleEndian);
    float f = 0.f;
    if (streamer.readFloat(f)) {
        freqHz_.store(f);
    }
    if (streamer.readFloat(f)) {
        volume_.store(f);
    }
    int32 m = 0;
    if (streamer.readInt32(m)) {
        mute_.store(m != 0);
    }
    updateOscillatorFromParams();
    return kResultOk;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream* state) {
    IBStreamer streamer(state, kLittleEndian);
    streamer.writeFloat(static_cast<float>(freqHz_.load()));
    streamer.writeFloat(static_cast<float>(volume_.load()));
    streamer.writeInt32(mute_.load() ? 1 : 0);
    return kResultOk;
}

void PluginProcessor::applyParamValue(ParamID tag, ParamValue value) {
    // Called by the host (e.g. automation) on a non-audio thread. We store the
    // new value into the registry and into the audio-thread atomics, then let
    // the DSP pick it up lock-free during process().
    registry_.setValue(tag, value);
    switch (tag) {
        case kParamFreq:
            freqHz_.store(registry_.toPlain(tag, value));
            break;
        case kParamVolume:
            volume_.store(registry_.toPlain(tag, value));
            break;
        case kParamMute:
            mute_.store(registry_.toPlain(tag, value) > 0.5);
            break;
    }
}

tresult PLUGIN_API PluginProcessor::setBusArrangements(SpeakerArrangement* inputs,
                                                       int32 numIns,
                                                       SpeakerArrangement* outputs,
                                                       int32 numOuts) {
    // Accept any arrangement that keeps a single audio output; we synthesize,
    // so the input arrangement is ignored.
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
    oscillator_.reset();
    return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API PluginProcessor::process(ProcessData& data) {
    // (1) Apply any parameter changes from the host for this block.
    if (IParameterChanges* paramChanges = data.inputParameterChanges) {
        int32 numParams = paramChanges->getParameterCount();
        for (int32 i = 0; i < numParams; ++i) {
            if (IParamValueQueue* queue = paramChanges->getParameterData(i)) {
                ParamValue value = 0.0;
                int32 sampleOffset = 0;
                int32 numPoints = queue->getPointCount();
                if (numPoints > 0 &&
                    queue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                    applyParamValue(queue->getParameterId(), value);
                }
            }
        }
    }

    // (2) Pull the audio-thread parameter snapshot into the DSP.
    oscillator_.setFrequency(freqHz_.load());
    oscillator_.setVolume(volume_.load());
    oscillator_.setMute(mute_.load());

    if (data.numOutputs == 0 || data.numSamples == 0) {
        return kResultOk;
    }

    // (3) Render the sine into the output buffers.
    int32 numChannels = data.outputs[0].numChannels;
    void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);

    if (data.symbolicSampleSize == kSample32) {
        // Render one mono buffer, then duplicate to all channels.
        // Use the first channel's buffer as the scratch render target.
        auto** out32 = reinterpret_cast<float**>(out);
        const int32 first = numChannels > 0 ? 0 : -1;
        if (first >= 0) {
            oscillator_.render(out32[first], static_cast<std::size_t>(data.numSamples));
            for (int32 c = 1; c < numChannels; ++c) {
                std::memcpy(out32[c], out32[first],
                            static_cast<std::size_t>(data.numSamples) * sizeof(float));
            }
        }
    } else {
        auto** out64 = reinterpret_cast<double**>(out);
        const int32 first = numChannels > 0 ? 0 : -1;
        if (first >= 0) {
            // Render through a small stack buffer and widen to double.
            std::array<float, 4096> scratch{};
            std::size_t remaining = static_cast<std::size_t>(data.numSamples);
            std::size_t offset = 0;
            while (remaining > 0) {
                const std::size_t chunk = remaining < scratch.size() ? remaining : scratch.size();
                oscillator_.render(scratch.data(), chunk);
                for (std::size_t i = 0; i < chunk; ++i) {
                    out64[first][offset + i] = static_cast<double>(scratch[i]);
                }
                offset += chunk;
                remaining -= chunk;
            }
            for (int32 c = 1; c < numChannels; ++c) {
                std::memcpy(out64[c], out64[first],
                            static_cast<std::size_t>(data.numSamples) * sizeof(double));
            }
        }
    }

    data.outputs[0].silenceFlags = 0;
    return kResultOk;
}

void PluginProcessor::updateOscillatorFromParams() {
    oscillator_.setFrequency(freqHz_.load());
    oscillator_.setVolume(volume_.load());
    oscillator_.setMute(mute_.load());
}

} // namespace netsdr
