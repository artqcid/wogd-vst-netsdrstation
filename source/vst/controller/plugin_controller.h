#pragma once
// VST3 edit controller for the NetSDRStation sine-synth proof (Milestone M1).
//
// The controller owns the parameter *definitions* (IDs, ranges, defaults) that
// the DAW exposes for automation, and forwards parameter edits back to the
// controller. It creates the plugin editor (webview) via createView().

#include "vst/common/parameter_registry.h"

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include <functional>
#include <string>
#include <vector>

namespace netsdr {

class PluginController : public Steinberg::Vst::EditControllerEx1 {
public:
    PluginController();

    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return static_cast<Steinberg::Vst::IEditController*>(new PluginController());
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    // IEditController
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

    // Sends a station-change message ("host:port") to the processor through
    // the host peer (IConnectionPoint messaging). No-op when no peer/host context.
    void setStation(const std::string& hostPort);

    // Sends a disconnect message to the processor through the host peer.
    // No-op when no peer/host context.
    void disconnect();

    // Status sink: receives connection status strings ("Connecting"/"Connected"/
    // "Error"/"Disconnected") from the controller and forwards them UI-wards.
    void setStatusSink(const std::function<void(const std::string&)>& sink);

    // Level sink: receives the audio signal level in dBm (S-meter) from the
    // processor and forwards it UI-wards.
    void setLevelSink(const std::function<void(float)>& sink);

    // Waterfall sink: receives spectrum bins (dBFS) from the processor and
    // forwards them UI-wards.
    void setWaterfallSink(const std::function<void(const std::vector<float>&)>& sink);

    // IConnectionPoint: receives "NetSDRStation:Status" messages from the processor.
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

    // Component handler: forward a UI-initiated parameter change to the host.
    Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID tag,
                                              Steinberg::Vst::ParamValue valueNormalized) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;

    // The shared definition set.
    const ParameterRegistry& registry() const { return registry_; }

private:
    ParameterRegistry registry_;
    std::function<void(const std::string&)> statusSink_;
    std::function<void(float)> levelSink_;
    std::function<void(const std::vector<float>&)> waterfallSink_;
};

} // namespace netsdr
