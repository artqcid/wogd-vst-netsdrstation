// Integration test for the KiwiClient handshake (Milestone M2.2).
// Spins up a local IXWebSocket server, connects with KiwiClient, and asserts
// the server receives the exact handshake frames in order (auth first, then
// optional ident_user, mod/freq, agc). No user name -> anonymous, no ident_user.

#include "catch.hpp"

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

// Records the text frames a mock server receives and signals when done.
class MockServer {
public:
    MockServer() {
        ix::initNetSystem();
        port_ = static_cast<std::uint16_t>(ix::getFreePort());
        server_ = std::make_unique<ix::WebSocketServer>(port_, "127.0.0.1");

        server_->setOnClientMessageCallback(
            [this](std::shared_ptr<ix::ConnectionState> /*state*/,
                   ix::WebSocket& /*socket*/, const ix::WebSocketMessagePtr& msg) {
                if (msg->type != ix::WebSocketMessageType::Message || msg->binary) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    frames_.push_back(msg->str);
                }
                cv_.notify_all();
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

    // Waits until at least `n` frames have been received (with timeout).
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

TEST_CASE("KiwiClient handshake: anonymous connection sends auth, mod/freq, agc",
          "[network][kiwi][integration]") {
    MockServer server;

    KiwiClient client;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = ""; // anonymous - no ident_user frame expected

    REQUIRE(client.connect(config) == true);

    // Wait for the handshake: auth, mod/freq, agc (3 frames, no ident_user).
    REQUIRE(server.waitForFrames(3, std::chrono::seconds(5)));

    const std::vector<std::string> frames = server.frames();
    REQUIRE(frames.size() >= 3);

    // Frame order is deterministic: auth -> mod/freq -> agc.
    REQUIRE(frames[0] == "SET auth t=kiwi p=");
    REQUIRE(frames[1] == "SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000");
    REQUIRE(frames[2] == "SET agc=1 hang=0 thresh=-100 slope=6 decay=1000 manGain=50");

    client.disconnect();
}

TEST_CASE("KiwiClient handshake: configured user name adds an ident_user frame",
          "[network][kiwi][integration]") {
    MockServer server;

    KiwiClient client;
    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "NetSDRStation-VST";

    REQUIRE(client.connect(config) == true);

    // auth, ident_user, mod/freq, agc (4 frames).
    REQUIRE(server.waitForFrames(4, std::chrono::seconds(5)));

    const std::vector<std::string> frames = server.frames();
    REQUIRE(frames.size() >= 4);
    REQUIRE(frames[0] == "SET auth t=kiwi p=");
    REQUIRE(frames[1] == "SET ident_user=NetSDRStation-VST");
    REQUIRE(frames[2] == "SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000");
    REQUIRE(frames[3] == "SET agc=1 hang=0 thresh=-100 slope=6 decay=1000 manGain=50");

    client.disconnect();
}

TEST_CASE("KiwiClient: connect with empty host returns false", "[network][kiwi]") {
    KiwiClient client;
    KiwiClientConfig config; // host empty
    REQUIRE(client.connect(config) == false);
    REQUIRE(client.isConnected() == false);
}
