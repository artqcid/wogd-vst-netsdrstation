#include "bridge_protocol.h"

#include "vst/common/generated/bridge_schema.h"
#include "vst/common/paramids.h"

#include <nlohmann/json.hpp>

namespace netsdr {

bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out) {
    // Robust JSON parsing via nlohmann::json; the payload validation itself is
    // generated from schema/bridge.schema.json (single source of truth).
    nlohmann::json j = nlohmann::json::parse(message, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    return schema::parseSetParameter(j, out);
}

bool parseSetStationMessage(const std::string& message, BridgeSetStation& out) {
    nlohmann::json j = nlohmann::json::parse(message, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    return schema::parseSetStation(j, out);
}

bool parseDisconnectMessage(const std::string& message) {
    nlohmann::json j = nlohmann::json::parse(message, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    return schema::parseDisconnect(j);
}

bool paramIdFromUiName(const std::string& id, std::uint32_t& outId) {
    if (id == kUiParamMode) {
        outId = kParamMode;
        return true;
    }
    if (id == kUiParamFreqKhz) {
        outId = kParamFreqKhz;
        return true;
    }
    if (id == kUiParamLowCut) {
        outId = kParamLowCut;
        return true;
    }
    if (id == kUiParamHighCut) {
        outId = kParamHighCut;
        return true;
    }
    if (id == kUiParamAgcOn) {
        outId = kParamAgcOn;
        return true;
    }
    if (id == kUiParamAgcHang) {
        outId = kParamAgcHang;
        return true;
    }
    if (id == kUiParamAgcThresh) {
        outId = kParamAgcThresh;
        return true;
    }
    if (id == kUiParamAgcSlope) {
        outId = kParamAgcSlope;
        return true;
    }
    if (id == kUiParamAgcDecay) {
        outId = kParamAgcDecay;
        return true;
    }
    if (id == kUiParamAgcManGain) {
        outId = kParamAgcManGain;
        return true;
    }
    if (id == kUiParamVolume) {
        outId = kParamVolume;
        return true;
    }
    if (id == kUiParamMute) {
        outId = kParamMute;
        return true;
    }
    if (id == kUiParamSquelchOn) {
        outId = kParamSquelchOn;
        return true;
    }
    if (id == kUiParamSquelchThr) {
        outId = kParamSquelchThr;
        return true;
    }
    if (id == kUiParamNbOn) {
        outId = kParamNbOn;
        return true;
    }
    if (id == kUiParamNbThresh) {
        outId = kParamNbThresh;
        return true;
    }
    if (id == kUiParamNrOn) {
        outId = kParamNrOn;
        return true;
    }
    if (id == kUiParamDeempOn) {
        outId = kParamDeempOn;
        return true;
    }
    if (id == kUiParamCompOn) {
        outId = kParamCompOn;
        return true;
    }
    if (id == kUiParamWfOn) {
        outId = kParamWfOn;
        return true;
    }
    if (id == kUiParamWfSpeed) {
        outId = kParamWfSpeed;
        return true;
    }
    if (id == kUiParamWfZoom) {
        outId = kParamWfZoom;
        return true;
    }
    if (id == kUiParamWfMaxDb) {
        outId = kParamWfMaxDb;
        return true;
    }
    if (id == kUiParamWfMinDb) {
        outId = kParamWfMinDb;
        return true;
    }
    if (id == kUiParamWfComp) {
        outId = kParamWfComp;
        return true;
    }
    if (id == kUiParamArOn) {
        outId = kParamArOn;
        return true;
    }
    if (id == kUiParamOvOn) {
        outId = kParamOvOn;
        return true;
    }
    return false;
}

} // namespace netsdr