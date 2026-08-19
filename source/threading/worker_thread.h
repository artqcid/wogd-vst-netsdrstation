#pragma once
// A dedicated worker thread that processes messages posted from other threads
// (e.g. the audio thread or the controller thread).
//
// This separates message handling from the real-time audio thread: the audio
// thread only posts a lightweight message (lock-free push) and never blocks on
// network, UI or other slow work. The worker thread runs the actual handler.

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
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

    // Posts a message to be executed on the worker thread. The caller never
    // blocks. This is the only method callable from the audio thread.
    void post(Message message);

    // True while the worker thread is running.
    bool isRunning() const { return running_.load(); }

private:
    void run();

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Message> queue_;
    std::atomic<bool> running_{false};
};

} // namespace netsdr
