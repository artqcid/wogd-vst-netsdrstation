#include "kiwi_connection.h"

#include <ixwebsocket/IXWebSocket.h>

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
        if (host_.empty()) {
            return false;
        }
        socket_.setUrl("ws://" + host_ + ":" + std::to_string(port_) + "/");

        socket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                connected_.store(true);
                if (callbacks_.onOpen) {
                    callbacks_.onOpen();
                }
            } else if (msg->type == ix::WebSocketMessageType::Message) {
                if (msg->binary && callbacks_.onBinaryMessage) {
                    callbacks_.onBinaryMessage(msg->str);
                } else if (!msg->binary && callbacks_.onTextMessage) {
                    callbacks_.onTextMessage(msg->str);
                }
            } else if (msg->type == ix::WebSocketMessageType::Error) {
                if (callbacks_.onError) {
                    callbacks_.onError();
                }
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                connected_.store(false);
                if (callbacks_.onClose) {
                    callbacks_.onClose();
                }
            }
        });

        socket_.start();
        return true;
    }

    void disconnect() {
        socket_.stop();
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
