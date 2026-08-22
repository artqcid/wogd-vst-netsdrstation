#pragma once
// WebSocket client used to talk to a KiwiSDR server (Milestone M2.1).
//
// Thin, reusable wrapper around IXWebSocket (BSD-3-Clause). It owns the socket
// and a background thread (provided by IXWebSocket) and reports events via
// user-supplied callbacks. It is intentionally protocol-agnostic: the KiwiSDR
// handshake and `SET` command protocol live in a higher-level client (M2.2).
//
// Default port: 8073 is the standard KiwiSDR control port. The concrete test
// station g8ure.ddns.net uses port 8078 (port is configurable via setServer()).
//
// Threading: `connect()`, `disconnect()` and `sendText()` may be called from
// any thread. Callbacks are invoked on the IXWebSocket background thread and
// must not perform blocking work.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace netsdr {

// Default KiwiSDR WebSocket control port.
inline constexpr std::uint16_t kiwiDefaultPort{8073};

class KiwiConnection {
public:
    // Callbacks are invoked on the IXWebSocket background thread.
    using TextMessageCallback = std::function<void(const std::string&)>;
    using BinaryMessageCallback = std::function<void(const std::string&)>;
    using StateCallback = std::function<void()>;

    struct Callbacks {
        TextMessageCallback onTextMessage;
        BinaryMessageCallback onBinaryMessage;
        StateCallback onOpen;
        StateCallback onError;
        StateCallback onClose;
    };

    KiwiConnection();
    ~KiwiConnection();

    KiwiConnection(const KiwiConnection&) = delete;
    KiwiConnection& operator=(const KiwiConnection&) = delete;

    // Sets the target host (e.g. "g8ure.ddns.net") and port. The default port
    // is the standard KiwiSDR control/audio port (8073).
    void setServer(const std::string& host, std::uint16_t port);

    // Installs the event callbacks. Must be set before connect().
    void setCallbacks(Callbacks callbacks);

    // Opens the WebSocket to ws://host:port/. Returns immediately; the actual
    // open is reported asynchronously via onOpen. Returns false only if the
    // URL could not be built.
    bool connect();

    // Closes the WebSocket and stops the background thread.
    void disconnect();

    // True once the socket reached the open state (until it closes again).
    bool isConnected() const;

    // Sends a text frame. Returns false if the socket is not open.
    bool sendText(const std::string& text);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace netsdr
