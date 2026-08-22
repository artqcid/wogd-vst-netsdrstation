#include "kiwi_commands.h"

#include <cstdio>

namespace netsdr {

std::string kiwiAuthCommand() {
    return "SET auth t=kiwi p=";
}

std::string kiwiIdentUserCommand(const std::string& name) {
    if (name.empty()) {
        return {};
    }
    return "SET ident_user=" + name;
}

std::string kiwiAgcCommand(bool on, bool hang, int thresh, int slope, int decay,
                           int manGain) {
    // Matches the server parser `SET agc=%d hang=%d thresh=%d ...` (rx_cmd.cpp).
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "SET agc=%d hang=%d thresh=%d slope=%d decay=%d manGain=%d",
                  on ? 1 : 0, hang ? 1 : 0, thresh, slope, decay, manGain);
    return buffer;
}

std::string kiwiSetModFreqCommand(const std::string& mode, int lowCut, int highCut,
                                  double freqKhz) {
    // Matches the server parser `SET mod=%s low_cut=%d high_cut=%d freq=%.3f`.
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "SET mod=%s low_cut=%d high_cut=%d freq=%.3f",
                  mode.c_str(), lowCut, highCut, freqKhz);
    return buffer;
}

} // namespace netsdr
