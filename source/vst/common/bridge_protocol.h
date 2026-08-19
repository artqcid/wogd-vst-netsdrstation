#pragma once
// Wire contract of the UI <-> C++ bridge (see source/webview/webview_editor.cpp).
//
// webview/webview's `bind` serializes the JS call arguments into a JSON array
// string (e.g. calling window.vstHostSetParameter("freq", 440) produces
// ["freq",440]). WebViewHost::dispatchMessage wraps that into one envelope:
//
//   {"type":"setParameter","data":["freq",440]}
//
// This header holds the pure-C++ parser for that envelope and the stable
// UI-facing parameter names, so the bridge logic can be unit-tested without
// the VST3 SDK or webview.

#include <cstdint>
#include <string>

namespace netsdr {

// UI-facing parameter names. These are the stable strings the Vue UI sends
// (see ui/src/views/PluginView.vue and ui/src/services/pluginService.ts) and
// must never change once released.
inline constexpr const char* kUiParamFreq = "freq";
inline constexpr const char* kUiParamVolume = "volume";
inline constexpr const char* kUiParamMute = "mute";

// Result of parsing a setParameter bridge message.
struct BridgeSetParameter {
    std::string id;      // UI parameter name ("freq"/"volume"/"mute")
    double value = 0.0;  // plain (unnormalized) parameter value
};

// Parses a bridge message envelope and fills `out` when it is a well-formed
// setParameter message:
//
//   {"type":"setParameter","data":["<id>",<value>]}
//
// Returns false for any other message type (getParameters, resize, ...) or for
// a malformed payload.
bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out);

// Maps a UI parameter name to its stable VST3 ParamID (see vst/common/paramids.h).
// Returns false when the name is unknown.
bool paramIdFromUiName(const std::string& id, std::uint32_t& outId);

} // namespace netsdr
