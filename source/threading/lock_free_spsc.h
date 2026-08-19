#pragma once
// Lock-free, wait-free single-producer single-consumer queue wrapper.
//
// Wraps moodycamel's ReaderWriterQueue (BSD-2-Clause, vendored in
// third_party/moodycamel) to enforce the SPSC contract and to give the audio
// thread a stable, non-blocking consumer interface.
//
// Real-time guarantees:
//   - pop() (audio/consumer side) is lock-free and allocation-free.
//   - push() (worker/producer side) is lock-free but MAY allocate if the queue
//     grows beyond its initial capacity; size the queue large enough up front
//     (see the constructor) so the producer never reallocates in practice.

#include "third_party/moodycamel/readerwriterqueue.h"

#include <cstddef>

namespace netsdr {

template <typename T>
class LockFreeSPSC {
public:
    explicit LockFreeSPSC(std::size_t capacity) : queue_(capacity) {}

    // Worker (producer) side.
    void push(const T& item) { queue_.enqueue(item); }
    void push(T&& item) { queue_.enqueue(std::move(item)); }

    // Audio (consumer) side. Returns false when empty.
    bool pop(T& out) { return queue_.try_dequeue(out); }

    // Approximate number of items currently buffered (for diagnostics only).
    std::size_t sizeApprox() const { return queue_.size_approx(); }

private:
    moodycamel::ReaderWriterQueue<T> queue_;
};

} // namespace netsdr
