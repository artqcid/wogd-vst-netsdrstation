#pragma once
// AudioSampleQueue: typed, lock-free SPSC queue carrying decoded audio sample
// blocks from the network thread to the DSP thread.
//
// Producer = network thread (pushes decoded blocks).
// Consumer = audio/DSP thread (pops blocks for processing).
// pop() is lock-free and allocation-free (real-time safe).
//
// Relies on LockFreeSPSC<AudioSampleBlock> from source/threading/lock_free_spsc.h.

#include "threading/lock_free_spsc.h"
#include <array>
#include <cstdint>
#include <cstddef>

namespace netsdr {

struct AudioSampleBlock {
    static constexpr std::size_t kMaxSamples = 2048;

    std::array<int16_t, kMaxSamples> samples{};
    std::size_t sampleCount = 0;   // number of valid samples
    std::uint32_t sequence = 0;    // monotonic, for order/loss detection
};

class AudioSampleQueue {
public:
    explicit AudioSampleQueue(std::size_t capacityBlocks)
        : queue_(capacityBlocks), capacityBlocks_(capacityBlocks) {}

    // Network/producer side: moves the block into the queue.
    // NOTE: may allocate/block when the queue overflows (moodycamel grows its
    // internal block chain). Prefer tryPush() on the network path.
    void push(AudioSampleBlock block) { queue_.push(std::move(block)); }

    // Bounded push (real-time-safe, allocation-free, non-blocking): drops the
    // block when the queue is at capacity. This bounds memory to a fixed limit
    // when the producer outpaces the consumer, mirroring the fixed-capacity
    // ring-buffer "drop newest when full" policy used by JUCE and other real-
    // time DSP frameworks (see FIX-19).
    void tryPush(AudioSampleBlock block) {
        if (queue_.sizeApprox() < capacityBlocks_) {
            queue_.push(std::move(block));
        }
    }

    // DSP/consumer side: lock-free, returns false when empty (graceful underflow).
    bool pop(AudioSampleBlock& out) { return queue_.pop(out); }

    // Approximate number of items currently buffered (diagnostics only).
    std::size_t sizeApprox() const { return queue_.sizeApprox(); }

private:
    LockFreeSPSC<AudioSampleBlock> queue_;
    std::size_t capacityBlocks_;
};

} // namespace netsdr