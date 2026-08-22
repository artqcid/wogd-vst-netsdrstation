#pragma once
// RateLimiter: limits the rate at which parameter updates may be emitted.
// Prevents spamming the KiwiSDR server with SET freq=... commands faster
// than the server can handle (~20 updates per second).
//
// Implementation: time-based interval check. The caller supplies the current
// monotonic time in seconds (nowSeconds) so tests are deterministic with no
// real clock or sleep dependency.
//
// maxUpdatesPerSecond <= 0 means "always allow" (never rate-limited).
namespace netsdr {

class RateLimiter {
public:
    // Constructor. maxUpdatesPerSecond > 0 sets the maximum update rate.
    // maxUpdatesPerSecond <= 0 means the limiter is never active (always allow).
    explicit RateLimiter(double maxUpdatesPerSecond);

    // Return true when an update is allowed now. When true, record nowSeconds
    // as the new last-emit time. An update is allowed if this is the first call,
    // or if maxUpdatesPerSecond <= 0, or if nowSeconds - lastEmitTime >=
    // 1.0 / maxUpdatesPerSecond.
    bool shouldEmit(double nowSeconds);

    // Reset the last-emit time so the next call to shouldEmit() is allowed
    // immediately (even if the same timestamp is used).
    void reset(double nowSeconds);

private:
    double maxUpdatesPerSecond_ = 0.0;
    double lastEmitTime_ = 0.0;
    bool firstCall_ = true;
};

} // namespace netsdr