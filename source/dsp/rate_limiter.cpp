#include "dsp/rate_limiter.h"

#include <algorithm>

namespace netsdr {

RateLimiter::RateLimiter(double maxUpdatesPerSecond)
    : maxUpdatesPerSecond_(std::max(0.0, maxUpdatesPerSecond))
    , lastEmitTime_(0.0)
    , firstCall_(true) {
}

bool RateLimiter::shouldEmit(double nowSeconds) {
    // "always allow" mode: never rate-limited.
    if (maxUpdatesPerSecond_ <= 0.0) {
        lastEmitTime_ = nowSeconds;
        return true;
    }

    // First call is always allowed, regardless of the timestamp.
    if (firstCall_) {
        firstCall_ = false;
        lastEmitTime_ = nowSeconds;
        return true;
    }

    double interval = 1.0 / maxUpdatesPerSecond_;
    if (nowSeconds - lastEmitTime_ >= interval) {
        lastEmitTime_ = nowSeconds;
        return true;
    }
    return false;
}

void RateLimiter::reset(double nowSeconds) {
    if (maxUpdatesPerSecond_ <= 0.0) {
        lastEmitTime_ = nowSeconds;
        return;
    }
    // Set lastEmitTime so that the next shouldEmit(nowSeconds) returns true:
    //   nowSeconds - lastEmitTime_ >= 1.0 / maxUpdatesPerSecond_
    // => lastEmitTime_ <= nowSeconds - 1.0 / maxUpdatesPerSecond_
    // We set it exactly to that boundary so the next call is allowed.
    lastEmitTime_ = nowSeconds - (1.0 / maxUpdatesPerSecond_);
}

} // namespace netsdr