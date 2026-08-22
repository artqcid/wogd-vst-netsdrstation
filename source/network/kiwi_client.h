#pragma once
// High-level KiwiSDR client (Milestone M2.2).
//
// Owns a KiwiConnection and, on connection, runs the KiwiSDR handshake:
//   1. `SET auth t=kiwi p=`        (anonymous - no user required)
//   2. `SET ident_user=<name>`     (only when a user name is configured)
//   3. `SET mod=... freq=...`      (mode, passband, tuning frequency)
//   4. `SET agc=...`               (automatic gain control)
//
// The user name is optional; leave it empty to connect anonymously. This keeps
// the plugin usable without any user configuration (see kiwi_commands.h).

#include "kiwi_commands.h"
#include "kiwi_connection.h"

#include <cstdint>
#include <functional>
#include <string>

namespace netsdr {

struct KiwiClientConfig {
    std::string host;
    std::uint16_t port = kiwiDefaultPort;
    std::string userName;               // empty = anonymous
    std::string mode = kKiwiDefaultMode;
    int lowCut = kKiwiDefaultLowCut;
    int highCut = kKiwiDefaultHighCut;
    double freqKhz = kKiwiDefaultFreqKhz;
    bool agcOn = true;
};

class KiwiClient {
public:
    // Invoked (on the connection thread) when a text message arrives.
    using TextMessageCallback = std::function<void(const std::string&)>;

    KiwiClient() = default;

    // Opens the connection and, once open, performs the handshake. Returns
    // false if the connection could not be started (e.g. empty host).
    bool connect(const KiwiClientConfig& config);

    // Closes the underlying connection.
    void disconnect();

    bool isConnected() const;

    // Re-tunes the receiver. Sends `SET mod=... freq=...` (no-op if closed).
    bool setFrequency(double freqKhz);

    // Enables/disables AGC (no-op if closed).
    bool setAgc(bool on);

    // Installed before connect(); receives server text messages (e.g. MSG ...).
    void setOnTextMessage(TextMessageCallback callback);

private:
    KiwiConnection connection_;
    KiwiClientConfig config_;
    TextMessageCallback onTextMessage_;
};

} // namespace netsdr
