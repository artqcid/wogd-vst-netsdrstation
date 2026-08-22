#include "kiwi_client.h"

#include "kiwi_commands.h"

namespace netsdr {

bool KiwiClient::connect(const KiwiClientConfig& config) {
    config_ = config;
    if (config_.host.empty()) {
        return false;
    }

    connection_.setServer(config_.host, config_.port);
    connection_.setCallbacks(KiwiConnection::Callbacks{
        // Text messages from the server (e.g. MSG sample_rate=...) forwarded.
        [this](const std::string& message) {
            if (onTextMessage_) {
                onTextMessage_(message);
            }
        },
        // Binary messages are audio frames; handled by the DSP bridge (M2.4+).
        [](const std::string&) {},
        // On open: run the handshake sequence.
        [this]() {
            connection_.sendText(kiwiAuthCommand());
            const std::string ident = kiwiIdentUserCommand(config_.userName);
            if (!ident.empty()) {
                connection_.sendText(ident);
            }
            connection_.sendText(kiwiSetModFreqCommand(config_.mode, config_.lowCut,
                                                       config_.highCut, config_.freqKhz));
            connection_.sendText(kiwiAgcCommand(config_.agcOn, false, -100, 6, 1000, 50));
        },
        []() {},
        []() {},
    });

    return connection_.connect();
}

void KiwiClient::disconnect() {
    connection_.disconnect();
}

bool KiwiClient::isConnected() const {
    return connection_.isConnected();
}

bool KiwiClient::setFrequency(double freqKhz) {
    config_.freqKhz = freqKhz;
    if (!connection_.isConnected()) {
        return false;
    }
    return connection_.sendText(kiwiSetModFreqCommand(config_.mode, config_.lowCut,
                                                      config_.highCut, freqKhz));
}

bool KiwiClient::setAgc(bool on) {
    config_.agcOn = on;
    if (!connection_.isConnected()) {
        return false;
    }
    return connection_.sendText(kiwiAgcCommand(on, false, -100, 6, 1000, 50));
}

void KiwiClient::setOnTextMessage(TextMessageCallback callback) {
    onTextMessage_ = std::move(callback);
}

} // namespace netsdr
