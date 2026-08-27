#pragma once
// Wire contract of the UI <-> C++ bridge (see source/webview/webview_editor.cpp).
//
// webview/webview's `bind` serializes the JS call arguments into a JSON array
// string (e.g. calling window.vstHostSetParameter("freq", 440) produces
// ["freq",440]). WebViewHost::dispatchMessage wraps that into one envelope:
//
//   {"type":"setParameter","data":["freq",440]}
//
// This header holds the pure-C++ parsers for that envelope and the stable
// UI-facing parameter names, so the bridge logic can be unit-tested without
// the VST3 SDK or webview.

#include <cstdint>
#include <string>

namespace netsdr {

// UI-facing parameter names. These are the stable strings the Vue UI sends
// (see ui/src/views/PluginView.vue and ui/src/services/pluginService.ts) and
// must never change once released.
inline constexpr const char* kUiParamMode = "mode";
inline constexpr const char* kUiParamFreqKhz = "freqKhz";
inline constexpr const char* kUiParamLowCut = "lowCut";
inline constexpr const char* kUiParamHighCut = "highCut";
inline constexpr const char* kUiParamAgcOn = "agcOn";
inline constexpr const char* kUiParamAgcHang = "agcHang";
inline constexpr const char* kUiParamAgcThresh = "agcThresh";
inline constexpr const char* kUiParamAgcSlope = "agcSlope";
inline constexpr const char* kUiParamAgcDecay = "agcDecay";
inline constexpr const char* kUiParamAgcManGain = "agcManGain";
inline constexpr const char* kUiParamVolume = "volume";
inline constexpr const char* kUiParamMute = "mute";
inline constexpr const char* kUiParamSquelchOn = "squelchOn";
inline constexpr const char* kUiParamSquelchThr = "squelchThr";
inline constexpr const char* kUiParamNbOn = "nbOn";
inline constexpr const char* kUiParamNbThresh = "nbThresh";
inline constexpr const char* kUiParamNrOn = "nrOn";
inline constexpr const char* kUiParamDeempOn = "deempOn";
inline constexpr const char* kUiParamCompOn = "compOn";
inline constexpr const char* kUiParamWfOn = "wfOn";
inline constexpr const char* kUiParamWfSpeed = "wfSpeed";
inline constexpr const char* kUiParamWfZoom = "wfZoom";
inline constexpr const char* kUiParamWfMaxDb = "wfMaxDb";
inline constexpr const char* kUiParamWfMinDb = "wfMinDb";
inline constexpr const char* kUiParamWfComp = "wfComp";
inline constexpr const char* kUiParamArOn = "arOn";
inline constexpr const char* kUiParamOvOn = "ovOn";

// Result of parsing a setParameter bridge message.
struct BridgeSetParameter {
    std::string id;      // UI parameter name (e.g. "freqKhz")
    double value = 0.0;  // plain (unnormalized) parameter value
};

// Result of parsing a setStation bridge message.
struct BridgeSetStation {
    std::string hostPort; // "host:port"
};

// Parses a bridge message envelope and fills `out` when it is a well-formed
// setParameter message:
//
//   {"type":"setParameter","data":["<id>",<value>]}
//
// Returns false for any other message type (getParameters, resize, ...) or for
// a malformed payload.
bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out);

// Parses a station-change envelope:
//
//   {"type":"setStation","data":["<host:port>"]}
//
// Returns false for any other message type or a malformed payload.
bool parseSetStationMessage(const std::string& message, BridgeSetStation& out);

// Maps a UI parameter name to its stable VST3 ParamID (see vst/common/paramids.h).
// Returns false when the name is unknown.
bool paramIdFromUiName(const std::string& id, std::uint32_t& outId);

} // namespace netsdr