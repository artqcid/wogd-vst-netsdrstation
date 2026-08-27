#include "catch.hpp"
#include "dsp/ima_adpcm.h"
#include "../data/kiwi_stream_fixture.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace netsdr;
using namespace netsdr_test;

// ---------------------------------------------------------------------------
// KiwiSDR real stream: plugin decode matches reference PCM
// ---------------------------------------------------------------------------
TEST_CASE("KiwiSDR real stream: plugin decode matches reference PCM",
          "[network][kiwi][dsp][regression]") {
    // Single stateful ImaAdpcmDecoder reused across all 54 frames (matches
    // production behavior and how the fixture was generated).
    ImaAdpcmDecoder decoder;
    std::vector<int16_t> decoded;

    // Each frame has a known length from kKiwiFixtureFrameLengths (54 entries,
    // each = 1034 bytes: 10-byte header + 1024 ADPCM payload).  Use the length
    // array for per-frame byte lengths and compute cumulative byte offsets into
    // the concatenated frame buffer.
    std::size_t offset = 0;

    for (std::size_t frameIdx = 0;
         frameIdx < kKiwiFixtureFrameCount; ++frameIdx) {
        // Frame length for this index.
        const std::size_t frameLen = kKiwiFixtureFrameLengths[frameIdx];

        // Frame starts at `offset` into the concatenated frame buffer.
        const std::uint8_t* frame = &kKiwiFixtureFrames[offset];

        // --- Plugin decode path checks ---
        // Verify SND magic bytes.
        REQUIRE(frame[0] == 'S');
        REQUIRE(frame[1] == 'N');
        REQUIRE(frame[2] == 'D');

        // Verify flags byte: compressed flag 0x10 set, voice flag 0x08 clear.
        const std::uint8_t flags = frame[3];
        INFO("Frame " << frameIdx << ": flags=0x" << std::hex << (int)flags
             << std::dec);
        REQUIRE((flags & 0x10) != 0);
        REQUIRE((flags & 0x08) == 0);

        // ADPCM payload starts at byte offset 10 (not 20).
        const std::size_t payloadOffset = 10;
        const std::size_t payloadBytes = frameLen - payloadOffset; // 1024
        const std::uint8_t* adpcmPayload = frame + payloadOffset;

        // Decode the ADPCM payload into 2 * payloadBytes int16 samples.
        // The decoder state carries across frames — do NOT call reset().
        std::vector<int16_t> frameSamples(2 * payloadBytes);
        decoder.decode(adpcmPayload, payloadBytes, frameSamples.data());
        decoded.insert(decoded.end(), frameSamples.begin(), frameSamples.end());

        offset += frameLen;
    }

    // Total decoded sample count must match the reference fixture count.
    INFO("Decoded " << decoded.size() << " samples, expected "
                    << kKiwiFixtureSampleCount);
    REQUIRE(decoded.size() == kKiwiFixtureSampleCount);

    // Sample-by-sample comparison against the reference fixture.
    // They must be bit-identical since both use the same IMA ADPCM algorithm.
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        if (decoded[i] != kKiwiFixtureSamples[i]) {
            INFO("first mismatch at index " << i << ": got=" << decoded[i]
                                            << " expected=" << kKiwiFixtureSamples[i]);
            REQUIRE(decoded[i] == kKiwiFixtureSamples[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// KiwiSDR real stream: header is 10 bytes not 20
// ---------------------------------------------------------------------------
TEST_CASE("KiwiSDR real stream: header is 10 bytes not 20",
          "[network][kiwi][dsp][regression]") {
    // Decode ONLY the first frame's payload at offset 10 (1024 bytes)
    // and assert it produces exactly 2048 samples matching the first 2048
    // entries of kKiwiFixtureSamples.

    // First frame starts at offset 0; payload is at byte 10, length 1024.
    constexpr std::size_t kFirstFramePayloadBytes = 1024;
    constexpr std::size_t kExpectedSampleCount = 2048; // 1024 * 2

    const std::uint8_t* firstFrame = &kKiwiFixtureFrames[0];
    const std::uint8_t* payload = firstFrame + 10; // offset 10

    // Single stateful decoder; NOT reset so it carries from any prior usage.
    ImaAdpcmDecoder decoder;
    std::vector<int16_t> decoded(kExpectedSampleCount);

    decoder.decode(payload, kFirstFramePayloadBytes, decoded.data());

    // Exactly 2048 samples from 1024 ADPCM bytes.
    REQUIRE(static_cast<std::size_t>(decoded.size()) == kExpectedSampleCount);

    // First 2048 entries must match the reference fixture samples (bit-identical).
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        if (decoded[i] != kKiwiFixtureSamples[i]) {
            INFO("index " << i << ": got=" << decoded[i]
                          << " expected=" << kKiwiFixtureSamples[i]);
            REQUIRE(decoded[i] == kKiwiFixtureSamples[i]);
        }
    }
}