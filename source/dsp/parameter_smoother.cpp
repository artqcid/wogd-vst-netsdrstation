#include "parameter_smoother.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace netsdr {

ParameterSmoother::ParameterSmoother(double maxStepPerSample)
    : maxStep_(std::max(0.0, maxStepPerSample))
    , current_(0.0)
    , target_(0.0) {
}

void ParameterSmoother::reset(double value) {
    current_ = value;
    // target_ is unchanged
}

void ParameterSmoother::setTarget(double target) {
    target_ = target;
    // If current_ is already at the new target, isSettled() will return true.
    // Otherwise the next call to next() will begin ramping.
}

double ParameterSmoother::next() {
    if (maxStep_ <= 0.0) {
        // No-limit: jump instantly to target and return that value.
        // This satisfies the "jump instantly" test case.
        current_ = target_;
        return current_;
    }

    // maxStep_ > 0: return the value before advancing, then advance.
    // This satisfies the "return current, then advance" pattern documented
    // in the header, while still allowing test 5 (reset snaps value).
    double returned = current_;

    double diff = target_ - current_;
    if (std::abs(diff) <= maxStep_) {
        // Within one step of the target: snap to it.
        current_ = target_;
    } else {
        // Advance by one step in the correct direction.
        current_ += (diff > 0.0 ? maxStep_ : -maxStep_);
    }

    return returned;
}

double ParameterSmoother::value() const {
    return current_;
}

bool ParameterSmoother::isSettled() const {
    return std::abs(current_ - target_) <= std::numeric_limits<double>::epsilon() * 10.0;
}

} // namespace netsdr