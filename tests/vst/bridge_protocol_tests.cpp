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

// Reproduces the exact envelope built by WebViewHost::dispatchMessage for a
// setParameter binding call (webview serializes the JS args as a JSON array).
std::string setParameterEnvelope(const std::string& id, double value) {
    const std::string idJson = "\"" + id + "\"";
    const std::string payload = "[" + idJson + "," + std::to_string(value) + "]";
    return "{\"type\":\"setParameter\",\"data\":" + payload + "}";
}

TEST_CASE("Bridge: parseSetParameterMessage extracts id and integer value",
          "[vst][bridge]") {
    netsdr::BridgeSetParameter out;
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("freqKhz", 14100), out));
    REQUIRE(out.id == "freqKhz");
    REQUIRE(out.value == 14100.0);
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
    REQUIRE_FALSE(netsdr::parseSetParameterMessage("{\"type\":\"setParameter\",\"data\":[\"freqKhz\"]}", out)); // missing value
}

TEST_CASE("Bridge: parseSetStationMessage extracts host:port", "[vst][bridge]") {
    netsdr::BridgeSetStation out;

    // Valid setStation message.
    REQUIRE(netsdr::parseSetStationMessage(
                "{\"type\":\"setStation\",\"data\":[\"g8ure.ddns.net:8078\"]}", out));
    REQUIRE(out.hostPort == "g8ure.ddns.net:8078");

    // Malformed: empty data array.
    netsdr::BridgeSetStation out2;
    REQUIRE_FALSE(netsdr::parseSetStationMessage(
                      "{\"type\":\"setStation\",\"data\":[]}", out2));

    // Not a setStation message.
    REQUIRE_FALSE(netsdr::parseSetStationMessage(
                      "{\"type\":\"setParameter\",\"data\":[\"freqKhz\",440]}", out2));
}

TEST_CASE("Bridge: parseDisconnectMessage detects the disconnect envelope", "[vst][bridge]") {
    REQUIRE(netsdr::parseDisconnectMessage("{\"type\":\"disconnect\",\"data\":null}"));
    REQUIRE(netsdr::parseDisconnectMessage("{\"type\":\"disconnect\"}"));
    REQUIRE_FALSE(netsdr::parseDisconnectMessage("{\"type\":\"setStation\",\"data\":[\"h:8072\"]}"));
    REQUIRE_FALSE(netsdr::parseDisconnectMessage("{\"type\":\"setParameter\",\"data\":[\"volume\",0.5]}"));
}

TEST_CASE("Bridge: paramIdFromUiName maps the stable UI names", "[vst][bridge]") {
    std::uint32_t id = 0;

    // "mode" -> kParamMode
    REQUIRE(netsdr::paramIdFromUiName("mode", id));
    REQUIRE(id == netsdr::kParamMode);

    // "lowCut" -> kParamLowCut
    REQUIRE(netsdr::paramIdFromUiName("lowCut", id));
    REQUIRE(id == netsdr::kParamLowCut);

    // "highCut" -> kParamHighCut
    REQUIRE(netsdr::paramIdFromUiName("highCut", id));
    REQUIRE(id == netsdr::kParamHighCut);

    // "agcOn" -> kParamAgcOn
    REQUIRE(netsdr::paramIdFromUiName("agcOn", id));
    REQUIRE(id == netsdr::kParamAgcOn);

    // "wfOn" -> kParamWfOn
    REQUIRE(netsdr::paramIdFromUiName("wfOn", id));
    REQUIRE(id == netsdr::kParamWfOn);

    // "volume" -> kParamVolume
    REQUIRE(netsdr::paramIdFromUiName("volume", id));
    REQUIRE(id == netsdr::kParamVolume);

    // "mute" -> kParamMute
    REQUIRE(netsdr::paramIdFromUiName("mute", id));
    REQUIRE(id == netsdr::kParamMute);

    // "unknown" returns false
    REQUIRE_FALSE(netsdr::paramIdFromUiName("unknown", id));

    // "freq" is no longer a valid UI name (replaced by "freqKhz")
    REQUIRE_FALSE(netsdr::paramIdFromUiName("freq", id));
}

TEST_CASE("Bridge: plain frequency normalizes through the registry (FIX-03)",
          "[vst][bridge]") {
    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());

    // The UI sends 14100 kHz (plain); the registry must normalize it to
    // (14100 - 0.001) / (30000 - 0.001)
    netsdr::BridgeSetParameter parsed;
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("freqKhz", 14100), parsed));

    std::uint32_t tag = 0;
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));

    const double normalized = registry.toNormalized(tag, parsed.value);
    const double expected = (14100.0 - 0.001) / (30000.0 - 0.001);
    REQUIRE(std::abs(normalized - expected) < 1e-9);

    // Volume is already 0..1: plain == normalized.
    REQUIRE(netsdr::parseSetParameterMessage(setParameterEnvelope("volume", 0.5), parsed));
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));
    REQUIRE(registry.toNormalized(tag, parsed.value) == 0.5);
}