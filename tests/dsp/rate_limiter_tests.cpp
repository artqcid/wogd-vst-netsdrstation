// Unit tests for netsdr::RateLimiter (Milestone M2.6).
// Covers: time-based rate limiting, first-call allowance,
// "always allow" mode, and reset behaviour.

#include "catch.hpp"
#include "dsp/rate_limiter.h"

#include <cmath>

TEST_CASE("[dsp][ratelimit] allows updates no faster than the configured rate", "[dsp][ratelimit]") {
    // 20 updates/s -> one update per 0.05 s.
    netsdr::RateLimiter limiter(20.0);

    // First call at t=0.0 is always allowed.
    REQUIRE(limiter.shouldEmit(0.00) == true);
    REQUIRE(limiter.shouldEmit(0.01) == false);
    REQUIRE(limiter.shouldEmit(0.02) == false);
    REQUIRE(limiter.shouldEmit(0.03) == false);
    REQUIRE(limiter.shouldEmit(0.04) == false);
    REQUIRE(limiter.shouldEmit(0.05) == true);
    REQUIRE(limiter.shouldEmit(0.06) == false);
    REQUIRE(limiter.shouldEmit(0.07) == false);
    REQUIRE(limiter.shouldEmit(0.08) == false);
    REQUIRE(limiter.shouldEmit(0.09) == false);
    REQUIRE(limiter.shouldEmit(0.10) == true);
}

TEST_CASE("[dsp][ratelimit] first call is always allowed", "[dsp][ratelimit]") {
    netsdr::RateLimiter limiter(20.0);
    // Arbitrary large starting time should still allow the first call.
    REQUIRE(limiter.shouldEmit(12345.678) == true);
}

TEST_CASE("[dsp][ratelimit] N updates in T seconds -> at most ~20 per second", "[dsp][ratelimit]") {
    netsdr::RateLimiter limiter(20.0);
    // Simulate 10 seconds in 1 ms steps (10000 steps at t = i*0.001).
    int count = 0;
    for (int i = 0; i < 10000; ++i) {
        double t = i * 0.001;
        if (limiter.shouldEmit(t)) {
            ++count;
        }
    }
    // At 20 updates/s over 10 seconds = 200, small tolerance.
    REQUIRE(count >= 190);
    REQUIRE(count <= 210);
}

TEST_CASE("[dsp][ratelimit] zero or negative rate never limits", "[dsp][ratelimit]") {
    // RateLimiter with 0.0 should always allow.
    netsdr::RateLimiter limiterZero(0.0);
    REQUIRE(limiterZero.shouldEmit(0.0) == true);
    REQUIRE(limiterZero.shouldEmit(0.0) == true);  // same timestamp, still true

    // Negative rate should also always allow.
    netsdr::RateLimiter limiterNeg(-5.0);
    REQUIRE(limiterNeg.shouldEmit(1.0) == true);
}

TEST_CASE("[dsp][ratelimit] reset allows an immediate update", "[dsp][ratelimit]") {
    netsdr::RateLimiter limiter(20.0);

    // First update at t=0.0 allowed.
    REQUIRE(limiter.shouldEmit(0.0) == true);
    // Next update at t=0.01 is within the 0.05 s interval, suppressed.
    REQUIRE(limiter.shouldEmit(0.01) == false);

    // Reset the limiter so the next call is allowed immediately.
    limiter.reset(0.01);
    // Same timestamp as reset should now be allowed.
    REQUIRE(limiter.shouldEmit(0.01) == true);
}