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
                   ix::WebSocket& socket, const ix::WebSocketMessagePtr& msg) {
                if (msg->type != ix::WebSocketMessageType::Message || msg->binary) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    frames_.push_back(msg->str);
                }
                // After recording, check if this is the auth frame and send audio_rate
                // to trigger Phase 2 of the KiwiClient handshake.
                if (msg->str == "SET auth t=kiwi p=") {
                    socket.sendBinary("MSG audio_rate=12000");
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
        // Note: no ix::uninitNetSystem() here — the test runner initialises the
        // net system once for the whole process (see tests/test_main.cpp).
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

    // Wait for the handshake: options, auth, AR OK, squelch, genattn, gen,
    // mod/freq, agc, keepalive (9 frames).
    REQUIRE(server.waitForFrames(9, std::chrono::seconds(5)));

    const std::vector<std::string> frames = server.frames();
    REQUIRE(frames.size() >= 9);

    // Frame order is deterministic: options -> auth -> AR OK -> squelch ->
    // genattn -> gen -> mod/freq -> agc -> keepalive.
    REQUIRE(frames[0] == "SET options=1");
    REQUIRE(frames[1] == "SET auth t=kiwi p=");
    REQUIRE(frames[2] == "SET AR OK in=12000 out=12000");
    REQUIRE(frames[3] == "SET squelch=0 max=0");
    REQUIRE(frames[4] == "SET genattn=0");
    REQUIRE(frames[5] == "SET gen=0 mix=-1");
    REQUIRE(frames[6] == "SET mod=iq low_cut=-5980 high_cut=5980 freq=14100.000");
    REQUIRE(frames[7] == "SET agc=1 hang=0 thresh=-130 slope=6 decay=1000 manGain=20");
    REQUIRE(frames[8] == "SET keepalive");

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

    // options, auth, AR OK, ident_user, squelch, genattn, gen, mod/freq, agc,
    // keepalive (10 frames).
    REQUIRE(server.waitForFrames(10, std::chrono::seconds(5)));

    const std::vector<std::string> frames = server.frames();
    REQUIRE(frames.size() >= 10);
    REQUIRE(frames[0] == "SET options=1");
    REQUIRE(frames[1] == "SET auth t=kiwi p=");
    REQUIRE(frames[2] == "SET AR OK in=12000 out=12000");
    REQUIRE(frames[3] == "SET ident_user=NetSDRStation-VST");
    REQUIRE(frames[4] == "SET squelch=0 max=0");
    REQUIRE(frames[5] == "SET genattn=0");
    REQUIRE(frames[6] == "SET gen=0 mix=-1");
    REQUIRE(frames[7] == "SET mod=iq low_cut=-5980 high_cut=5980 freq=14100.000");
    REQUIRE(frames[8] == "SET agc=1 hang=0 thresh=-130 slope=6 decay=1000 manGain=20");
    REQUIRE(frames[9] == "SET keepalive");

    client.disconnect();
}

TEST_CASE("KiwiClient: connect with empty host returns false", "[network][kiwi]") {
    KiwiClient client;
    KiwiClientConfig config; // host empty
    REQUIRE(client.connect(config) == false);
    REQUIRE(client.isConnected() == false);
}

// Test: onOpen callback fires on connection.
TEST_CASE("KiwiClient: onOpen callback fires on connection", "[network][kiwi][integration]") {
    MockServer server;

    KiwiClient client;
    std::atomic<bool> onOpenFired{false};

    KiwiClientConfig config;
    config.host = "127.0.0.1";
    config.port = server.port();
    config.userName = "";

    client.setOnOpen([&onOpenFired]() {
        onOpenFired = true;
    });

    REQUIRE(client.connect(config) == true);

    // Poll until onOpen fires or timeout.
    bool fired = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (onOpenFired.load()) {
            fired = true;
            break;
        }
    }
    REQUIRE(fired);

    REQUIRE(client.isConnected());

    client.disconnect();
}
