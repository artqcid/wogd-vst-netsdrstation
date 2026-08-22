#pragma once
// JitterBuffer: SPSC FIFO cushion for network jitter.
// Fed by network thread, drained by DSP thread.
// No locking: caller guarantees single-producer/single-consumer usage.
//
// Conversions (documented in implementation):
//   ms_to_samples(ms, sampleRate) = (ms * sampleRate) / 1000.0  -> rounded to std::size_t
//   samples_to_ms(samples, sampleRate) = (samples * 1000.0) / sampleRate
//
// Overflow policy: drop oldest samples so buffer never exceeds maxCapacityMs.
// The vector acts as a FIFO: push appends at the end, overflow drops from the front.

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

    // Clears all buffered samples.
    void reset();

    // Appends `numSamples` from `samples` (network side).
    // If the buffer would exceed maxCapacityMs worth of samples, the oldest
    // samples are dropped first so the buffer never exceeds capacity.
    void push(const float* samples, std::size_t numSamples);

    // DSP side: pull up to `maxSamples` from the buffer.
    // If the buffered duration is below the target prefill, return 0 (not ready yet).
    // Otherwise copy min(buffered, maxSamples) samples out and remove them from the buffer.
    // Returns the number of samples copied.
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

    // FIFO storage: oldest at begin(), newest at end().
    std::vector<float> buffer_;
};

} // namespace netsdr

#endif // JITTER_BUFFER_H