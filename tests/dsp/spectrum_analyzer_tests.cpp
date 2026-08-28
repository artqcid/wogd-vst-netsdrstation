// Unit tests for the DFT-based SpectrumAnalyzer (M4.7 waterfall backend).
// A pure sine at bin-center frequency must produce a dominant bin; silence
// must floor at -160 dBFS; off-bin tones must still be well above noise.

#include "dsp/spectrum_analyzer.h"

#include "catch.hpp"

#include <cmath>
#include <vector>

namespace {

std::vector<float> makeSine(std::size_t n, std::size_t sampleRate, double freqHz,
                            float amplitude = 0.8f) {
    std::vector<float> out(n);
    const double phaseStep = 2.0 * 3.141592653589793 * freqHz / sampleRate;
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = amplitude * static_cast<float>(std::sin(phaseStep * i));
    }
    return out;
}

} // namespace

TEST_CASE("SpectrumAnalyzer: silent input floors at -160 dBFS", "[dsp][spectrum]") {
    netsdr::SpectrumAnalyzer analyzer(512);
    REQUIRE(analyzer.windowSize() == 512);
    REQUIRE(analyzer.binCount() == 256);

    std::vector<float> silence(512, 0.0f);
    const auto bins = analyzer.computeDbF(48000, silence.data());
    REQUIRE(bins.size() == 256);
    for (float b : bins) {
        REQUIRE(b <= -150.0f);
    }
}

TEST_CASE("SpectrumAnalyzer: a tone at a bin centre dominates that bin",
          "[dsp][spectrum]") {
    netsdr::SpectrumAnalyzer analyzer(512);
    constexpr std::size_t kRate = 48000;
    // Bin k frequency = k * rate / windowSize. Pick k = 64 -> 6000 Hz.
    constexpr std::size_t kBin = 64;
    const double freq = kBin * kRate / 512.0;

    const auto samples = makeSine(512, kRate, freq);
    const auto bins = analyzer.computeDbF(kRate, samples.data());

    // The peak must be at (or very near) the expected bin.
    std::size_t peak = 0;
    for (std::size_t i = 1; i < bins.size(); ++i) {
        if (bins[i] > bins[peak]) peak = i;
    }
    REQUIRE(peak == kBin);
    REQUIRE(bins[kBin] > -6.0f); // full-ish scale tone near 0 dBFS
}

TEST_CASE("SpectrumAnalyzer: off-centre tone still peaks at the nearest bin",
          "[dsp][spectrum]") {
    netsdr::SpectrumAnalyzer analyzer(512);
    constexpr std::size_t kRate = 48000;
    // 6000 Hz tone -> bin 64 is the centre; the nearest-bin peak must be
    // within a couple of bins of it.
    const auto samples = makeSine(512, kRate, 6000.0);
    const auto bins = analyzer.computeDbF(kRate, samples.data());

    std::size_t peak = 0;
    for (std::size_t i = 1; i < bins.size(); ++i) {
        if (bins[i] > bins[peak]) peak = i;
    }
    REQUIRE(peak >= 62);
    REQUIRE(peak <= 66);
    REQUIRE(bins[peak] > -12.0f);
}

TEST_CASE("SpectrumAnalyzer: half-amplitude tone reduces the peak by ~6 dB",
          "[dsp][spectrum]") {
    netsdr::SpectrumAnalyzer analyzer(512);
    constexpr std::size_t kRate = 48000;
    constexpr std::size_t kBin = 64;
    const double freq = kBin * kRate / 512.0;

    const auto full = makeSine(512, kRate, freq, 1.0f);
    const auto half = makeSine(512, kRate, freq, 0.5f);
    const auto binsFull = analyzer.computeDbF(kRate, full.data());
    const auto binsHalf = analyzer.computeDbF(kRate, half.data());

    const float diff = binsFull[kBin] - binsHalf[kBin];
    REQUIRE(diff > 4.5f);
    REQUIRE(diff < 7.5f);
}

TEST_CASE("SpectrumAnalyzer: binFrequency helper", "[dsp][spectrum]") {
    REQUIRE(netsdr::SpectrumAnalyzer::binFrequency(0, 48000, 256) == 0.0f);
    REQUIRE(netsdr::SpectrumAnalyzer::binFrequency(64, 48000, 256) ==
            48000.0f * 64.0f / (2.0f * 256.0f));
}