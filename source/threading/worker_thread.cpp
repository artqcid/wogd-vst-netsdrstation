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
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv_.notify_all();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void WorkerThread::post(Message message) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(message));
    }
    cv_.notify_one();
}

void WorkerThread::run() {
    while (running_.load()) {
        Message message;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !running_.load() || !queue_.empty(); });
            if (!running_.load() && queue_.empty()) {
                return;
            }
            message = std::move(queue_.front());
            queue_.pop();
        }
        if (message) {
            message();
        }
    }
}

} // namespace netsdr
