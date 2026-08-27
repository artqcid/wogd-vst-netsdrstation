#pragma once
// JitterBuffer: SPSC FIFO cushion for network jitter.
// Fed by network thread, drained by DSP thread.
// No locking: caller guarantees single-producer/single-consumer usage.
//
// Real-time safety: the storage is a pre-allocated fixed-capacity ring buffer.
// All allocation happens in the constructor; push()/pull()/reset() are
// allocation-free and lock-free (audio-thread safe).
//
// Conversions (documented in implementation):
//   ms_to_samples(ms, sampleRate) = (ms * sampleRate) / 1000.0  -> rounded to std::size_t
//   samples_to_ms(samples, sampleRate) = (samples * 1000.0) / sampleRate
//
// Overflow policy: drop the OLDEST samples so the buffer never exceeds
// maxCapacityMs. The ring buffer keeps the newest `maxCapacitySamples` samples.

#ifndef JITTER_BUFFER_H
#define JITTER_BUFFER_H

#include <cstddef>
#include <vector>

namespace netsdr {

class JitterBuffer {
public:
    // Constructs the jitter buffer.
    // sampleRate: samples per second (e.g. 12000.0).
    // targetDurationMs: prefill threshold in ms (must be > 0; clamped if <= 0).
    //   After construction: target_ < maxCapacityMs always holds.
    // maxCapacityMs: hard capacity ceiling in ms (clamped > targetDurationMs if needed).
    JitterBuffer(double sampleRate, double targetDurationMs, double maxCapacityMs);

    // Clears all buffered samples (allocation-free).
    void reset();

    // Appends `numSamples` from `samples` (network side). Allocation-free.
    // If the buffer would exceed maxCapacityMs worth of samples, the oldest
    // samples are dropped first so the buffer never exceeds capacity.
    void push(const float* samples, std::size_t numSamples);

    // DSP side: pull up to `maxSamples` from the buffer. Allocation-free.
    // Pre-fill gate (start latch): while the buffer has never delivered audio
    // (`started_ == false`), pull returns 0 until the buffered duration reaches
    // the target prefill. Once the buffer has started playing, the gate does
    // NOT re-arm: pull returns whatever is available (0 on underflow), so a
    // mid-stream dip below the target produces at most one silent block
    // instead of a burst of silence (real-time safe, JUCE-style "always fill").
    // reset() clears the start latch.
    std::size_t pull(float* out, std::size_t maxSamples);

    // True when buffered duration >= targetDurationMs.
    bool isReady() const;

    // Number of buffered samples.
    std::size_t available() const;

    // Buffered duration in ms (= available() / sampleRate * 1000).
    double bufferedMs() const;

private:
    // Convert ms to samples (rounded).
    static std::size_t msToSamples(double ms, double sampleRate);

    // Convert samples to ms.
    static double samplesToMs(std::size_t samples, double sampleRate);

    double sampleRate_ = 48000.0;
    double targetDurationMs_ = 100.0;
    double maxCapacityMs_ = 500.0;
    std::size_t maxCapacitySamples_ = 0;
    std::size_t targetDurationSamples_ = 0;

    // Ring-buffer storage: pre-allocated once in the constructor. Oldest sample
    // at index head_, newest at (head_ + count_ - 1) % capacity.
    std::vector<float> buffer_;
    std::size_t head_ = 0;   // index of the oldest buffered sample
    std::size_t count_ = 0;  // number of buffered samples (<= maxCapacitySamples_)
    bool started_ = false;   // start latch: pre-fill gate only before first pull
};

} // namespace netsdr

#endif // JITTER_BUFFER_H