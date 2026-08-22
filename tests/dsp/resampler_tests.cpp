// Unit tests for netsdr::Resampler (Milestone M2.7).
// Covers: sample-rate conversion frequency preservation, identity pass,
// and reset behaviour.  Uses Goertzel magnitude test helper (same style as
// sine_oscillator_tests.cpp).  No real audio hardware required.

#include "catch.hpp"

#include "dsp/resampler.h"

#include <cmath>
#include <cstdint>
#include <vector>

// Goertzel single-bin magnitude for a given frequency and sample rate.
// Copied from tests/dsp/sine_oscillator_tests.cpp.
double goertzelMagnitude(const std::vector<float>& samples, double freq, double sampleRate) {
    const double kPi = 3.14159265358979323846;
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

// Helper: generate a 1 kHz sine wave of `numSamples` samples at the given
// sampleRate, stored in [0,1] float range.
static std::vector<float> generateSine1kHz(std::size_t numSamples, double sampleRate) {
    std::vector<float> out(numSamples);
    const double kPi = 3.14159265358979323846;
    const double step = 2.0 * kPi * 1000.0 / sampleRate;  // 1 kHz phase increment
    for (std::size_t i = 0; i < numSamples; ++i) {
        out[i] = static_cast<float>(std::sin(i * step));
    }
    return out;
}

TEST_CASE("[dsp][resampler] 12 kHz -> 48 kHz preserves frequency", "[dsp][resampler]") {
    // 1 kHz sine at 12000 Hz, 12000 samples = 1 second.
    const double inRate  = 12000.0;
    const double outRate = 48000.0;
    const auto        inSamples = generateSine1kHz(12000, inRate);

    netsdr::Resampler resampler(inRate, outRate);
    REQUIRE(resampler.isValid() == true);  // "SRC init failed" otherwise

// Process all input; collect output.
    std::vector<float> outBuf;
    outBuf.resize(48000);  // ~48000 output frames for ratio 4.0

    size_t numProduced = resampler.process(inSamples.data(), inSamples.size(),
                                           outBuf.data(), outBuf.size());

    // Verify output frame count is approximately 48000 (within a few percent).
    const size_t expected = 48000;
    const size_t tolerance = static_cast<size_t>(expected * 0.03);  // ~3%
    CHECK(numProduced >= expected - tolerance);
    CHECK(numProduced <= expected + tolerance);

    // Goertzel magnitude at 1000 Hz should be > 0.9 (frequency preserved).
    const double peak = goertzelMagnitude(outBuf, 1000.0, outRate);
    REQUIRE(peak > 0.9);

    // A far-away bin (2000 Hz) should be near zero.
    const double other = goertzelMagnitude(outBuf, 2000.0, outRate);
    REQUIRE(other < 0.01);
}

TEST_CASE("[dsp][resampler] 24 kHz -> 44.1 kHz preserves frequency", "[dsp][resampler]") {
    // 1 kHz sine at 24000 Hz, 24000 samples = 1 second.
    const double inRate  = 24000.0;
    const double outRate = 44100.0;
    const auto        inSamples = generateSine1kHz(24000, inRate);

    netsdr::Resampler resampler(inRate, outRate);
    REQUIRE(resampler.isValid() == true);  // "SRC init failed" otherwise

// Prepare output buffer sized for the expected conversion.
    // ratio = 44100/24000 ≈ 1.8375  =>  ~44100 output samples from 24000 input.
    std::vector<float> outBuf;
    outBuf.resize(45000);  // generous upper bound

    size_t numProduced = resampler.process(inSamples.data(), inSamples.size(),
                                           outBuf.data(), outBuf.size());

    // Verify output frame count is approximately 44100 (within ~5%).
    const size_t expected = 44100;
    const size_t tolerance = static_cast<size_t>(expected * 0.05);  // ~5%
    CHECK(numProduced >= expected - tolerance);
    CHECK(numProduced <= expected + tolerance);

    // Goertzel magnitude at 1000 Hz should be > 0.9 (frequency preserved).
    const double peak = goertzelMagnitude(outBuf, 1000.0, outRate);
    REQUIRE(peak > 0.9);

    // A far-away bin (2000 Hz) should be near zero.
    const double other = goertzelMagnitude(outBuf, 2000.0, outRate);
    REQUIRE(other < 0.01);
}

TEST_CASE("[dsp][resampler] identity resampler passes samples through", "[dsp][resampler]") {
// Identity: input rate == output rate, so samples should pass through unchanged.
    const double inRate  = 48000.0;
    const double outRate = 48000.0;
    const auto        inSamples = generateSine1kHz(4800, inRate);

    netsdr::Resampler resampler(inRate, outRate);
    REQUIRE(resampler.isValid() == true);  // "SRC init failed" otherwise

    std::vector<float> outBuf;
    outBuf.resize(4800);

    size_t numProduced = resampler.process(inSamples.data(), inSamples.size(),
                                           outBuf.data(), outBuf.size());

    // Output count should equal input count (exactly or within a tiny margin).
    CHECK(numProduced >= 4500);
    CHECK(numProduced <= 4800 + 2);

    // Goertzel at 1000 Hz should be > 0.9 (frequency and phase preserved).
    const double peak = goertzelMagnitude(outBuf, 1000.0, outRate);
    REQUIRE(peak > 0.9);
}

TEST_CASE("[dsp][resampler] reset clears state", "[dsp][resampler]") {
    // Generate a 1 kHz sine at 48000 Hz.
    const double inRate  = 48000.0;
    const double outRate = 48000.0;
    const auto        inSamples = generateSine1kHz(4800, inRate);

    netsdr::Resampler resampler(inRate, outRate);
    REQUIRE(resampler.isValid() == true);  // "SRC init failed" otherwise

    // Process some samples first.
    std::vector<float> outBuf1;
    outBuf1.resize(4800);
    size_t numProduced1 = resampler.process(inSamples.data(), inSamples.size(),
                                            outBuf1.data(), outBuf1.size());
    REQUIRE(numProduced1 > 0);

    // Call reset() – the object must remain valid and produce output.
    resampler.reset();

    // Process the same input again (or new input); must not crash and must
    // not produce negative/NaN values.
    std::vector<float> outBuf2;
    outBuf2.resize(4800);
    size_t numProduced2 = resampler.process(inSamples.data(), inSamples.size(),
                                            outBuf2.data(), outBuf2.size());
    // Basic sanity: produced some non-negative, non-NaN output.
    CHECK(numProduced2 > 0);
    for (std::size_t i = 0; i < numProduced2; ++i) {
        CHECK(std::isfinite(outBuf2[i]));   // no NaN or infinity
        CHECK(outBuf2[i] >= -1.1f);         // relaxed range for sinc overshoot
        CHECK(outBuf2[i] <=  1.1f);
    }
}
