#pragma once
// A minimal sine oscillator built on a phase accumulator.
//
// Pure C++ with no VST3 dependency so it can be unit-tested in isolation.
// The oscillator renders samples into an output buffer, keeps the phase
// continuous across calls (phase-continuity test), and supports a gain and a
// mute flag. Rendering is allocation-free.

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace netsdr {

class SineOscillator {
public:
    // frequency: oscillator frequency in Hz.
    // sampleRate: output sample rate in Hz.
    SineOscillator(double frequency = 440.0, double sampleRate = 48000.0);

    // Sets the oscillator frequency (Hz). The next sample uses the new value.
    void setFrequency(double frequency);

    // Current oscillator frequency in Hz.
    double frequency() const { return frequency_; }

    // Sets the output sample rate (Hz) and recomputes the phase increment so
    // the oscillator produces the correct pitch when the host changes rate.
    void setSampleRate(double sampleRate);

    // Current output sample rate in Hz.
    double sampleRate() const { return sampleRate_; }

    // Sets the output gain (0..1). Scales the amplitude of the rendered block.
    void setVolume(double volume);

    // Current output gain (0..1).
    double volume() const { return volume_; }

    // Mutes the output (renders exact zeros) while keeping the phase running.
    void setMute(bool mute);

    // Whether the output is currently muted.
    bool mute() const { return mute_; }

    // Renders `numSamples` into `out`. The gain parameter of a block is
    // `volume`; output is `sin(phase) * volume` (or 0 when muted).
    void render(float* out, std::size_t numSamples);

    // Current phase in the range [0, 2*pi). Exposed for tests.
    double phase() const { return phase_; }

    // Resets the phase to zero (start of a fresh buffer).
    void reset() { phase_ = 0.0; }

private:
    double frequency_;
    double sampleRate_;
    double phase_;
    double phaseIncrement_;
    double volume_;
    bool mute_;
};

} // namespace netsdr
