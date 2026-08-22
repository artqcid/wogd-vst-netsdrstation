// Unit tests for the AudioSampleQueue (Milestone M2.4).
// Stress test: producer pushes N blocks, consumer reads N; order preserved,
// no loss, no corruption (checksum + deterministic sample pattern per block).

#include "catch.hpp"
#include "threading/audio_sample_queue.h"

#include <array>
#include <cstdint>
#include <thread>
#include <vector>



TEST_CASE("threading[spsc][audio] audio queue: order, no loss, no corruption under stress",
          "[threading][spsc][audio]")
{
    constexpr std::size_t kCount = 50000;
    constexpr std::size_t kCapacity = 1024;
    netsdr::AudioSampleQueue queue(kCapacity);

    std::atomic<bool> producerDone{false};
    std::vector<netsdr::AudioSampleBlock> received;
    received.reserve(kCount);

    std::thread producer([&] {
        for (std::uint32_t i = 0; i < kCount; ++i) {
            netsdr::AudioSampleBlock block;
            block.sequence = i;
            block.sampleCount = 8;
            // Deterministic checksum in samples[0]
            block.samples[0] = static_cast<int16_t>(i * 2654435761u & 0xFFFF);
            // Fill the remaining 7 samples with a known pattern.
            for (std::size_t j = 1; j < 8; ++j) {
                block.samples[j] = static_cast<int16_t>((i * 73 + j) & 0xFFFF);
            }
            queue.push(std::move(block));
        }
        producerDone = true;
    });

    // Consumer: read until we have all blocks and the producer is done.
    std::size_t expectedIndex = 0;
    netsdr::AudioSampleBlock b;
    while (expectedIndex < kCount) {
        while (queue.pop(b)) {
            // Strict order, no loss.
            REQUIRE(b.sequence == expectedIndex);
            // Checksum intact in samples[0].
            REQUIRE(b.samples[0] == static_cast<int16_t>(expectedIndex * 2654435761u & 0xFFFF));
            // Pattern intact in remaining samples.
            for (std::size_t j = 1; j < 8; ++j) {
                REQUIRE(b.samples[j] == static_cast<int16_t>(expectedIndex * 73 + j & 0xFFFF));
            }
            received.push_back(b);
            ++expectedIndex;
        }
        // Small backoff to let the producer make progress.
        if (!producerDone.load()) {
            std::this_thread::yield();
        }
    }
    producer.join();

    REQUIRE(received.size() == kCount);
}

TEST_CASE("threading[spsc][audio] audio queue: pop on empty returns false",
          "[threading][spsc][audio]")
{
    netsdr::AudioSampleQueue queue(16);
    netsdr::AudioSampleBlock out;
    REQUIRE_FALSE(queue.pop(out));
}

TEST_CASE("threading[spsc][audio] audio queue: push/pop single block roundtrip",
          "[threading][spsc][audio]")
{
    netsdr::AudioSampleQueue queue(16);
    netsdr::AudioSampleBlock block;
    block.sequence = 42;
    block.sampleCount = 8;
    block.samples[0] = static_cast<int16_t>(42 * 2654435761u & 0xFFFF);
    for (std::size_t j = 1; j < 8; ++j) {
        block.samples[j] = static_cast<int16_t>(42 * 73 + j & 0xFFFF);
    }

    queue.push(std::move(block));
    netsdr::AudioSampleBlock out;
    REQUIRE(queue.pop(out));

    // sampleCount and sequence survive the roundtrip.
    REQUIRE(out.sampleCount == 8);
    REQUIRE(out.sequence == 42);
    // checksum in samples[0] survives.
    REQUIRE(out.samples[0] == static_cast<int16_t>(42 * 2654435761u & 0xFFFF));
    // a couple of sample values survive the roundtrip.
    REQUIRE(out.samples[1] == static_cast<int16_t>(42 * 73 + 1 & 0xFFFF));
    REQUIRE(out.samples[2] == static_cast<int16_t>(42 * 73 + 2 & 0xFFFF));
}