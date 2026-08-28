// Audio-thread implementation of PluginProcessor (M3.7 refactoring):
//   - process()       (IAudioProcessor entry, param routing + render)
//   - renderPipeline() (network queue -> resampler -> jitter buffer -> out)

#include "plugin_processor.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "util/file_logger.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

// ---------------------------------------------------------------------------
// process — audio thread
// ---------------------------------------------------------------------------

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

    // (1b) Flush pending parameter changes to the KiwiSDR server, rate-limited
    // to avoid flooding it with SET commands (freq automation can fire many
    // changes per block). sendPendingParams() runs on the worker thread and
    // sends only the latest atomics values; the trailing edge is guaranteed
    // because paramsDirty_ stays set until the worker drains it.
    if (paramsDirty_.load(std::memory_order_acquire)) {
        const double nowSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (paramSendLimiter_.shouldEmit(nowSeconds)) {
            worker_.post([this]() { sendPendingParams(); });
        }
    }

    // (1c) Push the S-meter level to the UI at ~10 Hz. RMS is computed in
    // renderPipeline() (audio thread) into signalLevelDbM_; the worker thread
    // reads the atomic and forwards it UI-wards (eval is NOT audio-thread-safe).
    {
        const double nowSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (levelSendLimiter_.shouldEmit(nowSeconds)) {
            worker_.post([this]() { sendLevel(); });
        }
        // Waterfall spectrum frames (~10 Hz), computed on the worker thread
        // from the samples pushed into spectrumSamples_ by renderPipeline.
        if (waterfallLimiter_.shouldEmit(nowSeconds)) {
            worker_.post([this]() { sendWaterfall(); });
        }
    }

    // (2) Handle pipeline reset request (posted after reconnect).
    if (resetPipelineFlag_.exchange(false)) {
        if (jitterBuffer_) jitterBuffer_->reset();
        if (adpcmDecoder_) adpcmDecoder_->reset();
        telemetry_.reset();
        lastSequence_ = 0;
        criticalActive_ = false;
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
// renderPipeline — audio thread
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

                // CRITICAL only for genuine overflow/underflow (near capacity
                // or near empty), with hysteresis so a persistent condition is
                // logged once per episode instead of every adjust tick. Normal
                // network jitter (buffer oscillating ~0..600 ms around the
                // 300 ms target) must not fire CRITICAL.
                constexpr double kCriticalOverflowMs = 1600.0;  // near 2000 ms ceiling
                constexpr double kCriticalUnderflowMs = 20.0;   // near empty
                const bool critical = (bufferedMs > kCriticalOverflowMs) ||
                                      (bufferedMs < kCriticalUnderflowMs);
                if (critical && !criticalActive_) {
                    criticalActive_ = true;
                    criticalDriftCount_.fetch_add(1);
                    NETSDR_LOG_INFO(
                        "Clock-drift CRITICAL: bufferMs=%.1f target=%.1f ratio=%.6f",
                        bufferedMs, targetMs, smoothedRatio_);
                } else if (!critical) {
                    criticalActive_ = false;
                }
                if (shouldLog) {
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
        std::fill(out, out + numSamples, 0.0f);
        // During the initial pre-fill the buffer is intentionally empty (the
        // start latch has not engaged yet); that is expected silence, not an
        // underrun, so do not log/count it (avoids startup log spam).
        const bool prefill = jitterBuffer_ && !jitterBuffer_->hasStarted();
        if (!prefill) {
            const int underrunCount = static_cast<int>(telemetry_.underruns.fetch_add(1)) + 1;
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
    }

    // (d) Denormal protection: flush sub-normal values to zero.
    for (std::size_t i = 0; i < numSamples; ++i) {
        if (std::fabs(out[i]) < 1e-30f) {
            out[i] = 0.0f;
        }
    }

    // (e) Apply local volume gain (0..1). Volume is a client-side gain (there is
    // no KiwiSDR `SET vol=` command), so it must be applied here on the output.
    const float vol = static_cast<float>(volume_.load());
    if (vol != 1.0f) {
        for (std::size_t i = 0; i < numSamples; ++i) {
            out[i] *= vol;
        }
    }

    // (f) S-meter: compute RMS over this rendered block (post volume) and store
    // it as dBm. Full-scale (rms=1.0) maps to +10 dBm; floor at -140 dBm.
    // The worker thread delivers this to the UI at ~10 Hz (sendLevel).
    float sumSq = 0.0f;
    for (std::size_t i = 0; i < numSamples; ++i) {
        sumSq += out[i] * out[i];
    }
    const float rms = std::sqrt(sumSq / static_cast<float>(numSamples));
    const float dbm = 20.0f * std::log10(rms + 1e-9f) + 10.0f;
    signalLevelDbM_.store(std::max(-140.0f, dbm), std::memory_order_relaxed);

    // (g) Waterfall: feed the rendered samples into the spectrum queue. The
    // lock-free push is RT-safe; the worker thread computes DFT frames from
    // the queue at ~10 Hz (sendWaterfall). Only when a stream is active.
    if (kiwiClient_ && kiwiClient_->isConnected() && !mute_.load()) {
        for (std::size_t i = 0; i < numSamples; ++i) {
            spectrumSamples_.push(out[i]);
        }
    }
}

} // namespace netsdr
