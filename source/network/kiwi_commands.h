#pragma once
// KiwiSDR WebSocket control-command serialization (Milestone M2.2).
//
// The KiwiSDR WebSocket protocol (reference: jks-prv/kiwiclient and the KiwiSDR
// server rx/rx_cmd.cpp) is a set of ASCII `SET ...` text frames. This header
// holds the pure-C++ serializers so the command layer can be unit-tested
// without any network or VST3 dependency.
//
// A session is opened anonymously: `SET auth t=kiwi p=` needs no user name and
// no password. A user name (`SET ident_user=`) is entirely optional and is only
// sent when explicitly configured - a VST user never has to enter one.

#include <cstdint>
#include <string>

namespace netsdr {

// Default KiwiSDR modulation mode and passband used when none is configured.
inline constexpr const char* kKiwiDefaultMode = "am";
inline constexpr int kKiwiDefaultLowCut = -4900;
inline constexpr int kKiwiDefaultHighCut = 4900;

// Default tuning frequency in kHz (international shortwave broadcast band).
inline constexpr double kKiwiDefaultFreqKhz = 14100.0;

// Authenticates as an anonymous "kiwi" client. Requires no user name and no
// password, so the plugin works out of the box without any user input.
std::string kiwiAuthCommand();

// Optionally identifies the connection with a user name. Returns an empty
// string when `name` is empty (the caller then omits this frame entirely).
std::string kiwiIdentUserCommand(const std::string& name);

// Enables/disables AGC. `on`, `hang`, `thresh` (dB), `slope` (dB),
// `decay` (ms) and `manGain` (dB) map directly to the KiwiSDR `SET agc=...`
// parameters.
std::string kiwiAgcCommand(bool on, bool hang, int thresh, int slope, int decay,
                           int manGain);

// Sets the modulation mode, passband and tuning frequency in a single command.
// `freqKhz` is the baseband frequency in kHz, formatted with 3 decimals.
std::string kiwiSetModFreqCommand(const std::string& mode, int lowCut, int highCut,
                                  double freqKhz);

// Periodic keepalive frame; the server drops idle connections without it.
inline std::string kiwiKeepaliveCommand() { return "SET keepalive"; }

} // namespace netsdr
