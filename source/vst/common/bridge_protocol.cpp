#include "bridge_protocol.h"

#include "vst/common/paramids.h"

#include <cstdlib>

namespace netsdr {

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

bool paramIdFromUiName(const std::string& id, std::uint32_t& outId) {
    if (id == kUiParamFreq) {
        outId = kParamFreq;
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
    return false;
}

} // namespace netsdr
