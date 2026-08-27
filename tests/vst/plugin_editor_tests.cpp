// Unit tests for the Plugin editor (TEST-06/07/08/09/10).
// Covers the normalization chain, UI URL, and size constraint clamping.

#include "catch.hpp"
#include "vst/common/bridge_protocol.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"
#include "editor/plugin_editor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include <cmath>
#include <utility>
#include <vector>

namespace {

// Minimal concrete EditControllerEx1: records edit gestures so tests can
// verify that UI messages are routed to the host (beginEdit/performEdit/
// endEdit). The base forwards these to its componentHandler member; we
// intercept to observe the calls themselves.
class MockController : public Steinberg::Vst::EditControllerEx1 {
public:
    Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID) override {
        ++beginCount;
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID tag,
                                              Steinberg::Vst::ParamValue value) override {
        edits.emplace_back(tag, value);
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID) override {
        ++endCount;
        return Steinberg::kResultOk;
    }

    int beginCount = 0;
    int endCount = 0;
    std::vector<std::pair<Steinberg::Vst::ParamID, Steinberg::Vst::ParamValue>> edits;
};

} // namespace

// TEST-06: onJavaScriptMessage normalization chain (end-to-end via bridge protocol)
// Since PluginEditor requires a host window handle for attached(), a full
// instantiation is not possible in a headless unit test. Instead, we verify
// the normalization path that onJavaScriptMessage invokes:
//   parseSetParameterMessage -> paramIdFromUiName -> registry.toNormalized

TEST_CASE("PluginEditor: onJavaScriptMessage normalization chain (TEST-06)",
          "[vst][editor]") {
    // This test covers the normalization chain that onJavaScriptMessage invokes:
    // parseSetParameterMessage -> paramIdFromUiName -> registry.toNormalized

    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());

    auto makeEnvelope = [](const std::string& id, double value) -> std::string {
        return "{\"type\":\"setParameter\",\"data\":[\"" + id + "\"," + std::to_string(value) + "]}";
    };

    // freqKhz: UI sends 7100.5 kHz, normalized must be (7100.5-0.001)/(30000.0-0.001)
    netsdr::BridgeSetParameter parsed;
    REQUIRE(netsdr::parseSetParameterMessage(makeEnvelope("freqKhz", 7100.5), parsed));
    std::uint32_t tag;
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));
    double norm = registry.toNormalized(tag, parsed.value);
    double expected = (7100.5 - 0.001) / (30000.0 - 0.001);
    REQUIRE(std::abs(norm - expected) < 1e-9);

    // volume: UI sends 0.5, normalized = 0.5 (range 0..1)
    REQUIRE(netsdr::parseSetParameterMessage(makeEnvelope("volume", 0.5), parsed));
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));
    REQUIRE(registry.toNormalized(tag, parsed.value) == 0.5);

    // mute: UI sends 1, normalized = 1.0
    REQUIRE(netsdr::parseSetParameterMessage(makeEnvelope("mute", 1), parsed));
    REQUIRE(netsdr::paramIdFromUiName(parsed.id, tag));
    REQUIRE(registry.toNormalized(tag, parsed.value) == 1.0);

    // Unknown parameter: must not normalise
    REQUIRE_FALSE(netsdr::paramIdFromUiName("unknown", tag));
}

// TEST-07: uiUrl() returns correct URL per build type
// In debug builds (NDEBUG not defined), uiUrl() returns "http://localhost:5173".
// In release builds it must return a file:// URL that points at the ui/
// folder next to the plugin module (bundle layout: Contents/x86_64-win/ui).
TEST_CASE("PluginEditor: uiUrl returns correct URL per build type (TEST-07)",
          "[vst][editor]") {
    const std::string url = netsdr::PluginEditor::uiUrl();
#ifndef NDEBUG
    REQUIRE(url == std::string("http://localhost:5173"));
#else
    REQUIRE(url.rfind("file:///", 0) == 0);
    REQUIRE(url.find("/ui/index.html") != std::string::npos);
#endif
}

