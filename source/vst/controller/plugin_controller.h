#pragma once
// VST3 edit controller for the NetSDRStation sine-synth proof (Milestone M1).
//
// The controller owns the parameter *definitions* (IDs, ranges, defaults) that
// the DAW exposes for automation, and forwards parameter edits back to the
// processor. It creates the plugin editor (webview) via createView().

#include "vst/common/parameter_registry.h"

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

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

    // Component handler: forward a UI-initiated parameter change to the host.
    Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID tag,
                                              Steinberg::Vst::ParamValue valueNormalized) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;

    // The shared definition set.
    const ParameterRegistry& registry() const { return registry_; }

private:
    ParameterRegistry registry_;
};

} // namespace netsdr
