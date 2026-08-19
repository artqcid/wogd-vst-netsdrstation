#pragma once
// Lock-free, wait-free single-producer single-consumer queue wrapper.
//
// Wraps moodycamel's ReaderWriterQueue (BSD-2-Clause, vendored in
// third_party/moodycamel) to enforce the SPSC contract and to give the audio
// thread a stable, allocation-free interface. The audio thread only calls
// try_pop(); the worker thread calls push(). No locks, no allocations on
// either side after construction.

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
