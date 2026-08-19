#pragma once
// A dedicated worker thread that processes messages posted from other threads
// (e.g. the audio thread or the controller thread).
//
// The producer side (post) is lock-free: it enqueues into a moodycamel SPSC
// queue and only pokes the worker condition variable, so it never blocks on a
// mutex and is safe to call from the real-time audio thread. Only the worker
// thread uses the mutex (to wait for work).

#include "lock_free_spsc.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>

namespace netsdr {

class WorkerThread {
public:
    using Message = std::function<void()>;

    WorkerThread() = default;
    ~WorkerThread() { stop(); }

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    // Starts the worker thread. No-op if already running.
    void start();

    // Stops the worker thread and drains pending messages. Safe to call
    // multiple times; idempotent.
    void stop();

    // Posts a message to be executed on the worker thread. Lock-free on the
    // producer side (no mutex) and non-blocking: safe to call from the audio
    // thread.
    void post(Message message);

    // True while the worker thread is running.
    bool isRunning() const { return running_.load(); }

private:
    void run();

    LockFreeSPSC<Message> queue_{1024};
    std::thread thread_;
    std::mutex mutex_;                    // only guards the condition-variable wait
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::atomic<std::size_t> pending_{0}; // messages awaiting processing
};

} // namespace netsdr
