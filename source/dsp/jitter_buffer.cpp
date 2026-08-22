#include "dsp/jitter_buffer.h"

#include <algorithm>
#include <cmath>

namespace netsdr {

//////////////////////////////////////////////////////////////////////////////
// JitterBuffer implementation
//////////////////////////////////////////////////////////////////////////////

JitterBuffer::JitterBuffer(double sampleRate, double targetDurationMs, double maxCapacityMs)
    : sampleRate_(sampleRate),
      buffer_()
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
}

void JitterBuffer::reset() {
    buffer_.clear();
}

void JitterBuffer::push(const float* samples, std::size_t numSamples) {
    if (numSamples == 0 || samples == nullptr) {
        return;
    }

    // Append new samples at the end (newest).
    buffer_.insert(buffer_.end(), samples, samples + numSamples);

    // Trim from the front if we exceed max capacity.
    // "overflow drops the oldest samples so the buffer never exceeds capacity".
    if (buffer_.size() > maxCapacitySamples_) {
        const std::size_t tooMany = buffer_.size() - maxCapacitySamples_;
        buffer_.erase(buffer_.begin(), buffer_.begin() + tooMany);
    }
}

std::size_t JitterBuffer::pull(float* out, std::size_t maxSamples) {
    if (maxSamples == 0) {
        return 0;
    }

    // If not enough buffered for the target prefill, return 0 (not ready yet).
    if (bufferedMs() < targetDurationMs_) {
        return 0;
    }

    // Copy min(buffered, maxSamples) samples out and remove them.
    const std::size_t n = std::min(available(), maxSamples);

    if (n > 0 && out != nullptr) {
        std::copy(buffer_.begin(), buffer_.begin() + n, out);
    }

    // Remove the copied samples from the front (oldest).
    buffer_.erase(buffer_.begin(), buffer_.begin() + n);

    return n;
}

bool JitterBuffer::isReady() const {
    return bufferedMs() >= targetDurationMs_;
}

std::size_t JitterBuffer::available() const {
    return buffer_.size();
}

double JitterBuffer::bufferedMs() const {
    return samplesToMs(buffer_.size(), sampleRate_);
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