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

std::string kiwiSquelchCommand(bool on, double threshold) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET squelch=%d thresh=%.3f",
                  on ? 1 : 0, threshold);
    return buffer;
}

std::string kiwiNbCommand(bool on, double threshold) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET nb=%d thresh=%.3f",
                  on ? 1 : 0, threshold);
    return buffer;
}

std::string kiwiNrCommand(bool on) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "SET nr=%d", on ? 1 : 0);
    return buffer;
}

std::string kiwiDeempCommand(bool on) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "SET deemp=%d", on ? 1 : 0);
    return buffer;
}

std::string kiwiCompCommand(bool on) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "SET comp=%d", on ? 1 : 0);
    return buffer;
}

std::string kiwiDbugCommand(int v, int vSet, int v2, int v2Set, int dbgUs) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET dbug_v=%d,%d,%d,%d,%d",
                  v, vSet, v2, v2Set, dbgUs);
    return buffer;
}

std::string kiwiArOkCommand(int inRate, int outRate) {
    // Acknowledge the server's `audio_rate=` value and request the same rate as
    // output (no server-side resampling). The plugin's own Resampler converts
    // the source rate -> DAW rate, so the server must stream the raw source
    // rate, not resample to 44.1/48 kHz. Requesting `out=44100` here would make
    // the server resample while the plugin still assumes the `audio_rate=` rate
    // (a ~3.7x pitch error).
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET AR OK in=%d out=%d", inRate, outRate);
    return buffer;
}

std::string kiwiOptionsCommand() {
    return "SET options=1";
}

std::string kiwiSquelchMaxCommand(bool on, int max) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET squelch=%d max=%d", on ? 1 : 0, max);
    return buffer;
}

std::string kiwiGenCommand(int freq, int mix) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET gen=%d mix=%d", freq, mix);
    return buffer;
}

std::string kiwiGenAttnCommand(int attn) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "SET genattn=%d", attn);
    return buffer;
}

std::string kiwiBrowserCommand(const std::string& userAgent) {
    return "SET browser=" + userAgent;
}

} // namespace netsdr