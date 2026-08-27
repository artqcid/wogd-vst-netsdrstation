#include "bridge_protocol.h"

#include "vst/common/paramids.h"

#include <cstdlib>

namespace netsdr {

namespace {

// Extracts the first quoted string inside the "data" array of a bridge
// envelope. Returns false when no such string exists.
bool extractDataId(const std::string& message, std::string& outId) {
    const std::string::size_type dataPos = message.find("\"data\":");
    if (dataPos == std::string::npos) {
        return false;
    }
    const std::string::size_type bracket = message.find('[', dataPos);
    if (bracket == std::string::npos) {
        return false;
    }
    const std::string::size_type idOpen = message.find('"', bracket);
    if (idOpen == std::string::npos) {
        return false;
    }
    const std::string::size_type idClose = message.find('"', idOpen + 1);
    if (idClose == std::string::npos) {
        return false;
    }
    outId = message.substr(idOpen + 1, idClose - idOpen - 1);
    return true;
}

} // namespace

bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out) {
    // 1. Only setParameter messages carry an id/value; bail out otherwise.
    if (message.find("\"setParameter\"") == std::string::npos) {
        return false;
    }

    // 2. Locate the data array: {"type":"setParameter","data":["<id>",<value>]}
    const std::string::size_type dataPos = message.find("\"data\":");
    if (dataPos == std::string::npos) {
        return false;
    }
    const std::string::size_type bracket = message.find('[', dataPos);
    if (bracket == std::string::npos) {
        return false;
    }

    // 3. Extract the id string (first quoted element of the array).
    const std::string::size_type idOpen = message.find('"', bracket);
    if (idOpen == std::string::npos) {
        return false;
    }
    const std::string::size_type idClose = message.find('"', idOpen + 1);
    if (idClose == std::string::npos) {
        return false;
    }
    out.id = message.substr(idOpen + 1, idClose - idOpen - 1);

    // 4. Extract the numeric value after the comma.
    const std::string::size_type comma = message.find(',', idClose);
    if (comma == std::string::npos) {
        return false;
    }
    const char* numStart = message.c_str() + comma + 1;
    char* end = nullptr;
    out.value = std::strtod(numStart, &end);
    if (end == numStart) {
        return false;
    }
    return true;
}

bool parseSetStationMessage(const std::string& message, BridgeSetStation& out) {
    if (message.find("\"setStation\"") == std::string::npos) {
        return false;
    }
    std::string hostPort;
    if (!extractDataId(message, hostPort) || hostPort.empty()) {
        return false;
    }
    out.hostPort = hostPort;
    return true;
}

bool parseDisconnectMessage(const std::string& message) {
    return message.find("\"disconnect\"") != std::string::npos;
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