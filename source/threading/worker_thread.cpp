#include "worker_thread.h"

#include <utility>

namespace netsdr {

void WorkerThread::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // already running
    }
    thread_ = std::thread(&WorkerThread::run, this);
}

void WorkerThread::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // not running
    }
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
    // Drain anything posted concurrently with stop() so its destructor runs.
    Message leftover;
    while (queue_.pop(leftover)) {
        pending_.fetch_sub(1, std::memory_order_relaxed);
        if (leftover) {
            leftover();
        }
    }
}

void WorkerThread::post(Message message) {
    // Lock-free producer side (moodycamel SPSC). The pending counter is bumped
    // before the notify so the worker's wait predicate observes the work even
    // when it is already asleep in the condition-variable wait.
    queue_.push(std::move(message));
    pending_.fetch_add(1, std::memory_order_release);
    cv_.notify_one();
}

void WorkerThread::run() {
    while (true) {
        Message message;
        // Drain everything currently queued (single consumer).
        while (queue_.pop(message)) {
            pending_.fetch_sub(1, std::memory_order_relaxed);
            if (message) {
                message();
            }
        }
        if (!running_.load(std::memory_order_acquire)) {
            return; // stopped and the queue is drained
        }
        // Idle: wait for work or a stop request. The bounded timeout prevents
        // a lost wake-up deadlock with the lock-free producer.
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(5),
                     [this] { return !running_.load(std::memory_order_acquire) ||
                                     pending_.load(std::memory_order_acquire) != 0; });
    }
}

} // namespace netsdr
