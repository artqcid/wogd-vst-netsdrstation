#include "catch.hpp"
#include "vst/controller/plugin_controller.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/processor_state.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/common/memorystream.h"

#include <cmath>

TEST_CASE("PluginController: setComponentState mirrors processor state into params (TEST-05)",
          "[vst][controller]") {
    netsdr::PluginController controller;
    controller.initialize(nullptr);

    // Build a known processor state (v2) with freqKhz, volume, mute, mode, etc.
    // station is empty → no connect attempt.
    netsdr::ProcessorState state;
    state.station = "";
    state.freqKhz = 7100.5;
    state.volume = 0.25;
    state.mute = 1.0;
    state.mode = 2.0;
    state.agcOn = 0.0;
    state.lowCut = -100.0;
    state.highCut = 100.0;
    state.wfOn = 1.0;
    auto bytes = state.serialize();

    // Feed into the controller
    Steinberg::MemoryStream inStream(bytes.data(), static_cast<Steinberg::TSize>(bytes.size()));
    REQUIRE(controller.setComponentState(&inStream) == Steinberg::kResultOk);

    // Verify normalized values via getParamNormalized
    // freqKhz range: 0.001 .. 30000.0
    Steinberg::Vst::ParamValue normFreq = controller.getParamNormalized(netsdr::kParamFreqKhz);
    double expectedFreqNorm = (7100.5 - 0.001) / (30000.0 - 0.001);
    REQUIRE(std::abs(normFreq - expectedFreqNorm) < 1e-6);

    Steinberg::Vst::ParamValue normVol = controller.getParamNormalized(netsdr::kParamVolume);
    REQUIRE(normVol == 0.25);

    Steinberg::Vst::ParamValue normMute = controller.getParamNormalized(netsdr::kParamMute);
    REQUIRE(normMute == 1.0); // mute=1 → normalized=1.0

    Steinberg::Vst::ParamValue normMode = controller.getParamNormalized(netsdr::kParamMode);
    double expectedModeNorm = (2.0 - 0.0) / (17.0 - 0.0);
    REQUIRE(std::abs(normMode - expectedModeNorm) < 1e-6);

    Steinberg::Vst::ParamValue normAgcOn = controller.getParamNormalized(netsdr::kParamAgcOn);
    REQUIRE(normAgcOn == 0.0);

    // lowCut range: -8000.0 .. 0.0
    Steinberg::Vst::ParamValue normLowCut = controller.getParamNormalized(netsdr::kParamLowCut);
    double expectedLowCutNorm = (-100.0 - (-8000.0)) / (0.0 - (-8000.0));
    REQUIRE(std::abs(normLowCut - expectedLowCutNorm) < 1e-6);

    // highCut range: 0.0 .. 8000.0
    Steinberg::Vst::ParamValue normHighCut = controller.getParamNormalized(netsdr::kParamHighCut);
    double expectedHighCutNorm = (100.0 - 0.0) / (8000.0 - 0.0);
    REQUIRE(std::abs(normHighCut - expectedHighCutNorm) < 1e-6);

    Steinberg::Vst::ParamValue normWfOn = controller.getParamNormalized(netsdr::kParamWfOn);
    REQUIRE(normWfOn == 1.0);
}