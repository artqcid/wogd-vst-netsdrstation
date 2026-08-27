#include "catch.hpp"

#include "network/kiwi_connection.h"
#include <ixwebsocket/IXGetFreePort.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <string>
#include <thread>

using namespace netsdr;

// ---------------------------------------------------------------------------
// TEST: connect() with empty host returns false (no crash)
// ---------------------------------------------------------------------------
TEST_CASE("KiwiConnection connects to empty host returns false [short]",
          "[kiwi_connection]")
{
    KiwiConnection conn;
    // Default host is empty, connect() should return false
    REQUIRE(conn.connect() == false);
    REQUIRE(conn.isConnected() == false);
}

// ---------------------------------------------------------------------------
// TEST: KiwiConnection connects to a local mock KiwiSDR server
// ---------------------------------------------------------------------------
TEST_CASE("KiwiConnection connects to local mock KiwiSDR server [integration]",
          "[kiwi_connection][integration]")
{
    // Bind to an ephemeral free port so the test never collides with other
    // sockets on the machine.
    const std::uint16_t port = static_cast<std::uint16_t>(ix::getFreePort());

    // --- Mock KiwiSDR server (runs in its own thread) ---
    // IXWebSocket on Windows requires initNetSystem() before socket use
    ix::initNetSystem();
    ix::WebSocketServer server(port, "127.0.0.1");

    server.setOnConnectionCallback(
        [](std::weak_ptr<ix::WebSocket> weakSocket,
           std::shared_ptr<ix::ConnectionState> /*connectionState*/) {
            // Client connected – register a message callback so the
            // server doesn't terminate the connection immediately
            if (auto socket = weakSocket.lock()) {
                socket->setOnMessageCallback(
                    [](const ix::WebSocketMessagePtr& /*msg*/) {
                        // Message received – ignored
                    });
            }
        });

    // Start the server in a background thread
    std::thread serverThread([&server]() {
        server.listenAndStart();
    });

    // Small delay to ensure the server is bound before we connect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // --- KiwiConnection client ---
    KiwiConnection conn;
    std::promise<void> open_promise;
    std::future<void> open_future = open_promise.get_future();

    conn.setServer("127.0.0.1", port);
    conn.setCallbacks(KiwiConnection::Callbacks{
        [](const std::string&){},                    // onTextMessage – unused
        [](const std::string&){},                    // onBinaryMessage – unused
        [&](){ open_promise.set_value(); },          // onOpen
        [&](){},                                     // onError
        [&](){}                                      // onClose
    });

    // Connect
    REQUIRE(conn.connect() == true);

    // Wait for onOpen with timeout (prevents hanging forever)
    constexpr auto timeout = std::chrono::seconds(5);
    REQUIRE(open_future.wait_for(timeout) == std::future_status::ready);
    REQUIRE(conn.isConnected() == true);

    // Send a text frame (verify sendText succeeds)
    REQUIRE(conn.sendText("SET test=1") == true);

    // Clean shutdown
    conn.disconnect();

    // Stop server and join thread
    server.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
    // Note: no ix::uninitNetSystem() here — the test runner initialises the
    // net system once for the whole process (see tests/test_main.cpp).
}