#pragma once
// KiwiSDR WebSocket control-command serialization (Milestone M2.2, M3.2).
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
inline constexpr const char* kKiwiDefaultMode = "iq";
inline constexpr int kKiwiDefaultLowCut = -5980;
inline constexpr int kKiwiDefaultHighCut = 5980;

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

// Squelch: `SET squelch=<0|1> thresh=<0..1>`.
std::string kiwiSquelchCommand(bool on, double threshold);

// Noise blanker: `SET nb=<0|1> thresh=<0..1>`.
std::string kiwiNbCommand(bool on, double threshold);

// Noise reduction: `SET nr=<0|1>`.
std::string kiwiNrCommand(bool on);

// De-emphasis: `SET deemp=<0|1>`.
std::string kiwiDeempCommand(bool on);

// Compressor: `SET comp=<0|1>`.
std::string kiwiCompCommand(bool on);

// Phase-1 command: debug verbosity (sent immediately on OPEN).
std::string kiwiDbugCommand(int v, int vSet, int v2, int v2Set, int dbgUs);

// Marks the connection as external (non-local). The KiwiSDR server requires
// this to be sent BEFORE `SET auth` (reference kiwiclient/client.py open()).
std::string kiwiOptionsCommand();

// Auth ACK: acknowledges the server's audio rate and requests the audio stream
// at that same rate (no server-side resampling; the plugin's own Resampler
// converts 12 kHz -> DAW rate). `inRate`/`outRate` must equal the `audio_rate=`
// value advertised by the server (do not hard-code 12000: some Kiwis differ).
std::string kiwiArOkCommand(int inRate, int outRate);

// Server-side squelch gate: `SET squelch=<0|1> max=<0..1>` (reference format).
std::string kiwiSquelchMaxCommand(bool on, int max);

// Signal generator off: `SET gen=<freq> mix=<mix>` (0/-1 disables the generator).
std::string kiwiGenCommand(int freq, int mix);

// Signal generator attenuation: `SET genattn=<dB>`.
std::string kiwiGenAttnCommand(int attn);

// Phase-2 command: identify the browser/client to the server.
std::string kiwiBrowserCommand(const std::string& userAgent);

// Periodic keepalive frame; the server drops idle connections without it.
inline std::string kiwiKeepaliveCommand() { return "SET keepalive"; }



} // namespace netsdr