#pragma once
// High-level KiwiSDR client (Milestone M2.2, extended M3.2).
//
// Owns a KiwiConnection and, on connection, runs the KiwiSDR handshake:
//   1. `SET auth t=kiwi p=`        (anonymous - no user required)
//   2. `SET ident_user=<name>`     (only when a user name is configured)
//   3. `SET mod=... freq=...`      (mode, passband, tuning frequency)
//   4. `SET agc=...`               (automatic gain control)
//
// The user name is optional; leave it empty to connect anonymously. This keeps
// the plugin usable without any user configuration (see kiwi_commands.h).
//
// Binary messages (SND audio frames) are forwarded to an installable callback;
// the DSP layer decodes them (see source/vst/processor/plugin_processor.cpp).
// Keepalive is sent after each SND frame to prevent the server from closing
// the connection after ~5 seconds of inactivity (matches kiwiclient.py reference).

#include "kiwi_commands.h"
#include "kiwi_connection.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

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
    // Audio-stream sample rate in Hz (KiwiSDR narrowband 12 kHz, wideband 24 kHz).
    double sampleRate = 12000.0;
};

class KiwiClient {
public:
    // Invoked (on the connection thread) when a text message arrives.
    using TextMessageCallback = std::function<void(const std::string&)>;
    // Invoked (on the connection thread) when a binary message (SND frame) arrives.
    using BinaryMessageCallback = std::function<void(const std::string&)>;
    // Invoked (on the connection thread) on connection state changes.
    using StateCallback = std::function<void()>;

    KiwiClient() = default;
    ~KiwiClient();

    // Opens the connection and, once open, performs the handshake. Returns
    // false if the connection could not be started (e.g. empty host).
    bool connect(const KiwiClientConfig& config);

    // Closes the underlying connection.
    void disconnect();

    bool isConnected() const;

    // Re-tunes the receiver. Sends `SET mod=... freq=...` (no-op if closed).
    bool setFrequency(double freqKhz);

    // Updates mode + passband + frequency in a single frame (no-op if closed).
    bool setTuning(const std::string& mode, int lowCut, int highCut, double freqKhz);

    // Enables/disables AGC with the full parameter set (no-op if closed).
    bool setAgc(bool on, bool hang, int thresh, int slope, int decay, int manGain);

    // Audio processing toggles (no-op if closed).
    bool setSquelch(bool on, double threshold);
    bool setNb(bool on, double threshold);
    bool setNr(bool on);
    bool setDeemp(bool on);
    bool setComp(bool on);

    // Installed before connect(); receives server text messages (e.g. MSG ...).
    void setOnTextMessage(TextMessageCallback callback);
    // Installed before connect(); receives binary SND frames for audio decode.
    void setOnBinaryMessage(BinaryMessageCallback callback);
    // Installed before connect(); called on connection state changes.
    void setOnOpen(StateCallback cb);
    void setOnError(StateCallback cb);
    void setOnClose(StateCallback cb);

private:
    KiwiConnection connection_;
    KiwiClientConfig config_;
    TextMessageCallback onTextMessage_;
    BinaryMessageCallback onBinaryMessage_;
    StateCallback onOpen_;
    StateCallback onError_;
    StateCallback onClose_;

    // True once the server's `audio_rate=` MSG triggered phase 2 (SET AR OK
    // etc.). Read by the keepalive thread, written by the network/reconnect
    // threads, so it must be atomic.
    std::atomic<bool> handshakePhase2Done_{false};

    // Keepalive throttling: send at most once per second (matches kiwirecorder.py).
    std::atomic<std::int64_t> lastKeepaliveSecs_{0};

    // Log the keepalive once per connection (avoids a 1 Hz DEBUG log flood).
    std::atomic<bool> keepaliveLogged_{false};

    // Reconnect logic with exponential backoff (max 3 attempts).
    void scheduleReconnect();
    void reconnectLoop();
    std::thread reconnectThread_;
    std::atomic<bool> reconnectRunning_{false};
    std::atomic<bool> destroying_{false}; // Set in destructor to prevent new threads
    std::atomic<int> reconnectAttempts_{0};
    static constexpr int kMaxReconnectAttempts = 3;

    // Background keepalive timer: sends `SET keepalive` at 1 Hz independent of
    // incoming audio frames. Prevents the server's inactivity timeout from
    // firing whenever the host / audio thread briefly pauses SND-frame delivery
    // (in a VST host the process/network thread can be starved, stopping the
    // keepalive that is normally sent in the SND binary callback — FIX-40).
    void keepaliveLoop();
    std::thread keepaliveThread_;
    std::atomic<bool> keepaliveRunning_{false};

    void handleTextMessage(const std::string& message);
    void sendKeepaliveThrottled();
};

} // namespace netsdr