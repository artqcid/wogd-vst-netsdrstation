#pragma once
// VST3 audio processor for the NetSDRStation sine-synth proof (Milestone M1).
//
// The processor owns the DSP core (SineOscillator) and the parameter snapshot.
// The audio thread only reads atomics and renders samples: no locks, no
// allocation, no network (real-time safety). A WorkerThread receives messages
// (e.g. future network bridge) decoupled from the audio thread.

#include "dsp/sine_oscillator.h"
#include "threading/worker_thread.h"
#include "vst/common/parameter_registry.h"

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <atomic>
#include <vector>

namespace netsdr {

class PluginProcessor : public Steinberg::Vst::AudioEffect {
public:
    PluginProcessor();

    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new PluginProcessor());
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

    // IAudioProcessor
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs,
                                                     Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs,
                                                     Steinberg::int32 numOuts) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

    // The shared definition set (constant after construction).
    const ParameterRegistry& registry() const { return registry_; }

    // Worker thread used to decouple non-real-time work from the audio thread.
    WorkerThread& worker() { return worker_; }

    // Applies a host automation value. Public for unit-test access.
    void applyParamValue(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value);

private:
    void updateOscillatorFromParams();

    ParameterRegistry registry_;
    SineOscillator oscillator_;
    WorkerThread worker_;

    // Audio-thread parameter snapshot (written from non-audio threads via atomics).
    std::atomic<double> freqHz_{440.0};
    std::atomic<double> volume_{1.0};
    std::atomic<bool> mute_{false};
};

} // namespace netsdr
