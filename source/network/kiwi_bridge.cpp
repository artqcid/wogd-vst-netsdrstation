#include "kiwi_bridge.h"

#include "network/kiwi_client.h"
#include "vst/common/bridge_protocol.h"

#include <algorithm>

namespace netsdr {

KiwiBridge::KiwiBridge()
    : rateLimiter_(20.0) {
}

bool KiwiBridge::connect(const KiwiClientConfig& config) {
    // Start the underlying KiWiClient connection and handshake.
    if (!client_.connect(config)) {
        return false;
    }

    // Install a text-message handler that forwards server messages to the
    // UI-facing state callback.  The KiWiClient's onTextMessage_ is empty by
    // default (no-op in the base connect), so we can safely install our own
    // callback after connect() without interfering with the handshake.
    client_.setOnTextMessage(
        [this](const std::string& message) {
            if (onStateUpdate_) {
                onStateUpdate_(message);
            }
        });

    return true;
}

void KiwiBridge::disconnect() {
    client_.disconnect();
}

bool KiwiBridge::isConnected() const {
    return client_.isConnected();
}

void KiwiBridge::setOnStateUpdate(StateCallback cb) {
    onStateUpdate_ = std::move(cb);
}

bool KiwiBridge::handleUiMessage(const std::string& envelope, double nowSeconds) {
    // 1. Parse the envelope as a setParameter message.
    BridgeSetParameter parsed;
    if (!parseSetParameterMessage(envelope, parsed)) {
        return false;
    }

    // 2. Only the "freq" parameter is handled in M2.9; all other ids are out of scope.
    if (parsed.id != kUiParamFreq) {
        return false;
    }

    // 3. The UI freq value is already in kHz (KiwiSDR `SET freq=` convention).
    double khz = parsed.value;

    // 4. Rate-limit the update.  If allowed, send to the KiWiSDR server
    //    and remember the new frequency.
    if (rateLimiter_.shouldEmit(nowSeconds)) {
        client_.setFrequency(khz);
        currentFreqKhz_ = khz;
        return true;
    }

    // Rate-limited: do not send, do not update currentFreqKhz_.
    return false;
}

double KiwiBridge::currentFreqKhz() const {
    return currentFreqKhz_;
}

} // namespace netsdr