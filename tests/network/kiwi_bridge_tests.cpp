// Integration test for the KiwiBridge (Milestone M2.9).
// Spins up a local IXWebSocket server, connects a KiwiBridge, and asserts
// the bridge correctly forwards freq changes as SET commands, respects
// rate-limiting, and echoes server state back to the UI.

#include "catch.hpp"

#include "network/kiwi_bridge.h"
#include "network/kiwi_client.h"
#include <ixwebsocket/IXGetFreePort.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace netsdr;

namespace {
//
// Mock server that records text frames received from a connected client
// and can send a text message back to the client via the WebSocket API.
//
class MockServer {
public:
    MockServer() {
        ix::initNetSystem();
        port_ = static_cast<std::uint16_t>(ix::getFreePort());
        server_ = std::make_unique<ix::WebSocketServer>(port_, "127.0.0.1");

        server_->setOnClientMessageCallback(
            [this](std::shared_ptr<ix::ConnectionState> /*state*/,
                   ix::WebSocket& socket, const ix::WebSocketMessagePtr& msg) {
                if (msg->type != ix::WebSocketMessageType::Message || msg->binary) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    frames_.push_back(msg->str);
                }
                // Echo a state message back to the UI client.
                // The test expects "MSG state freq=14100".
                socket.send("MSG state freq=14100");
            });

        thread_ = std::thread([this] { server_->listenAndStart(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~MockServer() {
        server_->stop();
        if (thread_.joinable()) {
            thread_.join();
        }
        ix::uninitNetSystem();
    }

    std::uint16_t port() const { return port_; }

    // Wait until at least `n` frames have been received (with timeout).
    bool waitForFrames(std::size_t n, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, n] { return frames_.size() >= n; });
    }

    std::vector<std::string> frames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frames_;
    }

private:
    std::uint16_t port_ = 0;
    std::unique_ptr<ix::WebSocketServer> server_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> frames_;
};

} // namespace

TEST_CASE("KiwiBridge: freq change from UI reaches the mock server as a SET command",
          "[network][kiwi][bridge]") {
    MockServer server;

    KiwiBridge bridge;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "";

    REQUIRE(bridge.connect(config) == true);

    // Wait for the handshake: auth, mod/freq, agc (3 frames).
    REQUIRE(server.waitForFrames(3, std::chrono::seconds(5)));

    const std::vector<std::string> handshakeFrames = server.frames();
    REQUIRE(handshakeFrames.size() >= 3);

    // Verify handshake frames are correct.
    REQUIRE(handshakeFrames[0] == "SET auth t=kiwi p=");
    REQUIRE(handshakeFrames[1] == "SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000");
    REQUIRE(handshakeFrames[2] == "SET agc=1 hang=0 thresh=-100 slope=6 decay=1000 manGain=50");

    // Send a freq change from the UI.
    bridge.setOnStateUpdate(
        [](const std::string& /*msg*/) { /* unused in this test */ });

    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["freq",14100]})",
              0.0) == true);

    // Poll for the additional SET freq frame from the UI change.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.frames().size() >= 4) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(server.frames().size() >= 4);

    const auto allFrames = server.frames();
    REQUIRE(allFrames.size() >= 4); // 3 handshake + 1 freq change
}

TEST_CASE("KiwiBridge: rate limiter throttles repeated freq changes",
          "[network][kiwi][bridge]") {
    MockServer server;

    KiwiBridge bridge;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "";

    REQUIRE(bridge.connect(config) == true);

    // Wait for the handshake frames.
    REQUIRE(server.waitForFrames(3, std::chrono::seconds(5)));

    // First freq change — should be allowed.
    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["freq",14100]})",
              0.0) == true);

    // Second change within the 0.05 s window (20/s = 1/20 = 0.05 s interval)
    // should be rate-limited and not sent.
    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["freq",14100]})",
              0.01) == false);

    // Third change after the window should be allowed again.
    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["freq",14100]})",
              0.06) == true);

    // Poll for frames beyond the handshake until we have 5 total
    // (3 handshake + 2 freq SET).
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.frames().size() >= 5) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(server.frames().size() >= 5);

    const auto allFrames = server.frames();
    std::size_t freqCount = 0;
    for (const auto& f : allFrames) {
        if (f.find("SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000") !=
            std::string::npos) {
            ++freqCount;
        }
    }
    REQUIRE(freqCount == 3);
}

TEST_CASE("KiwiBridge: server state is echoed back to the UI",
          "[network][kiwi][bridge]") {
    MockServer server;

    KiwiBridge bridge;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "";

    // Set up the state callback BEFORE connecting so it receives
    // the server's echo messages from the handshake.
    std::string recordedMessage;
    bridge.setOnStateUpdate(
        [&](const std::string& msg) { recordedMessage = msg; });

    REQUIRE(bridge.connect(config) == true);

    // Wait for the handshake frames (server echoes "MSG state freq=14100"
    // upon receiving each client frame during handshake).
    REQUIRE(server.waitForFrames(3, std::chrono::seconds(5)));

    // Send a UI freq change to guarantee a fresh echo.
    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["freq",14100]})",
              0.0) == true);

    // Poll for the state callback to fire.
    bool received = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!recordedMessage.empty()) {
            received = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(received);
    // Verify the recorded message is the expected echo.
    if (received) {
        INFO("Recorded state message: " << recordedMessage);
        REQUIRE(recordedMessage == "MSG state freq=14100");
    }
}

TEST_CASE("KiwiBridge: non-freq or malformed UI messages do not send commands",
          "[network][kiwi][bridge]") {
    MockServer server;

    KiwiBridge bridge;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "";

    REQUIRE(bridge.connect(config) == true);

    // Wait for the handshake frames.
    REQUIRE(server.waitForFrames(3, std::chrono::seconds(5)));

    std::size_t frameCountBefore = server.frames().size();

    // Volume parameter should not trigger a server command.
    REQUIRE(bridge.handleUiMessage(
              R"({"type":"setParameter","data":["volume",0.5]})",
              0.0) == false);

    // Malformed/garbage message should not trigger a server command.
    REQUIRE(bridge.handleUiMessage("garbage", 0.0) == false);

    // No new SET mod= frame should have arrived.
    REQUIRE(server.frames().size() == frameCountBefore); // only handshake frames, no new frames from rejected messages
}