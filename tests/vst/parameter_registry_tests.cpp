// Unit tests for the ParameterRegistry (Milestone M3.2).
// Verifies parameter IDs, ranges, defaults and add/get roundtrip and
// normalized<->plain conversions under the new parameter model.

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

TEST_CASE("ParameterRegistry: 27 parameters defined", "[vst][params]") {
    auto registry = makeRegistry();

    // M3.2: full parameter set of 27 parameters.
    REQUIRE(registry.definitions().size() == 27);
}

TEST_CASE("ParameterRegistry: key definitions spot-checked", "[vst][params]") {
    auto registry = makeRegistry();

    // kParamMode: min 0, max 17, default 0, stepCount 17
    const auto* mode = registry.definition(netsdr::kParamMode);
    REQUIRE(mode != nullptr);
    REQUIRE(mode->min == 0.0);
    REQUIRE(mode->max == 17.0);
    REQUIRE(mode->defaultValue == 0.0);

    // kParamFreqKhz: min 0.001, max 30000, default 14100, stepCount 0
    const auto* freq = registry.definition(netsdr::kParamFreqKhz);
    REQUIRE(freq != nullptr);
    REQUIRE(freq->min == 0.001);
    REQUIRE(freq->max == 30000.0);
    REQUIRE(freq->defaultValue == 14100.0);

    // kParamVolume: min 0, max 1, default 1
    const auto* volume = registry.definition(netsdr::kParamVolume);
    REQUIRE(volume != nullptr);
    REQUIRE(volume->min == 0.0);
    REQUIRE(volume->max == 1.0);
    REQUIRE(volume->defaultValue == 1.0);

    // kParamMute default 0, stepCount 1
    const auto* mute = registry.definition(netsdr::kParamMute);
    REQUIRE(mute != nullptr);
    REQUIRE(mute->defaultValue == 0.0);

    // kParamLowCut default -4900
    const auto* lowCut = registry.definition(netsdr::kParamLowCut);
    REQUIRE(lowCut != nullptr);
    REQUIRE(lowCut->defaultValue == -4900.0);

    // kParamAgcOn default 1, stepCount 1
    const auto* agcOn = registry.definition(netsdr::kParamAgcOn);
    REQUIRE(agcOn != nullptr);
    REQUIRE(agcOn->defaultValue == 1.0);
}

TEST_CASE("ParameterRegistry: mute is a discrete toggle, others are continuous",
          "[vst][params]") {
    auto registry = makeRegistry();

    // FIX-07: mute must be registered as a binary toggle (stepCount == 1) so a
    // DAW shows an on/off switch instead of a continuous knob.
    REQUIRE(registry.definition(netsdr::kParamMute)->stepCount == 1);
    REQUIRE(registry.definition(netsdr::kParamFreqKhz)->stepCount == 0);
    REQUIRE(registry.definition(netsdr::kParamVolume)->stepCount == 0);
    REQUIRE(registry.definition(netsdr::kParamMode)->stepCount == 17);
}

TEST_CASE("ParameterRegistry: default values are stored normalized",
          "[vst][params]") {
    auto registry = makeRegistry();
    // kParamFreqKhz default 14100 in [0.001,30000] ->
    // norm = (14100.0-0.001)/(30000.0-0.001)
    const double normFreq = registry.value(netsdr::kParamFreqKhz);
    REQUIRE(std::abs(normFreq - (14100.0 - 0.001) / (30000.0 - 0.001)) < 1e-9);
    // kParamVolume default 1.0 -> normalized 1.0
    REQUIRE(registry.value(netsdr::kParamVolume) == 1.0);
    // kParamMute default 0 -> normalized 0
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

    // kParamFreqKhz: toPlain(kParamFreqKhz, 0.5) == 0.001 + 0.5*(30000.0-0.001)
    const double mid = registry.toPlain(netsdr::kParamFreqKhz, 0.5);
    REQUIRE(std::abs(mid - (0.001 + 0.5 * (30000.0 - 0.001))) < 1e-6);

    const double norm = registry.toNormalized(netsdr::kParamFreqKhz, mid);
    REQUIRE(std::abs(norm - 0.5) < 1e-9);

    // Unknown id returns a safe default.
    REQUIRE(registry.toPlain(999u, 0.5) == 0.0);
    REQUIRE(registry.value(999u) == 0.0);
}

TEST_CASE("ParameterRegistry: all 27 parameter IDs resolve via definition()",
          "[vst][params]") {
    auto registry = makeRegistry();

    // Every ID from 0 through kNumParams-1 must have a non-null definition.
    for (uint32_t id = 0; id < netsdr::kNumParams; ++id) {
        REQUIRE(registry.definition(id) != nullptr);
    }
    // Size check is already verified in the first test, but also ensure
    // definitions().size() == kNumParams.
    REQUIRE(static_cast<size_t>(netsdr::kNumParams) == registry.definitions().size());
}