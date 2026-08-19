// Unit tests for the SineOscillator (Milestone M1.4/M1.9).
// Covers: frequency (Goertzel peak), amplitude == volume, phase continuity
// across blocks, mute == exact zeros.

#include "catch.hpp"
#include "dsp/sine_oscillator.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

// Goertzel single-bin magnitude for a given frequency and sample rate.
double goertzelMagnitude(const std::vector<float>& samples, double freq, double sampleRate) {
    const double w = 2.0 * kPi * freq / sampleRate;
    const double coeff = 2.0 * std::cos(w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (float s : samples) {
        s0 = s + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const std::size_t n = samples.size();
    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return 2.0 * std::sqrt(power) / static_cast<double>(n);
}

} // namespace

TEST_CASE("SineOscillator: frequency peak matches the set frequency", "[dsp][sine]") {
    const double sampleRate = 48000.0;
    const double freq = 1000.0;
    netsdr::SineOscillator osc(freq, sampleRate);
    osc.setVolume(1.0);

    std::vector<float> out(4800); // 0.1 s
    osc.render(out.data(), out.size());

    // The peak magnitude should be well above noise and concentrated at freq.
    const double peak = goertzelMagnitude(out, freq, sampleRate);
    REQUIRE(peak > 0.9);

    // A far-away bin should be (almost) zero.
    const double other = goertzelMagnitude(out, 2000.0, sampleRate);
    REQUIRE(other < 0.01);
}

TEST_CASE("SineOscillator: output amplitude equals the volume parameter", "[dsp][sine]") {
    const double sampleRate = 48000.0;
    const double freq = 440.0;
    netsdr::SineOscillator osc(freq, sampleRate);
    osc.setVolume(0.5);

    std::vector<float> out(4800);
    osc.render(out.data(), out.size());

    const double peak = goertzelMagnitude(out, freq, sampleRate);
    // amplitude == volume (0.5)
    REQUIRE(std::abs(peak - 0.5) < 0.02);
}

TEST_CASE("SineOscillator: phase is continuous across block boundaries", "[dsp][sine]") {
    const double sampleRate = 48000.0;
    const double freq = 1000.0;
    netsdr::SineOscillator osc(freq, sampleRate);
    osc.setVolume(1.0);

    // Render in two blocks and compare the boundary with a continuous render.
    std::vector<float> twoBlocks(1600);
    osc.render(twoBlocks.data(), 800);
    osc.render(twoBlocks.data() + 800, 800);

    // Re-render continuously from a fresh oscillator for reference.
    netsdr::SineOscillator ref(freq, sampleRate);
    ref.setVolume(1.0);
    std::vector<float> continuous(1600);
    ref.render(continuous.data(), continuous.size());

    // The sample immediately before the boundary in the two-block render must
    // equal the continuous one (no discontinuity/click at sample 799 -> 800).
    const std::size_t boundary = 800;
    REQUIRE(std::abs(twoBlocks[boundary] - continuous[boundary]) < 1e-6);
    REQUIRE(std::abs(twoBlocks[boundary - 1] - continuous[boundary - 1]) < 1e-6);
}

TEST_CASE("SineOscillator: mute produces exact zeros", "[dsp][sine]") {
    netsdr::SineOscillator osc(440.0, 48000.0);
    osc.setVolume(1.0);
    osc.setMute(true);

    std::vector<float> out(1024);
    osc.render(out.data(), out.size());

    for (float s : out) {
        REQUIRE(s == 0.0f);
    }
}

TEST_CASE("SineOscillator: unmute resumes without a phase jump", "[dsp][sine]") {
    const double sampleRate = 48000.0;
    netsdr::SineOscillator osc(440.0, sampleRate);
    osc.setVolume(1.0);

    std::vector<float> pre(1024);
    osc.render(pre.data(), pre.size());

    // Mute renders zeros but keeps the phase running.
    std::vector<float> muted(512);
    osc.setMute(true);
    osc.render(muted.data(), muted.size());

    // Unmute: output must be a continuation of the un-muted phase.
    std::vector<float> post(512);
    osc.setMute(false);
    osc.render(post.data(), post.size());

    // Reference continuous render for phase comparison after the mute span.
    netsdr::SineOscillator ref(440.0, sampleRate);
    ref.setVolume(1.0);
    std::vector<float> refBuf(1024 + 512 + 512);
    ref.render(refBuf.data(), refBuf.size());

    const std::size_t postStart = 1024 + 512;
    for (std::size_t i = 0; i < 512; ++i) {
        REQUIRE(std::abs(post[i] - refBuf[postStart + i]) < 1e-6);
    }
}
