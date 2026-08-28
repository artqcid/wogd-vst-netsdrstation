// AUTO-GENERATED from schema/bridge.schema.json - DO NOT EDIT.
//
// Wire contract of the UI <-> C++ bridge (see
// source/vst/common/bridge_protocol.h for the message
// envelope). Regenerate with:
//   python schema/generate-cpp.py schema/bridge.schema.json \
//       --output source/vst/common/generated/bridge_schema.h
#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace netsdr::schema {

// Stable UI-facing parameter names (ParamId in the schema).
inline bool isParamId(const std::string& id) {
    static const char* const kIds[] = {
        "mode",
        "freqKhz",
        "lowCut",
        "highCut",
        "agcOn",
        "agcHang",
        "agcThresh",
        "agcSlope",
        "agcDecay",
        "agcManGain",
        "volume",
        "mute",
        "squelchOn",
        "squelchThr",
        "nbOn",
        "nbThresh",
        "nrOn",
        "deempOn",
        "compOn",
        "wfOn",
        "wfSpeed",
        "wfZoom",
        "wfMaxDb",
        "wfMinDb",
        "wfComp",
        "arOn",
        "ovOn",
    };
    for (const char* k : kIds) {
        if (id == k) return true;
    }
    return false;
}

// Result of parsing a setParameter bridge message.
struct BridgeSetParameter {
    std::string id;
    double value = 0.0;
};

// Result of parsing a setStation bridge message.
struct BridgeSetStation {
    std::string hostPort;
};

inline bool parseSetParameter(const nlohmann::json& j, BridgeSetParameter& out) {
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "setParameter") return false;
    if (!j.contains("data") || !j["data"].is_array()) return false;
    const auto& d = j["data"];
    if (d.size() != 2) return false;
    if (!d[0].is_string() || !isParamId(d[0].get<std::string>())) return false;
    if (!d[1].is_number()) return false;
    out.id = d[0].get<std::string>();
    out.value = d[1].get<double>();
    return true;
}

inline bool parseSetStation(const nlohmann::json& j, BridgeSetStation& out) {
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "setStation") return false;
    if (!j.contains("data") || !j["data"].is_array()) return false;
    const auto& d = j["data"];
    if (d.size() != 1) return false;
    if (!d[0].is_string() || d[0].get<std::string>().empty()) return false;
    out.hostPort = d[0].get<std::string>();
    return true;
}

inline bool parseDisconnect(const nlohmann::json& j) {
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "disconnect") return false;
    return !j.contains("data") || j["data"].is_null();
}

inline bool parseGetParameters(const nlohmann::json& j) {
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "getParameters") return false;
    return !j.contains("data") || j["data"].is_null();
}

inline bool parseLevel(const nlohmann::json& j, double& outDbM) {
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "level") return false;
    if (!j.contains("data") || !j["data"].is_array()) return false;
    const auto& d = j["data"];
    if (d.size() != 1 || !d[0].is_number()) return false;
    outDbM = d[0].get<double>();
    return true;
}

} // namespace netsdr::schema
