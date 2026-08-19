#include "sine_oscillator.h"

#include <algorithm>

namespace netsdr {

namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
}

SineOscillator::SineOscillator(double frequency, double sampleRate)
    : frequency_(frequency)
    , sampleRate_(sampleRate)
    , phase_(0.0)
    , phaseIncrement_(0.0)
    , volume_(1.0)
    , mute_(false) {
    setFrequency(frequency);
}

void SineOscillator::setFrequency(double frequency) {
    frequency_ = std::max(0.0, frequency);
    phaseIncrement_ = (kTwoPi * frequency_) / sampleRate_;
}

void SineOscillator::setVolume(double volume) {
    volume_ = std::clamp(volume, 0.0, 1.0);
}

void SineOscillator::setMute(bool mute) {
    mute_ = mute;
}

void SineOscillator::render(float* out, std::size_t numSamples) {
    const double gain = mute_ ? 0.0 : volume_;
    for (std::size_t i = 0; i < numSamples; ++i) {
        out[i] = static_cast<float>(std::sin(phase_) * gain);
        phase_ += phaseIncrement_;
        if (phase_ >= kTwoPi) {
            phase_ -= kTwoPi;
        }
    }
}

} // namespace netsdr
