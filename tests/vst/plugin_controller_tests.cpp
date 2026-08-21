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

    // Build a known processor state
    netsdr::ProcessorState state;
    state.freqHz = 10000.0;
    state.volume = 0.25;
    state.mute = true;
    auto bytes = state.serialize();

    // Feed into the controller
    Steinberg::MemoryStream inStream(bytes.data(), static_cast<Steinberg::TSize>(bytes.size()));
    REQUIRE(controller.setComponentState(&inStream) == Steinberg::kResultOk);

    // Verify normalized values via getParamNormalized
    Steinberg::Vst::ParamValue normFreq = controller.getParamNormalized(netsdr::kParamFreq);
    double expectedFreqNorm = (10000.0 - 20.0) / (20000.0 - 20.0);
    REQUIRE(std::abs(normFreq - expectedFreqNorm) < 1e-6);

    Steinberg::Vst::ParamValue normVol = controller.getParamNormalized(netsdr::kParamVolume);
    REQUIRE(normVol == 0.25);

    Steinberg::Vst::ParamValue normMute = controller.getParamNormalized(netsdr::kParamMute);
    REQUIRE(normMute == 1.0); // mute=true → normalized=1.0
}