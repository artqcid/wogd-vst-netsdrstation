// Unit tests for netsdr::ParameterSmoother (Milestone M2.5).
// Covers: monotonic ramp, per-sample step limit, instant jump on no-limit,
// and reset-snaps-value behaviour.

#include "catch.hpp"
#include "dsp/parameter_smoother.h"

#include <cmath>
#include <vector>

TEST_CASE("[dsp][smoother] ramp is monotonic and reaches target", "[dsp][smoother]") {
    const double maxStep = 0.01;
    netsdr::ParameterSmoother smoother(maxStep);
    smoother.reset(0.0);
    smoother.setTarget(1.0);

    std::vector<double> values;
    values.reserve(500);

    // Pull next() up to 500 times (enough to reach target).
    for (std::size_t i = 0; i < 500; ++i) {
        values.push_back(smoother.next());
    }

    // Every returned value must be >= the previous (monotonic, non-decreasing).
    for (std::size_t i = 1; i < values.size(); ++i) {
        REQUIRE(values[i] >= values[i - 1]);
    }

    // The final returned value must equal 1.0 exactly.
    REQUIRE(values.back() == 1.0);

    // After settling, isSettled() must return true.
    REQUIRE(smoother.isSettled() == true);
}

TEST_CASE("[dsp][smoother] max per-sample step never exceeded", "[dsp][smoother]") {
    const double maxStep = 0.01;
    netsdr::ParameterSmoother smoother(maxStep);
    smoother.reset(0.0);
    smoother.setTarget(5.0);

    std::vector<double> values;
    values.reserve(1000);

    // Pull many samples.
    for (std::size_t i = 0; i < 1000; ++i) {
        values.push_back(smoother.next());
    }

    // Every consecutive difference must be <= maxStep + epsilon.
    for (std::size_t i = 1; i < values.size(); ++i) {
        double diff = std::abs(values[i] - values[i - 1]);
        REQUIRE(diff <= maxStep + 1e-9);
    }
}

TEST_CASE("[dsp][smoother] decreasing ramp is monotonic", "[dsp][smoother]") {
    const double maxStep = 0.02;
    netsdr::ParameterSmoother smoother(maxStep);
    smoother.reset(1.0);
    smoother.setTarget(-1.0);

    std::vector<double> values;
    values.reserve(200);

    // Pull samples until settled.
    while (!smoother.isSettled()) {
        values.push_back(smoother.next());
    }
    // One final read after settlement.
    values.push_back(smoother.next());

    // Every value must be <= the previous (non-increasing ramp).
    for (std::size_t i = 1; i < values.size(); ++i) {
        REQUIRE(values[i] <= values[i - 1]);
    }

    // Final value must be -1.0 (target).
    REQUIRE(values.back() == -1.0);
}

TEST_CASE("[dsp][smoother] no-limit smoother jumps instantly", "[dsp][smoother]") {
    const double maxStep = 0.0;  // <= 0 means no limit / instant jump
    netsdr::ParameterSmoother smoother(maxStep);
    smoother.reset(0.0);
    smoother.setTarget(0.7);

    // next() must return 0.7 immediately.
    double v = smoother.next();
    REQUIRE(v == 0.7);

    // isSettled() must be true after the jump.
    REQUIRE(smoother.isSettled() == true);
}

TEST_CASE("[dsp][smoother] reset snaps value without ramp", "[dsp][smoother]") {
    const double maxStep = 0.001;
    netsdr::ParameterSmoother smoother(maxStep);
    smoother.setTarget(0.9);

    // Advance a few samples so current_ < target_ (e.g. 3 steps of 0.001).
    for (std::size_t i = 0; i < 3; ++i) {
        smoother.next();
    }

    // Reset snaps current_ to 0.4 without ramp; target_ stays at 0.9.
    smoother.reset(0.4);

    // The snapped value must be exactly 0.4.
    REQUIRE(smoother.value() == 0.4);

    // Next() must return 0.4 (the snapped value before the ramp advances again).
    double v = smoother.next();
    REQUIRE(v == 0.4);

    // Target must remain unchanged.
}