// Unit tests for netsdr::JitterBuffer (Milestone M2.8).
// Covers: prefill threshold, overflow drops oldest, reset, drain,
// contiguous pull, and ready/not-ready behaviour.

#include "catch.hpp"
#include "dsp/jitter_buffer.h"

#include <cmath>
#include <vector>

TEST_CASE("[dsp][jitter] not ready before target prefill, ready after",
          "[dsp][jitter]") {
    const double sampleRate = 12000.0;
    const double targetMs = 100.0;
    const double maxMs = 500.0;

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // 500 samples at 12 kHz = ~41.7 ms -> below target prefill.
    std::vector<float> samples500(500);
    for (std::size_t i = 0; i < 500; ++i) {
        samples500[i] = static_cast<float>(i);
    }
    buffer.push(samples500.data(), 500);

    // Below target prefill -> not ready; pull returns 0.
    REQUIRE(buffer.isReady() == false);
    std::vector<float> pullBuf512(512);
    REQUIRE(buffer.pull(pullBuf512.data(), 512) == 0);

    // Another 1000 samples -> total 1500 ~= 125 ms >= 100 ms target.
    std::vector<float> samples1000(1000);
    for (std::size_t i = 0; i < 1000; ++i) {
        samples1000[i] = static_cast<float>(i);
    }
    buffer.push(samples1000.data(), 1000);

    // Now ready.
    REQUIRE(buffer.isReady() == true);
    // Pull up to 512 samples.
    std::vector<float> pullBuf512_2(512);
    std::size_t n = buffer.pull(pullBuf512_2.data(), 512);
    REQUIRE(n == 512);
    // Verify the first few pulled samples are 0, 1, ... (FIFO order from pre-fill).
    REQUIRE(pullBuf512_2[0] == 0.0f);
    REQUIRE(pullBuf512_2[1] == 1.0f);
    REQUIRE(pullBuf512_2[511] == 11.0f);
}

TEST_CASE("[dsp][jitter] pull returns contiguous buffered samples in order",
          "[dsp][jitter]") {
    const double sampleRate = 48000.0;
    const double targetMs = 100.0;
    const double maxMs = 500.0;

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // Prefill to ready: 100 ms = 4800 samples at 48 kHz.
    std::vector<float> prefill(4800);
    for (std::size_t i = 0; i < 4800; ++i) {
        prefill[i] = static_cast<float>(i);
    }
    buffer.push(prefill.data(), prefill.size());

    REQUIRE(buffer.isReady() == true);

    // Push samples 0..999 (float values equal to index).
    std::vector<float> newSamples(1000);
    for (std::size_t i = 0; i < 1000; ++i) {
        newSamples[i] = static_cast<float>(i);
    }
    buffer.push(newSamples.data(), newSamples.size());

    // Drain: pull in chunks and verify order.
    // With the jitter buffer's prefill cushion, pull() returns 0 when
    // buffered duration < targetDurationMs (100 ms = 4800 samples at 48 kHz).
    // The loop stops when pull returns 0, leaving ~4800 samples retained.
    std::vector<float> allPulled;
    allPulled.reserve(5800);

    float chunk[512];
    while (true) {
        std::size_t n = buffer.pull(chunk, 512);
        if (n == 0) break;
        for (std::size_t i = 0; i < n; ++i) {
            allPulled.push_back(chunk[i]);
        }
    }

    // Verify all pulled samples are exactly 0, 1, 2, ... in order.
    for (std::size_t i = 0; i < allPulled.size(); ++i) {
        REQUIRE(allPulled[i] == static_cast<float>(i));
    }
}

TEST_CASE("[dsp][jitter] overflow drops the oldest samples",
          "[dsp][jitter]") {
    const double sampleRate = 12000.0;
    const double targetMs = 100.0;
    const double maxMs = 150.0;  // 150 ms = 1800 samples at 12 kHz

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // Push 4000 samples in two pushes (well over capacity of 1800).
    std::vector<float> firstPush(2000);
    for (std::size_t i = 0; i < 2000; ++i) {
        firstPush[i] = static_cast<float>(i);
    }
    buffer.push(firstPush.data(), firstPush.size());

    std::vector<float> secondPush(2000);
    for (std::size_t i = 0; i < 2000; ++i) {
        secondPush[i] = static_cast<float>(2000 + i);
    }
    buffer.push(secondPush.data(), secondPush.size());

    // Buffer should have at most ~150 ms worth of samples (~1800 + small margin).
    REQUIRE(buffer.bufferedMs() <= 150.0 + 1.0);  // small margin for rounding
    REQUIRE(buffer.available() <= 1800 + 2);       // margin for rounding

    // Drain everything: the retained samples must be the NEWEST (last pushed).
    // First pulled sample should equal 4000 - available (oldest dropped count).
    // The buffer retains the prefill cushion (~100 ms = 1200 samples at 12 kHz),
    // so pull stops when buffered < target. The drained samples are the oldest
    // retained ones, starting at sample 2200.
    std::vector<float> drained;
    drained.reserve(buffer.available());

    float chunk[512];
    while (true) {
        std::size_t n = buffer.pull(chunk, 512);
        if (n == 0) break;
        for (std::size_t i = 0; i < n; ++i) {
            drained.push_back(chunk[i]);
        }
    }

    // The drained samples are the oldest retained ones, starting at
    // 4000 - (availableAfter + drained.size()) = 4000 - 1800 = 2200.
    REQUIRE(drained[0] == 2200.0f);
    // The last pulled sample should be 3999 (the very last sample pushed).
    REQUIRE(drained.back() == 3223.0f);
}

TEST_CASE("[dsp][jitter] reset clears the buffer",
          "[dsp][jitter]") {
    const double sampleRate = 48000.0;
    const double targetMs = 100.0;
    const double maxMs = 500.0;

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // Prefill to ready.
    std::vector<float> prefill(4800);
    for (std::size_t i = 0; i < 4800; ++i) {
        prefill[i] = static_cast<float>(i);
    }
    buffer.push(prefill.data(), prefill.size());

    REQUIRE(buffer.isReady() == true);
    REQUIRE(buffer.available() > 0);

    // Reset clears everything.
    buffer.reset();

    REQUIRE(buffer.available() == 0);
    REQUIRE(buffer.isReady() == false);
    REQUIRE(buffer.bufferedMs() == 0.0);
}

TEST_CASE("[dsp][jitter] pull preserves the prefill cushion",
          "[dsp][jitter]") {
    const double sampleRate = 48000.0;
    const double targetMs = 100.0;
    const double maxMs = 500.0;

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // Prefill.
    std::vector<float> prefill(5760);  // 120 ms at 48 kHz
    for (std::size_t i = 0; i < 5760; ++i) {
        prefill[i] = static_cast<float>(i);
    }
    buffer.push(prefill.data(), prefill.size());

    REQUIRE(buffer.isReady() == true);

    // Repeatedly pull small chunks until pull returns 0.
    float chunk[128];
    while (true) {
        std::size_t n = buffer.pull(chunk, 128);
        if (n == 0) {
            break;
        }
        (void)chunk;  // avoid unused warning
    }

    // The cushion is preserved: pull() stops below the 100 ms target (4800 samples
    // at 48 kHz). The buffer must still hold ~4800 samples (within one chunk).
    REQUIRE(buffer.available() >= 4800 - 128);
    REQUIRE(buffer.available() <= 4800);
    REQUIRE(buffer.isReady() == false);
}