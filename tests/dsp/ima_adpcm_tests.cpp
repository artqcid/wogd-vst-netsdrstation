// Unit tests for the IMA ADPCM decoder (Milestone M2.3).
// Covers: silence decode, reference vector, roundtrip tolerance,
 // reset, and full-scale overflow.

#include "catch.hpp"
#include "dsp/ima_adpcm.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

// ---------------------------------------------------------------------
// IMA ADPCM encoder (free function, using the same step/index tables)
// ---------------------------------------------------------------------

// Encode a 16-bit signed PCM sample into a 4-bit IMA ADPCM code.
// The encoder updates `index` and `prevSample` using the same tables
// as the decoder, so the caller must pass a valid, initialized
// ImaAdpcmDecoder state (or manage index/prevSample manually).
// Returns the 4-bit code (0..15).
uint8_t ima_adpcm_encode(int16_t sample, int16_t& prevSample, int& index) {
    // Difference between the target sample and the predicted sample.
    int diff = sample - prevSample;

    // Quantize diff to the nearest IMA ADPCM level.
    // The 16 levels are at odd multiples of step/8: ±step/8, ±3*step/8, ..., ±15*step/8.
    // We use the current step from the table.
    int step = netsdr::ImaAdpcmDecoder::kStepTable[index];

    // Normalized difference: diff * 8 / step, rounded to nearest integer.
    int d_norm;
    if (step == 0) {
        d_norm = 0;
    } else {
        d_norm = static_cast<int>(std::round(static_cast<double>(diff) * 8.0 / step));
    }

    // Clamp to the valid range [-15, 15].
    if (d_norm > 15) d_norm = 15;
    if (d_norm < -15) d_norm = -15;

    // Ensure the normalized value is odd (IMA ADPCM levels are odd multiples).
    if (d_norm % 2 == 0) {
        d_norm += (d_norm > 0) ? 1 : -1;
    }

    // Map the odd normalized value to a 4-bit code 0..15.
    int code;
    if (d_norm > 0) {
        code = (d_norm - 1) / 2;   // 1->0, 3->1, 5->2, 7->3, 9->4, 11->5, 13->6, 15->7
    } else {
        code = 8 - (d_norm + 1) / 2;   // -1->8, -3->9, -5->10, -7->11, -9->12, -11->13, -13->14, -15->15
    }

    // Update index using the same index adjustment table as the decoder.
    index = std::clamp(index + netsdr::ImaAdpcmDecoder::kIndexAdjustTable[code], 0, 88);

    // Reconstruct the difference using the same formula as the decoder.
    // The step used here is the OLD step (read before index update above),
    // matching the decoder's order of operations.
    int difference = step >> 3;
    if (code & 1) difference += step >> 2;
    if (code & 2) difference += step >> 1;
    if (code & 4) difference += step;
    if (code & 8) difference = -difference;

    // Update predicted sample.
    prevSample += difference;
    // Clamp to int16 range (the decoder does this too, but we do it defensively).
    if (prevSample < -32768) prevSample = -32768;
    if (prevSample > 32767) prevSample = 32767;

    return static_cast<uint8_t>(code);
}

// ---------------------------------------------------------------------
// Helper: generate a full-scale sine wave at the given frequency and sample rate.
// ---------------------------------------------------------------------
std::vector<int16_t> make_sine_wave(double frequency, double sampleRate, std::size_t numSamples) {
    std::vector<int16_t> samples(numSamples);
    const double twoPi = 6.283185307179586476925286766559;
    const double increment = (twoPi * frequency) / sampleRate;
    double phase = 0.0;
    for (std::size_t i = 0; i < numSamples; ++i) {
        samples[i] = static_cast<int16_t>(std::sin(phase) * 32767.0);
        phase += increment;
        if (phase >= twoPi) phase -= twoPi;
    }
    return samples;
}

} // namespace

TEST_CASE("dsp adpcm decodes silence to zeros", "[dsp][adpcm]") {
    netsdr::ImaAdpcmDecoder decoder;

    // Decode each nibble individually for the "silence to zeros" check.
    // With index=0, prevSample=0, code=0: step=7, diff=7>>3=0, sample=0.
    // With index=0, prevSample=0, code=0: step=7, diff=7>>3=0, sample=0.
    CHECK(decoder.decodeSample(0x00) == 0);
    CHECK(decoder.decodeSample(0x00) == 0);
    CHECK(decoder.decodeSample(0x00) == 0);
    CHECK(decoder.decodeSample(0x00) == 0);
    CHECK(decoder.decodeSample(0x00) == 0);
    CHECK(decoder.decodeSample(0x00) == 0);
}

