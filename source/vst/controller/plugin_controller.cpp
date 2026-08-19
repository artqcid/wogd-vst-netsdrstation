#include "plugin_controller.h"

#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "editor/plugin_editor.h"
#include "version.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

#include "base/source/fstreamer.h"

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

PluginController::PluginController()
    : registry_(createParameterDefinitions()) {
}

tresult PLUGIN_API PluginController::initialize(FUnknown* context) {
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    for (const auto& def : registry_.definitions()) {
        int32 flags = ParameterInfo::kCanAutomate;
        int32 stepCount = 0;
        if (def.isBypass) {
            flags |= ParameterInfo::kIsBypass;
            stepCount = 1;
        }
        parameters.addParameter(
            USTRING(def.title.c_str()), USTRING(def.units.c_str()),
            stepCount, registry_.value(def.id), flags, def.id);
    }
    return kResultOk;
}

tresult PLUGIN_API PluginController::terminate() {
    return EditControllerEx1::terminate();
}

tresult PLUGIN_API PluginController::setComponentState(IBStream* state) {
    // The processor state is not needed for the sine-synth proof; accept it
    // without modifying controller-side parameters.
    return kResultOk;
}

IPlugView* PLUGIN_API PluginController::createView(FIDString name) {
    if (name && FIDStringsEqual(name, ViewType::kEditor)) {
        return new PluginEditor(this);
    }
    return nullptr;
}

tresult PluginController::beginEdit(ParamID tag) {
    return EditControllerEx1::beginEdit(tag);
}

tresult PluginController::performEdit(ParamID tag, ParamValue valueNormalized) {
    return EditControllerEx1::performEdit(tag, valueNormalized);
}

tresult PluginController::endEdit(ParamID tag) {
    return EditControllerEx1::endEdit(tag);
}

} // namespace netsdr
