// Unit tests for the UI <-> C++ bridge protocol (FIX-02/FIX-03).
// Verifies that the JSON envelope produced by WebViewHost::dispatchMessage is
// parsed correctly (id + plain value) and that the UI parameter names map to
// the stable ParamIDs and normalize through the ParameterRegistry.

#include "catch.hpp"
#include "vst/common/bridge_protocol.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"

#include <cmath>

namespace {

// Reproduces the exact envelope built by WebViewHost::dispatchMessage for a
// setParameter binding call (webview serializes the JS args as a JSON array).
std::string setParameterEnvelope(const std::string& id, double value) {
    const std::string idJson = "\"" + id + "\"";
    const std::string payload = "[" + idJson + "," + std::to_string(value) + "]";
    return "{\"type\":\"setParameter\",\"data\":" + payload + "}";
}

} // namespace

TEST_CASE("Bridge: parseSetParameterMessage extracts id and integer value",
          "[vst][bridge]") {
    netsdr::BridgeSetParameter out;
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("freq", 440), out));
    REQUIRE(out.id == "freq");
    REQUIRE(out.value == 440.0);
}

TEST_CASE("Bridge: parseSetParameterMessage extracts a fractional value",
          "[vst][bridge]") {
    netsdr::BridgeSetParameter out;
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("volume", 0.5), out));
    REQUIRE(out.id == "volume");
    REQUIRE(out.value == 0.5);
}

TEST_CASE("Bridge: parseSetParameterMessage rejects non-setParameter messages",
          "[vst][bridge]") {
    netsdr::BridgeSetParameter out;
    // getParameters envelope carries no id/value.
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"getParameters\",\"data\":null}", out));
    // resize envelope carries an array of two ints but no "setParameter" type.
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"resize\",\"data\":[640,400]}", out));
}

TEST_CASE("Bridge: parseSetParameterMessage rejects malformed payloads",
          "[vst][bridge]") {
    netsdr::BridgeSetParameter out;
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"setParameter\"}", out));          // no data
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"setParameter\",\"data\":null}", out)); // no array
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"setParameter\",\"data\":[\"freq\"]}", out)); // missing value
}

TEST_CASE("Bridge: paramIdFromUiName maps the stable UI names", "[vst][bridge]") {
    std::uint32_t id = 0;
    REQUIRE(netsdr::paramIdFromUiName("freq", id));
    REQUIRE(id == netsdr::kParamFreq);
    REQUIRE(netsdr::paramIdFromUiName("volume", id));
    REQUIRE(id == netsdr::kParamVolume);
    REQUIRE(netsdr::paramIdFromUiName("mute", id));
    REQUIRE(id == netsdr::kParamMute);
    REQUIRE_FALSE(netsdr::paramIdFromUiName("unknown", id));
}

TEST_CASE("Bridge: plain frequency normalizes through the registry (FIX-03)",
          "[vst][bridge]") {
    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());

    // The UI sends 440 Hz (plain); the registry must normalize it to
    // (440 - 20) / (20000 - 20) ~= 0.021, not clamp it to 1.0.
    netsdr::BridgeSetParameter parsed;
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("freq", 440), parsed));

    std::uint32_t tag = 0;
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));

    const double normalized = registry.toNormalized(tag, parsed.value);
    const double expected = (440.0 - 20.0) / (20000.0 - 20.0);
    REQUIRE(std::abs(normalized - expected) < 1e-9);

    // Volume is already 0..1: plain == normalized.
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("volume", 0.5), parsed));
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));
    REQUIRE(registry.toNormalized(tag, parsed.value) == 0.5);
}
