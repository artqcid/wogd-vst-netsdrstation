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

    // Push distinct samples 4800..5799 so the whole buffer content is a single
    // contiguous run 0..5799 once drained.
    std::vector<float> newSamples(1000);
    for (std::size_t i = 0; i < 1000; ++i) {
        newSamples[i] = static_cast<float>(4800 + i);
    }
    buffer.push(newSamples.data(), newSamples.size());

    // Drain: the pre-fill gate latches on the first pull, so pull() keeps
    // delivering until the buffer is fully empty (0 on underflow) — the buffer
    // is NOT re-armed to the prefill target mid-stream.
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

    // All 5800 samples come out, in FIFO order (0, 1, 2, ...).
    REQUIRE(allPulled.size() == 5800);
    for (std::size_t i = 0; i < allPulled.size(); ++i) {
        REQUIRE(allPulled[i] == static_cast<float>(i));
    }
    REQUIRE(buffer.available() == 0);
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
    // The start latch engages on the first pull, so pull() delivers until the
    // buffer is empty. The oldest dropped count = 4000 - 1800 = 2200.
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

    // All 1800 retained samples come out: 2200..3999 (the newest capacity worth).
    REQUIRE(drained.size() == 1800);
    REQUIRE(drained[0] == 2200.0f);
    REQUIRE(drained.back() == 3999.0f);
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

TEST_CASE("[dsp][jitter] prefill gate latches: once started, pull drains fully",
          "[dsp][jitter]") {
    const double sampleRate = 48000.0;
    const double targetMs = 100.0;
    const double maxMs = 500.0;

    netsdr::JitterBuffer buffer(sampleRate, targetMs, maxMs);

    // Prefill to ready.
    std::vector<float> prefill(5760);  // 120 ms at 48 kHz
    for (std::size_t i = 0; i < 5760; ++i) {
        prefill[i] = static_cast<float>(i);
    }
    buffer.push(prefill.data(), prefill.size());

    REQUIRE(buffer.isReady() == true);

    // Gate before start: with only a sub-target amount buffered and no pull yet,
    // pull returns 0. Simulate a fresh buffer below the target prefill.
    netsdr::JitterBuffer cold(sampleRate, targetMs, maxMs);
    std::vector<float> few(100);  // ~2 ms << 100 ms target
    for (std::size_t i = 0; i < 100; ++i) {
        few[i] = static_cast<float>(i);
    }
    cold.push(few.data(), few.size());
    float cbuf[128];
    REQUIRE(cold.pull(cbuf, 128) == 0);  // not started, below target -> gated

    // Once started, pull keeps delivering until the buffer is empty (underflow
    // returns 0), even far below the 100 ms prefill target. No re-arm gaps.
    std::size_t total = 0;
    float chunk[128];
    std::size_t n;
    while ((n = buffer.pull(chunk, 128)) != 0) {
        total += n;
        (void)chunk;  // avoid unused warning
    }

    // Full drain (5760 samples in FIFO order), no cushion retained mid-stream.
    REQUIRE(total == 5760);
    REQUIRE(buffer.available() == 0);
    REQUIRE(buffer.isReady() == false);

    // reset() re-arms the start latch: a fresh push below target is gated again.
    buffer.reset();
    buffer.push(few.data(), few.size());
    REQUIRE(buffer.pull(cbuf, 128) == 0);
}