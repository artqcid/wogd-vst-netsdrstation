#pragma once
// KiwiSDR <-> UI JSON parameter bridge (Milestone M2.9).
//
// Bridges UI JSON parameter messages (the bridge_protocol envelope) to the
// KiWiSDR connection and echoes server state back to the UI.
//
// Threading contract:
//   - handleUiMessage() may be called from the UI/controller thread.
//   - The state callback (setOnStateUpdate) fires on the connection thread
//     (i.e. whenever the KiWiClient receives a text message from the server).
//   - The bridge itself is not internally locked: callers must use it in the
//     established single-writer / single-consumer pattern (one thread posting
//     UI messages, one thread consuming server state echoes).
//
// Bidirectional JSON bridge roundtrip (UI -> param -> server -> state echo -> UI):
//   - UI sends a JSON envelope ({"type":"setParameter","data":["freq",14100]})
//     -> the bridge parses it -> for freq, the change is rate-limited (max ~20/s)
//     -> sent to the KiWiSDR server as SET mod=... freq=....
//   - Text messages from the server (state echoes) are forwarded to the UI-facing
//     state callback.

#include "kiwi_client.h"
#include "kiwi_commands.h"
#include "dsp/rate_limiter.h"
#include "vst/common/bridge_protocol.h"

#include <functional>
#include <string>

namespace netsdr {

class KiwiBridge {
public:
    using StateCallback = std::function<void(const std::string& stateMessage)>;

    KiwiBridge();

    // Passes through to KiwiClient::connect and installs the text-message handler
    // that forwards server messages to the state callback.
    bool connect(const KiwiClientConfig& config);

    void disconnect();

    bool isConnected() const;

    // UI-facing callback for server state echoes (e.g. "MSG state freq=14100").
    // Invoked on the connection thread.
    void setOnStateUpdate(StateCallback cb);

    // Bridge entry point callable from the UI/controller thread.
    // 1. Parses the envelope with parseSetParameterMessage.
    //    If not a well-formed setParameter message, returns false.
    // 2. If the parsed id is kUiParamFreq, converts the plain Hz value to kHz
    //    (plainValue / 1000.0), then: if rateLimiter_.shouldEmit(nowSeconds) is true,
    //    calls client_.setFrequency(khz) and returns true; otherwise returns false
    //    (rate-limited, not sent). Updates internal currentFreqKhz_.
    // 3. For any other id (volume/mute/unknown), returns false (no server command;
    //    out of scope for M2.9).
    bool handleUiMessage(const std::string& envelope, double nowSeconds);

    // Last accepted frequency in kHz (for state echo / UI display).
    double currentFreqKhz() const;

private:
    KiwiClient client_;
    RateLimiter rateLimiter_{20.0};
    double currentFreqKhz_{0.0};
    StateCallback onStateUpdate_;
};

} // namespace netsdr