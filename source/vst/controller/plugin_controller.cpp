#include "plugin_controller.h"

#include "common/diag.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/processor_state.h"
#include "editor/plugin_editor.h"
#include "version.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include "base/source/fstreamer.h"
#include "base/source/fstring.h"

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
    // Get the whole variable-length stream size.
    IBStreamer streamer(state, kLittleEndian);
    int64 size = 0;
    state->seek(0, IBStream::kIBSeekEnd, &size);
    int64 pos = 0;
    state->seek(0, IBStream::kIBSeekSet, &pos);
    if (size <= 0) {
        return kResultOk;
    }
    // Read the entire stream into a string.
    std::string bytes(static_cast<TSize>(size), '\0');
    if (streamer.readRaw(bytes.data(), static_cast<TSize>(size)) != static_cast<TSize>(size)) {
        return kResultFalse;
    }
    ProcessorState s;
    if (!s.deserialize(bytes)) {
        return kResultFalse;
    }
    // Mirror all 27 params into the controller registry.
    setParamNormalized(kParamMode, registry_.toNormalized(kParamMode, s.mode));
    setParamNormalized(kParamFreqKhz, registry_.toNormalized(kParamFreqKhz, s.freqKhz));
    setParamNormalized(kParamLowCut, registry_.toNormalized(kParamLowCut, s.lowCut));
    setParamNormalized(kParamHighCut, registry_.toNormalized(kParamHighCut, s.highCut));
    setParamNormalized(kParamAgcOn, registry_.toNormalized(kParamAgcOn, s.agcOn));
    setParamNormalized(kParamAgcHang, registry_.toNormalized(kParamAgcHang, s.agcHang));
    setParamNormalized(kParamAgcThresh, registry_.toNormalized(kParamAgcThresh, s.agcThresh));
    setParamNormalized(kParamAgcSlope, registry_.toNormalized(kParamAgcSlope, s.agcSlope));
    setParamNormalized(kParamAgcDecay, registry_.toNormalized(kParamAgcDecay, s.agcDecay));
    setParamNormalized(kParamAgcManGain, registry_.toNormalized(kParamAgcManGain, s.agcManGain));
    setParamNormalized(kParamVolume, registry_.toNormalized(kParamVolume, s.volume));
    setParamNormalized(kParamMute, registry_.toNormalized(kParamMute, s.mute ? 1.0 : 0.0));
    setParamNormalized(kParamSquelchOn, registry_.toNormalized(kParamSquelchOn, s.squelchOn));
    setParamNormalized(kParamSquelchThr, registry_.toNormalized(kParamSquelchThr, s.squelchThr));
    setParamNormalized(kParamNbOn, registry_.toNormalized(kParamNbOn, s.nbOn));
    setParamNormalized(kParamNbThresh, registry_.toNormalized(kParamNbThresh, s.nbThresh));
    setParamNormalized(kParamNrOn, registry_.toNormalized(kParamNrOn, s.nrOn));
    setParamNormalized(kParamDeempOn, registry_.toNormalized(kParamDeempOn, s.deempOn));
    setParamNormalized(kParamCompOn, registry_.toNormalized(kParamCompOn, s.compOn));
    setParamNormalized(kParamWfOn, registry_.toNormalized(kParamWfOn, s.wfOn));
    setParamNormalized(kParamWfSpeed, registry_.toNormalized(kParamWfSpeed, s.wfSpeed));
    setParamNormalized(kParamWfZoom, registry_.toNormalized(kParamWfZoom, s.wfZoom));
    setParamNormalized(kParamWfMaxDb, registry_.toNormalized(kParamWfMaxDb, s.wfMaxDb));
    setParamNormalized(kParamWfMinDb, registry_.toNormalized(kParamWfMinDb, s.wfMinDb));
    setParamNormalized(kParamWfComp, registry_.toNormalized(kParamWfComp, s.wfComp));
    setParamNormalized(kParamArOn, registry_.toNormalized(kParamArOn, s.arOn));
    setParamNormalized(kParamOvOn, registry_.toNormalized(kParamOvOn, s.ovOn));
    return kResultOk;
}

// ---------------------------------------------------------------------------
// notify: receive status messages from the processor
// ---------------------------------------------------------------------------
tresult PLUGIN_API PluginController::notify(IMessage* message) {
    if (message != nullptr && message->getMessageID() != nullptr) {
        diagLog("controller notify: id=%s", message->getMessageID());
        if (FIDStringsEqual(message->getMessageID(), "NetSDRStation:Status")) {
            TChar status[64] = {};
            if (message->getAttributes()->getString("Status", status, sizeof(status)) == kResultOk) {
                Steinberg::String tmp(status);
                tmp.toMultiByte(Steinberg::kCP_Utf8);
                if (statusSink_) {
                    statusSink_(tmp.text8());
                }
            }
            return kResultOk;
        }
    }
    return ComponentBase::notify(message);
}

void PluginController::setStation(const std::string& hostPort) {
    diagLog("controller setStation: hostPort=%s", hostPort.c_str());
    if (hostPort.empty()) {
        return;
    }
    if (auto msg = IPtr<IMessage>(allocateMessage())) {
        msg->setMessageID("NetSDRStation:SetStation");
        Steinberg::String tmp(hostPort.c_str(), Steinberg::kCP_Utf8);
        msg->getAttributes()->setString("HostPort", tmp.text16());
        sendMessage(msg);
        diagLog("controller setStation: message sent");
    } else {
        diagLog("controller setStation: allocateMessage returned NULL (no IHostApplication context?)");
    }
}

void PluginController::disconnect() {
    diagLog("controller disconnect");
    if (auto msg = IPtr<IMessage>(allocateMessage())) {
        msg->setMessageID("NetSDRStation:Disconnect");
        sendMessage(msg);
        diagLog("controller disconnect: message sent");
    } else {
        diagLog("controller disconnect: allocateMessage returned NULL (no IHostApplication context?)");
    }
}

void PluginController::setStatusSink(const std::function<void(const std::string&)>& sink) {
    statusSink_ = sink;
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