// TEST-08: checkSizeConstraint clamps to minimum size
// Construct a PluginEditor with a mock controller, then test checkSizeConstraint.
TEST_CASE("PluginEditor: checkSizeConstraint clamps to minimum (TEST-08)",
          "[vst][editor]") {
    // MockController satisfies the IEditController interface required by
    // PluginEditor's constructor.
    MockController mockCtrl;
    netsdr::ParameterRegistry reg(netsdr::createParameterDefinitions());
    netsdr::PluginEditor editor(&mockCtrl, reg);

    // Below minimum -> clamped
    Steinberg::ViewRect small(0, 0, 100, 50);
    editor.checkSizeConstraint(&small);
    REQUIRE(small.getWidth() == 320);
    REQUIRE(small.getHeight() == 200);

    // Above minimum -> unchanged
    Steinberg::ViewRect large(0, 0, 800, 600);
    editor.checkSizeConstraint(&large);
    REQUIRE(large.getWidth() == 800);
    REQUIRE(large.getHeight() == 600);
}

// TEST-09: UI messages must reach the host edit gestures (BUG-02 regression).
// onJavaScriptMessage must call beginEdit/performEdit/endEdit on the
// controller so the host routes the change into the processor's
// inputParameterChanges. A message with an unknown parameter id must be
// ignored entirely.
TEST_CASE("PluginEditor: UI messages reach host edit gestures (TEST-09)",
          "[vst][editor]") {
    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());
    MockController mockCtrl;
    netsdr::PluginEditor editor(&mockCtrl, registry);

    auto makeEnvelope = [](const std::string& id, double value) -> std::string {
        return "{\"type\":\"setParameter\",\"data\":[\"" + id + "\"," +
               std::to_string(value) + "]}";
    };

    // Known parameter: full gesture with normalized value.
    editor.onJavaScriptMessage(makeEnvelope("freqKhz", 7100.5).c_str());
    REQUIRE(mockCtrl.beginCount == 1);
    REQUIRE(mockCtrl.endCount == 1);
    REQUIRE(mockCtrl.edits.size() == 1);
    if (mockCtrl.edits.size() == 1) {
        REQUIRE(mockCtrl.edits[0].first == netsdr::kParamFreqKhz);
        const double expected =
            registry.toNormalized(netsdr::kParamFreqKhz, 7100.5);
        REQUIRE(std::abs(mockCtrl.edits[0].second - expected) < 1e-9);
    }

    // Unknown parameter: no gesture at all.
    editor.onJavaScriptMessage(makeEnvelope("unknown", 1.0).c_str());
    REQUIRE(mockCtrl.beginCount == 1);
    REQUIRE(mockCtrl.endCount == 1);
    REQUIRE(mockCtrl.edits.size() == 1);

    // Malformed JSON: no gesture at all.
    editor.onJavaScriptMessage("{\"type\":\"setParameter\"}");
    REQUIRE(mockCtrl.beginCount == 1);
    REQUIRE(mockCtrl.endCount == 1);
    REQUIRE(mockCtrl.edits.size() == 1);
}

// TEST-10: setStation envelope is parsed but does NOT produce edit gestures
// on a non-PluginController mock. The editor's dispatch path safely ignores
// setStation when no PluginController is present (dynamic_cast fails → no-op).
TEST_CASE("PluginEditor: setStation does not produce edit gestures without PluginController (TEST-10)",
          "[vst][editor]") {
    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());
    MockController mockCtrl;
    netsdr::PluginEditor editor(&mockCtrl, registry);

    // setStation message — should be safely ignored since MockController
    // is not a PluginController (dynamic_cast fails → no-op).
    editor.onJavaScriptMessage(
        "{\"type\":\"setStation\",\"data\":[\"127.0.0.1:8073\"]}");

    // Counts must remain at 0 (no beginEdit/performEdit/endEdit called).
    REQUIRE(mockCtrl.beginCount == 0);
    REQUIRE(mockCtrl.endCount == 0);
    REQUIRE(mockCtrl.edits.empty());
}