// Unit tests for the ParameterRegistry (Milestone M1.2/M1.5).
// Verifies parameter IDs, ranges, defaults and add/get roundtrip and
// normalized<->plain conversions.

#include "catch.hpp"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"

#include <cmath>

namespace {

netsdr::ParameterRegistry makeRegistry() {
    return netsdr::ParameterRegistry(netsdr::createParameterDefinitions());
}

} // namespace

TEST_CASE("ParameterRegistry: three parameters with correct ids/ranges/defaults", "[vst][params]") {
    auto registry = makeRegistry();

    REQUIRE(registry.definitions().size() == 3);

    const auto* freq = registry.definition(netsdr::kParamFreq);
    REQUIRE(freq != nullptr);
    REQUIRE(freq->min == 20.0);
    REQUIRE(freq->max == 20000.0);
    REQUIRE(freq->defaultValue == 440.0);

    const auto* volume = registry.definition(netsdr::kParamVolume);
    REQUIRE(volume != nullptr);
    REQUIRE(volume->min == 0.0);
    REQUIRE(volume->max == 1.0);
    REQUIRE(volume->defaultValue == 1.0);

    const auto* mute = registry.definition(netsdr::kParamMute);
    REQUIRE(mute != nullptr);
    REQUIRE(mute->isBypass == false);
    REQUIRE(mute->defaultValue == 0.0);
}

TEST_CASE("ParameterRegistry: mute is a discrete toggle, others are continuous",
          "[vst][params]") {
    auto registry = makeRegistry();

    // FIX-07: mute must be registered as a binary toggle (stepCount == 1) so a
    // DAW shows an on/off switch instead of a continuous knob.
    REQUIRE(registry.definition(netsdr::kParamMute)->stepCount == 1);
    REQUIRE(registry.definition(netsdr::kParamFreq)->stepCount == 0);
    REQUIRE(registry.definition(netsdr::kParamVolume)->stepCount == 0);
}

TEST_CASE("ParameterRegistry: default values are stored normalized", "[vst][params]") {
    auto registry = makeRegistry();
    // freq default 440 Hz in [20,20000] -> normalized 440/19980 ~ 0.0210
    const double normFreq = registry.value(netsdr::kParamFreq);
    REQUIRE(std::abs(normFreq - (440.0 - 20.0) / (20000.0 - 20.0)) < 1e-9);
    // volume default 1.0 -> normalized 1.0
    REQUIRE(registry.value(netsdr::kParamVolume) == 1.0);
    // mute default 0 -> normalized 0
    REQUIRE(registry.value(netsdr::kParamMute) == 0.0);
}

TEST_CASE("ParameterRegistry: set/get roundtrip and clamping", "[vst][params]") {
    auto registry = makeRegistry();

    registry.setValue(netsdr::kParamVolume, 0.5);
    REQUIRE(registry.value(netsdr::kParamVolume) == 0.5);

    // Values outside [0,1] are clamped.
    registry.setValue(netsdr::kParamVolume, 2.0);
    REQUIRE(registry.value(netsdr::kParamVolume) == 1.0);
    registry.setValue(netsdr::kParamVolume, -1.0);
    REQUIRE(registry.value(netsdr::kParamVolume) == 0.0);
}

TEST_CASE("ParameterRegistry: normalized <-> plain conversions", "[vst][params]") {
    auto registry = makeRegistry();

    // freq: normalized 0.5 -> 20 + 0.5*19980 = 10010 Hz.
    const double mid = registry.toPlain(netsdr::kParamFreq, 0.5);
    REQUIRE(std::abs(mid - (20.0 + 0.5 * (20000.0 - 20.0))) < 1e-6);

    const double norm = registry.toNormalized(netsdr::kParamFreq, mid);
    REQUIRE(std::abs(norm - 0.5) < 1e-9);

    // Unknown id returns a safe default.
    REQUIRE(registry.toPlain(999u, 0.5) == 0.0);
    REQUIRE(registry.value(999u) == 0.0);
}
