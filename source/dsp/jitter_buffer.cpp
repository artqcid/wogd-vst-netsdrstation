#include "dsp/jitter_buffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace netsdr {

//////////////////////////////////////////////////////////////////////////////
// JitterBuffer implementation
//////////////////////////////////////////////////////////////////////////////

JitterBuffer::JitterBuffer(double sampleRate, double targetDurationMs, double maxCapacityMs)
    : sampleRate_(sampleRate)
{
    // Clamp targetDurationMs > 0
    if (targetDurationMs <= 0.0) {
        targetDurationMs_ = 1.0;  // minimal positive
    } else {
        targetDurationMs_ = targetDurationMs;
    }

    // Ensure maxCapacityMs > targetDurationMs
    if (maxCapacityMs <= targetDurationMs_) {
        maxCapacityMs_ = targetDurationMs_ + 1.0;
    } else {
        maxCapacityMs_ = maxCapacityMs;
    }

    // Pre-compute sample capacities (rounded).
    maxCapacitySamples_ = msToSamples(maxCapacityMs_, sampleRate_);
    targetDurationSamples_ = msToSamples(targetDurationMs_, sampleRate_);

    // Pre-allocate the ring storage exactly once. All allocation happens here;
    // push()/pull()/reset() never allocate.
    buffer_.resize(maxCapacitySamples_);
}

void JitterBuffer::reset() {
    head_ = 0;
    count_ = 0;
    started_ = false;
}

void JitterBuffer::push(const float* samples, std::size_t numSamples) {
    if (numSamples == 0 || samples == nullptr) {
        return;
    }

    // Input larger than the whole ring: keep only the newest `capacity` samples
    // (everything currently buffered plus the older incoming samples are dropped).
    if (numSamples >= maxCapacitySamples_) {
        const std::size_t keep = maxCapacitySamples_;
        std::memcpy(buffer_.data(), samples + (numSamples - keep), keep * sizeof(float));
        head_ = 0;
        count_ = keep;
        return;
    }

    // Append new samples at the logical end (newest), wrapping around the ring.
    const std::size_t writePos = (head_ + count_) % maxCapacitySamples_;
    const std::size_t first = std::min(numSamples, maxCapacitySamples_ - writePos);
    std::memcpy(buffer_.data() + writePos, samples, first * sizeof(float));
    if (numSamples > first) {
        std::memcpy(buffer_.data(), samples + first, (numSamples - first) * sizeof(float));
    }
    count_ += numSamples;

    // Overflow: drop the oldest samples so only the newest `capacity` remain.
    if (count_ > maxCapacitySamples_) {
        const std::size_t drop = count_ - maxCapacitySamples_;
        head_ = (head_ + drop) % maxCapacitySamples_;
        count_ = maxCapacitySamples_;
    }
}

std::size_t JitterBuffer::pull(float* out, std::size_t maxSamples) {
    if (maxSamples == 0) {
        return 0;
    }

    // Pre-fill gate: only before the buffer has started playing. Once started,
    // the gate never re-arms, so a mid-stream dip below the target produces at
    // most one silent block instead of bursts of silence.
    if (!started_ && bufferedMs() < targetDurationMs_) {
        return 0;
    }

    // Copy min(buffered, maxSamples) samples out, oldest first, wrapping around.
    const std::size_t n = std::min(count_, maxSamples);
    if (n > 0 && out != nullptr) {
        const std::size_t first = std::min(n, maxCapacitySamples_ - head_);
        std::memcpy(out, buffer_.data() + head_, first * sizeof(float));
        if (n > first) {
            std::memcpy(out + first, buffer_.data(), (n - first) * sizeof(float));
        }
    }

    // Remove the copied samples from the front (oldest).
    head_ = (head_ + n) % maxCapacitySamples_;
    count_ -= n;

    // Engage the start latch on the first delivered samples.
    if (n > 0) {
        started_ = true;
    }

    return n;
}

bool JitterBuffer::isReady() const {
    return bufferedMs() >= targetDurationMs_;
}

std::size_t JitterBuffer::available() const {
    return count_;
}

double JitterBuffer::bufferedMs() const {
    return samplesToMs(count_, sampleRate_);
}

// msToSamples: (ms * sampleRate) / 1000.0  -> rounded to nearest std::size_t
std::size_t JitterBuffer::msToSamples(double ms, double sampleRate) {
    return static_cast<std::size_t>(std::round(ms * sampleRate / 1000.0));
}

// samplesToMs: (samples * 1000.0) / sampleRate
double JitterBuffer::samplesToMs(std::size_t samples, double sampleRate) {
    return (static_cast<double>(samples) * 1000.0) / sampleRate;
}

} // namespace netsdr