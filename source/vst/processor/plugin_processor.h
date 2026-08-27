#pragma once
// VST3 audio processor for the NetSDRStation KiwiSDR receiver (Milestone M3).
// The audio thread pulls decoded audio from a lock-free SPSC queue, resamples
// it and feeds a jitter buffer; all network/ADPCM work happens on the network
// thread. The audio thread stays lock-free and allocation-free in steady state.

#include "dsp/ima_adpcm.h"
#include "dsp/jitter_buffer.h"
#include "dsp/rate_limiter.h"
#include "dsp/resampler.h"
#include "network/kiwi_client.h"
#include "threading/audio_sample_queue.h"
#include "threading/worker_thread.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/parameter_registry.h"
#include "vst/common/processor_state.h"
#include "vst/processor/pipeline_telemetry.h"

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace netsdr {

class PluginProcessor : public Steinberg::Vst::AudioEffect {
public:
    PluginProcessor();
    // Stops the worker thread and disconnects before members are destroyed, so
    // a pending worker callback can never access freed members (use-after-free).
    ~PluginProcessor() override;

    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new PluginProcessor());
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

    // IConnectionPoint: receives the setStation message from the controller.
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

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

    // Applies a full processor state (params + station). Connects to the
    // station on the worker thread when state.station is non-empty.
    void applyState(const ProcessorState& state);

    // Register a callback that receives connection status strings:
    // "Connecting", "Connected", "Error", "Disconnected".
    using StatusCallback = std::function<void(const std::string&)>;
    void setOnStatus(StatusCallback cb);

    // Current station "host:port" (thread-safe snapshot; may be empty).
    std::string station() const;

    // For testing only: expose smoothedRatio_ so tests can verify the clock-drift
    // controller does not aggressively adjust the ratio when no KiwiSDR is connected.
    double smoothedRatioForTest() const { return smoothedRatio_; }

    // For testing only: count of CRITICAL clock-drift log events since last reset.
    int criticalDriftCountForTest() const { return criticalDriftCount_.load(); }
    void resetCriticalDriftCountForTest() { criticalDriftCount_.store(0); }

private:
    void connectToStation(const std::string& hostPort);   // worker thread
    void disconnectStation();                             // worker thread
    void emitStatus(const std::string& status);           // network thread -> worker -> UI
    void sendPendingParams();                             // worker thread
    void decodeAndQueue(const std::string& data);         // network thread
    void renderPipeline(float* out, std::size_t numSamples);

    ParameterRegistry registry_;
    WorkerThread worker_;
    std::unique_ptr<netsdr::KiwiClient> kiwiClient_;
    std::unique_ptr<netsdr::ImaAdpcmDecoder> adpcmDecoder_;
    netsdr::AudioSampleQueue audioQueue_{512};
    std::unique_ptr<netsdr::Resampler> resampler_;
    std::unique_ptr<netsdr::JitterBuffer> jitterBuffer_;
    // Rate-limits pushing of pending parameter changes to the KiwiSDR server
    // (~20 sends/s max). Prevents flooding the server when the host delivers
    // dense automation (esp. frequency). Accessed only from the audio thread.
    netsdr::RateLimiter paramSendLimiter_{20.0};

    // Status callback for UI binding.
    StatusCallback onStatus_;

    // Audio-thread parameter snapshot (written from non-audio threads via atomics).
    std::atomic<double> freqKhz_{kDefaultFreqKhz};
    std::atomic<int> mode_{0};
    std::atomic<int> lowCut_{kDefaultLowCut};
    std::atomic<int> highCut_{kDefaultHighCut};
    std::atomic<bool> agcOn_{true};
    std::atomic<int> agcHang_{0};
    std::atomic<int> agcThresh_{-100};
    std::atomic<int> agcSlope_{6};
    std::atomic<int> agcDecay_{1000};
    std::atomic<int> agcManGain_{50};
    std::atomic<bool> squelchOn_{false};
    std::atomic<double> squelchThr_{0.5};
    std::atomic<bool> nbOn_{false};
    std::atomic<double> nbThresh_{0.5};
    std::atomic<bool> nrOn_{false};
    std::atomic<bool> deempOn_{true};
    std::atomic<bool> compOn_{false};
    std::atomic<double> volume_{1.0};
    std::atomic<bool> mute_{false};

    // Shared state between host thread and worker thread.
    std::string station_;
    mutable std::mutex stationMutex_;
    std::atomic<bool> paramsDirty_{false};
    std::atomic<bool> resetPipelineFlag_{false};
    std::atomic<std::uint32_t> sequence_{0};

    // Pipeline telemetry (lock-free counters for diagnostics).
    PipelineTelemetry telemetry_;

    // Clock-drift compensation: track actual server sample rate.
    std::atomic<double> serverSampleRate_{12000.0};
    std::atomic<std::int64_t> totalInputSamples_{0};
    std::atomic<std::int64_t> totalOutputSamples_{0};
    std::uint32_t lastSequence_{0};

    // Smoothed clock-drift compensation (avoid aggressive ratio changes).
    double smoothedRatio_{1.0};
    std::chrono::steady_clock::time_point lastRatioAdjust_{std::chrono::steady_clock::now()};

    // For testing only: counts CRITICAL clock-drift log events (|bufferMs-target|>300ms).
    std::atomic<int> criticalDriftCount_{0};

    // Hysteresis latch for CRITICAL logging (audio-thread only): stays true while
    // the buffer is in the critical band so CRITICAL is logged once per episode.
    bool criticalActive_{false};
};

} // namespace netsdr