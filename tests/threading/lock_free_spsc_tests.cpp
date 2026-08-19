// Unit tests for the lock-free SPSC queue (Milestone M1.3/M1.9).
// Stress test: producer writes N blocks, consumer reads N; order preserved,
// no loss, no corruption (checksum per block).

#include "catch.hpp"
#include "threading/lock_free_spsc.h"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

struct Block {
    std::uint32_t index = 0;
    std::uint32_t checksum = 0;
};

Block makeBlock(std::uint32_t index) {
    Block b;
    b.index = index;
    // A deterministic checksum so corruption is detectable.
    b.checksum = index * 2654435761u + 0x9E3779B9u;
    return b;
}

bool valid(const Block& b) {
    return b.checksum == b.index * 2654435761u + 0x9E3779B9u;
}

} // namespace

TEST_CASE("SPSC: order, no loss, no corruption under stress", "[threading][spsc]") {
    constexpr std::size_t kCount = 100000;
    netsdr::LockFreeSPSC<Block> queue(1024);

    std::atomic<bool> producerDone{false};
    std::vector<Block> received;
    received.reserve(kCount);

    std::thread producer([&] {
        for (std::uint32_t i = 0; i < kCount; ++i) {
            queue.push(makeBlock(i));
        }
        producerDone = true;
    });

    // Consumer: read until we have all blocks and the producer is done.
    std::size_t expectedIndex = 0;
    Block b;
    while (expectedIndex < kCount) {
        while (queue.pop(b)) {
            REQUIRE(valid(b));
            REQUIRE(b.index == expectedIndex); // strict order, no loss
            received.push_back(b);
            ++expectedIndex;
        }
        // Small backoff to let the producer make progress.
        if (!producerDone.load()) {
            std::this_thread::yield();
        }
    }
    producer.join();

    REQUIRE(received.size() == kCount);
}

TEST_CASE("SPSC: pop on empty queue returns false", "[threading][spsc]") {
    netsdr::LockFreeSPSC<int> queue(16);
    int out = 0;
    REQUIRE_FALSE(queue.pop(out));
}