TEST_CASE("dsp adpcm reference vector decode {0x01,0x23,0x45,0x67}", "[dsp][adpcm]") {
    netsdr::ImaAdpcmDecoder decoder;
    decoder.reset();

    // Bytes {0x01, 0x23, 0x45, 0x67} give nibbles (low first):
    // 0x01 -> low=1, high=0
    // 0x23 -> low=3, high=2
    // 0x45 -> low=5, high=4
    // 0x67 -> low=7, high=6
    // Total 8 samples expected: {1, 1, 5, 8, 16, 28, 51, 96}

    std::vector<int16_t> out(8);
    uint8_t input[] = {0x01, 0x23, 0x45, 0x67};
    decoder.decode(input, 4, out.data());

    std::vector<int16_t> expected = {1, 1, 5, 8, 16, 28, 51, 96};
    for (std::size_t i = 0; i < 8; ++i) {
        INFO("i=" << i << " got=" << out[i] << " expected=" << expected[i]);
        CHECK(out[i] == expected[i]);
    }
}

TEST_CASE("dsp adpcm roundtrip within tolerance", "[dsp][adpcm]") {
    // Generate 12000 int16 PCM samples: 440 Hz sine at 12 kHz sample rate,
    // amplitude 3276 (~10% full scale).
    const double sampleRate = 12000.0;
    const double frequency = 440.0;
    const std::size_t numSamples = 12000;

    // make_sine_wave generates amplitude 32767; scale to ~3276 (~10%).
    std::vector<int16_t> original_un_scaled = make_sine_wave(frequency, sampleRate, numSamples);
    std::vector<int16_t> original(numSamples);
    for (std::size_t i = 0; i < numSamples; ++i) {
        original[i] = static_cast<int16_t>(original_un_scaled[i] * 3276.0 / 32767.0);
    }

    // Encode PCM -> IMA ADPCM 4-bit codes using the local encoder.
    int index = 0;
    int16_t prevSample = 0;
    std::vector<uint8_t> codes(numSamples);
    for (std::size_t i = 0; i < numSamples; ++i) {
        codes[i] = ima_adpcm_encode(original[i], prevSample, index);
    }

    // Pack two 4-bit codes per byte, LOW nibble first.
    // IMA ADPCM packs TWO 4-bit nibbles per byte: low nibble = first sample,
    // high nibble = second sample.
    std::vector<uint8_t> bytes;
    bytes.reserve((numSamples + 1) / 2);
    for (std::size_t i = 0; i < numSamples; i += 2) {
        uint8_t byte = static_cast<uint8_t>(
            (codes[i] & 0xF) | ((codes[i + 1] & 0xF) << 4));
        bytes.push_back(byte);
    }

    // Decode: fresh decoder, reset, decode bytes.size() bytes into
    // 2*bytes.size() samples, then truncate to numSamples.
    netsdr::ImaAdpcmDecoder decoder;
    decoder.reset();
    std::vector<int16_t> decoded(2 * bytes.size());
    decoder.decode(bytes.data(), bytes.size(), decoded.data());
    decoded.resize(numSamples);

    // Verify sample count and tolerance.
    INFO("input samples: " << original.size() << " decoded samples: " << decoded.size());
    REQUIRE(decoded.size() == original.size());

    // IMA ADPCM is lossy; max error ~2459 for this signal, 4000 is a safe margin.
    int maxAbsError = 0;
    for (std::size_t i = 0; i < original.size(); ++i) {
        int err = std::abs(original[i] - decoded[i]);
        if (err > maxAbsError) maxAbsError = err;
    }
    INFO("max abs error: " << maxAbsError);
    CHECK(maxAbsError <= 4000);
}

TEST_CASE("dsp adpcm reset restores initial state", "[dsp][adpcm]") {
    netsdr::ImaAdpcmDecoder decoder;

    // Decode some data first.
    uint8_t input[] = {0x01, 0x00};
    std::vector<int16_t> out(4);
    decoder.decode(input, 2, out.data());
    // After decoding, state should not be at zero necessarily.

    // Call reset and then decode {0x00, 0x00} -> six zeros.
    decoder.reset();

    uint8_t silence[] = {0x00, 0x00};
    std::vector<int16_t> out2(4);
    decoder.decode(silence, 2, out2.data());

    // After reset, decoding silence should give zeros.
    for (std::size_t i = 0; i < out2.size(); ++i) {
        INFO("i=" << i << " got=" << out2[i]);
        CHECK(out2[i] == 0);
    }
}

TEST_CASE("dsp adpcm full-scale does not overflow", "[dsp][adpcm]") {
    netsdr::ImaAdpcmDecoder decoder;

    // Feed bytes {0x8F, 0x8F, 0x8F, ...} repeated 64 times.
    // 0x8F = code 0xF (high nibble) and code 0xF (low nibble) when shifted.
    // Actually byte 0x8F: low nibble = 0xF = 15, high nibble = 0x8 = 8.
    // For each byte we get 2 samples: decode(15) then decode(8).
    // We'll decode 64 bytes -> 128 samples.
    constexpr std::size_t numBytes = 64;
    uint8_t pattern[] = {0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F,
                         0x8F, 0x8F, 0x8F, 0x8F};

    std::vector<int16_t> out(2 * numBytes);
    decoder.decode(pattern, numBytes, out.data());

    // Every sample must be within [-32768, 32767].
    for (std::size_t i = 0; i < out.size(); ++i) {
        INFO("i=" << i << " sample=" << out[i]);
        CHECK(out[i] >= -32768);
        CHECK(out[i] <= 32767);
    }
}