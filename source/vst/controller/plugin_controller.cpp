#include "plugin_controller.h"

#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/processor_state.h"
#include "editor/plugin_editor.h"
#include "version.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

#include "base/source/fstreamer.h"

#include <string>

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
        if (def.isBypass) {
            flags |= ParameterInfo::kIsBypass;
        }
        parameters.addParameter(
            USTRING(def.title.c_str()), USTRING(def.units.c_str()),
            def.stepCount, registry_.value(def.id), flags, def.id);
    }
    return kResultOk;
}

tresult PLUGIN_API PluginController::terminate() {
    return EditControllerEx1::terminate();
}

tresult PLUGIN_API PluginController::setComponentState(IBStream* state) {
    // Deserialize the processor state (same wire format as PluginProcessor) and
    // mirror it into the controller parameters so the UI/automation display
    // matches the loaded preset/project.
    if (state == nullptr) {
        return kResultFalse;
    }
    char buffer[ProcessorState::kSerializedSize] = {};
    IBStreamer streamer(state, kLittleEndian);
    if (streamer.readRaw(buffer, ProcessorState::kSerializedSize) !=
        static_cast<TSize>(ProcessorState::kSerializedSize)) {
        return kResultFalse;
    }
    ProcessorState s;
    if (!s.deserialize(std::string(buffer, ProcessorState::kSerializedSize))) {
        return kResultFalse;
    }
    setParamNormalized(kParamFreq, registry_.toNormalized(kParamFreq, s.freqHz));
    setParamNormalized(kParamVolume, registry_.toNormalized(kParamVolume, s.volume));
    setParamNormalized(kParamMute, registry_.toNormalized(kParamMute, s.mute ? 1.0 : 0.0));
    return kResultOk;
}

IPlugView* PLUGIN_API PluginController::createView(FIDString name) {
    if (name && FIDStringsEqual(name, ViewType::kEditor)) {
        return new PluginEditor(this, registry_);
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
