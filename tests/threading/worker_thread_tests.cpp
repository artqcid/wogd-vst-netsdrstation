// Unit tests for the WorkerThread (Milestone M1.3/M1.9).
// Verifies that messages posted from another thread are executed on the worker
// thread, in FIFO order, and that start/stop are idempotent and clean.

#include "catch.hpp"
#include "threading/worker_thread.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

TEST_CASE("WorkerThread: executes posted messages in FIFO order", "[threading][worker]") {
    netsdr::WorkerThread worker;
    worker.start();

    std::atomic<int> counter{0};
    std::vector<int> order;
    constexpr int kMessages = 100;

    for (int i = 0; i < kMessages; ++i) {
        worker.post([i, &order, &counter] {
            order.push_back(i);
            counter.fetch_add(1);
        });
    }

    // Wait until all messages have been processed.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (counter.load() < kMessages && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    REQUIRE(counter.load() == kMessages);
    REQUIRE(order.size() == kMessages);
    for (int i = 0; i < kMessages; ++i) {
        REQUIRE(order[static_cast<std::size_t>(i)] == i);
    }

    worker.stop();
}

TEST_CASE("WorkerThread: start/stop are idempotent and safe", "[threading][worker]") {
    netsdr::WorkerThread worker;
    worker.start();
    worker.start(); // second start is a no-op

    std::atomic<int> executed{0};
    worker.post([&executed] { executed.fetch_add(1); });

    worker.stop();
    worker.stop(); // second stop is a no-op

    REQUIRE_FALSE(worker.isRunning());
}
