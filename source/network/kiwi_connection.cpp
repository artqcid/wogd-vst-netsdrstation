#include "kiwi_connection.h"

#include "common/diag.h"
#include "util/file_logger.h"
#include <ixwebsocket/IXWebSocket.h>

#include <string>

#include <ixwebsocket/IXNetSystem.h>

#include <atomic>
#include <utility>

namespace netsdr {

class KiwiConnection::Impl {
public:
    Impl() = default;

    void setServer(const std::string& host, std::uint16_t port) {
        host_ = host;
        port_ = port;
    }

    void setCallbacks(Callbacks callbacks) {
        callbacks_ = std::move(callbacks);
    }

    bool connect() {
        // Initialize the platform net system once (WSAStartup on Windows).
        // Idempotent; without this every socket call fails on Windows.
        static const bool kNetInitOnce = [] {
            const bool ok = ix::initNetSystem();
            diagLog("kiwi_connection: ix::initNetSystem() = %d", (int)ok);
            return true;
        }();
        (void)kNetInitOnce;

        if (host_.empty()) {
            return false;
        }

        // KiwiSDR WebSocket path: /ws/kiwi/<timestamp_ms>/SND
        // The server rejects connections to "/" with HTTP 200 (no WS upgrade).
        const auto tsMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const std::string url = "ws://" + host_ + ":" + std::to_string(port_)
            + "/ws/kiwi/" + std::to_string(tsMs) + "/SND";
        diagLog("kiwi_connection connect: url=%s", url.c_str());

        // Wrap the IXWebSocket calls in try/catch: a malformed host/port must
        // never throw out of here and crash the DAW host. Treat any exception
        // as a failed connect (the UI gets the Error status instead).
        try {
            // Stop existing socket if running (for reconnect)
            socket_.stop();

            socket_.setUrl(url);

            socket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
                if (msg->type == ix::WebSocketMessageType::Open) {
                    diagLog("kiwi_connection: WebSocket OPEN");
                    connected_.store(true);
                    if (callbacks_.onOpen) {
                        callbacks_.onOpen();
                    }
                } else if (msg->type == ix::WebSocketMessageType::Message) {
                    // KiwiSDR quirk: the server sends "MSG ..." text commands
                    // using the WebSocket binary opcode. Real audio frames are
                    // raw ADPCM data that never starts with "MSG ". Route
                    // accordingly so the handshake (Phase 2) sees the MSG frames.
                    const bool isBinaryMsg = msg->binary;
                    const bool looksLikeText =
                        msg->str.size() >= 4 &&
                        msg->str[0] == 'M' &&
                        msg->str[1] == 'S' &&
                        msg->str[2] == 'G' &&
                        msg->str[3] == ' ';
                    if (isBinaryMsg && !looksLikeText && callbacks_.onBinaryMessage) {
                        diagLog("kiwi_connection: RX binary: %zu bytes", msg->str.size());
                        callbacks_.onBinaryMessage(msg->str);
                    } else if ((!isBinaryMsg || looksLikeText) && callbacks_.onTextMessage) {
                        diagLog("kiwi_connection: RX text(%s): %s",
                                isBinaryMsg ? "via-bin" : "native",
                                msg->str.substr(0, 256).c_str());
                        callbacks_.onTextMessage(msg->str);
                    }
                } else if (msg->type == ix::WebSocketMessageType::Error) {
                    diagLog("kiwi_connection: WebSocket ERROR reason=%s",
                            msg->errorInfo.reason.c_str());
                    NETSDR_LOG_INFO("WebSocket ERROR reason=%s",
                            msg->errorInfo.reason.c_str());
                    if (callbacks_.onError) {
                        callbacks_.onError();
                    }
                } else if (msg->type == ix::WebSocketMessageType::Close) {
                    diagLog("kiwi_connection: WebSocket CLOSE code=%u reason=%s remote=%d",
                            msg->closeInfo.code,
                            msg->closeInfo.reason.c_str(),
                            (int)msg->closeInfo.remote);
                    NETSDR_LOG_INFO("WebSocket CLOSE code=%u reason=%s remote=%d",
                            msg->closeInfo.code,
                            msg->closeInfo.reason.c_str(),
                            (int)msg->closeInfo.remote);
                    connected_.store(false);
                    if (callbacks_.onClose) {
                        callbacks_.onClose();
                    }
                }
            });

            socket_.disableAutomaticReconnection();
            socket_.start();
            diagLog("kiwi_connection connect: socket_.start() done");
        } catch (const std::exception& e) {
            diagLog("kiwi_connection connect: EXCEPTION %s", e.what());
            NETSDR_LOG_INFO("WebSocket connect exception: %s", e.what());
            connected_.store(false);
            return false;
        } catch (...) {
            diagLog("kiwi_connection connect: UNKNOWN EXCEPTION");
            NETSDR_LOG_INFO("WebSocket connect: unknown exception");
            connected_.store(false);
            return false;
        }
        return true;
    }

    void disconnect() {
        socket_.stop();
        callbacks_ = {};
    }

    bool isConnected() const {
        return connected_.load();
    }

    bool sendText(const std::string& text) {
        auto info = socket_.sendText(text);
        return info.success;
    }

private:
    ix::WebSocket socket_;
    Callbacks callbacks_;
    std::string host_;
    std::uint16_t port_ = netsdr::kiwiDefaultPort;
    std::atomic<bool> connected_{false};
};

KiwiConnection::KiwiConnection() : impl_(std::make_unique<Impl>()) {}

KiwiConnection::~KiwiConnection() {
    disconnect();
}

void KiwiConnection::setServer(const std::string& host, std::uint16_t port) {
    impl_->setServer(host, port);
}

void KiwiConnection::setCallbacks(Callbacks callbacks) {
    impl_->setCallbacks(std::move(callbacks));
}

bool KiwiConnection::connect() {
    return impl_->connect();
}

void KiwiConnection::disconnect() {
    impl_->disconnect();
}

bool KiwiConnection::isConnected() const {
    return impl_->isConnected();
}

bool KiwiConnection::sendText(const std::string& text) {
    return impl_->sendText(text);
}

} // namespace netsdr
