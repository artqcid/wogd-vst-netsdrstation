#pragma once
// Two-level file logger for %TEMP%/netsdrstation.log
//   - INFO : minimal, critical events (connect/disconnect, errors) — always on
//   - DEBUG: verbose diagnostics — Debug builds only (compiled out in Release)
//
// Real-time safety: producers (including the audio thread) format their message
// into a stack buffer and publish it into a lock-free SPSC ring buffer of
// fixed-size slots. No mutex, no heap allocation, no file I/O and no clock call
// happen on the hot path. A dedicated background thread drains the ring, stamps
// the wall-clock time and writes the file (throttled flush for DEBUG).
//
// NOTE: requires <windows.h> (with WIN32_LEAN_AND_MEAN) to be included BEFORE
// this header (same contract as before; see source/network/kiwi_client.cpp).

#include <array>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>
#include <thread>

namespace netsdr {

class FileLogger {
public:
    enum Level {
        INFO,   // Critical events: connect/disconnect, errors, exceptions
        DEBUG   // Verbose: frame counts, buffer states, pipeline details
    };

    static FileLogger& instance() {
        static FileLogger logger;
        return logger;
    }

    // Lock-free, allocation-free producer. Safe to call from the audio thread.
    // Drops the message (never blocks) when the ring buffer is full.
    void log(Level level, const char* format, ...) {
#ifdef NDEBUG
        if (level == DEBUG) {
            return;
        }
#endif
        const std::size_t w = writePos_.load(std::memory_order_relaxed);
        const std::size_t r = readPos_.load(std::memory_order_acquire);
        if (w - r >= kSlotCount) {
            return;  // ring full -> drop (never block a real-time producer)
        }

        char* slot = buffer_.data() + (w % kSlotCount) * kSlotSize;
        slot[0] = (level == INFO) ? 'I' : 'D';

        std::va_list args;
        va_start(args, format);
        std::vsnprintf(slot + 1, kSlotSize - 1, format, args);
        va_end(args);
        slot[kSlotSize - 1] = '\0';

        writePos_.store(w + 1, std::memory_order_release);
    }

    ~FileLogger() {
        stop_.store(true, std::memory_order_release);
        if (thread_.joinable()) {
            thread_.join();
        }
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

private:
    FileLogger() {
        char tempPath[MAX_PATH] = {};
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            return;  // no temp dir -> logger disabled (file_ stays null)
        }
        logPath_ = std::string(tempPath) + "netsdrstation.log";
        backupPath_ = std::string(tempPath) + "netsdrstation.log.1";
        fopen_s(&file_, logPath_.c_str(), "a");
        if (file_ == nullptr) {
            return;  // cannot open -> logger disabled
        }
        rotateIfNeeded(true);  // rotate if existing log is already too large
#ifdef NDEBUG
        writeLine(INFO, "=== NetSDRStation Log Started (Release) ===");
#else
        writeLine(INFO, "=== NetSDRStation Log Started (Debug) ===");
#endif
        thread_ = std::thread([this] { loggerLoop(); });
    }

    // Background consumer: drains the ring, stamps time, writes the file.
    void loggerLoop() {
        auto lastFlush = std::chrono::steady_clock::now();
        for (;;) {
            bool wroteInfo = false;
            std::size_t w = writePos_.load(std::memory_order_acquire);
            std::size_t r = readPos_.load(std::memory_order_relaxed);
            while (r < w) {
                const char* slot = buffer_.data() + (r % kSlotCount) * kSlotSize;
                if (file_) {
                    writeLine((slot[0] == 'I') ? INFO : DEBUG, slot + 1);
                    if (slot[0] == 'I') {
                        wroteInfo = true;
                    }
                }
                readPos_.store(r + 1, std::memory_order_release);
                ++r;
            }

            // Flush: INFO always (critical, never lose), DEBUG throttled (500 ms).
            const auto now = std::chrono::steady_clock::now();
            const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   now - lastFlush).count();
            if (file_ && (wroteInfo || since >= 500)) {
                std::fflush(file_);
                lastFlush = now;
                rotateIfNeeded(false);  // check size cap after flush
            }

            if (stop_.load(std::memory_order_acquire)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    // Rotates the log file when it exceeds kMaxLogBytes. Must run only on the
    // logger thread or in the constructor — never on the producer (log()) path.
    // When startup == true we are in the constructor and skip the INFO rotation
    // message (the start header is written right after this call).
    bool rotateIfNeeded(bool startup) {
        if (!file_) {
            return false;
        }
        std::fflush(file_);
        const long long size = _ftelli64(file_);
        if (size < 0 || static_cast<std::size_t>(size) <= kMaxLogBytes) {
            return false;
        }

        // Current log is too large: rotate.
        const char* logPath = logPath_.c_str();
        const char* backupPath = backupPath_.c_str();

        std::fclose(file_);
        file_ = nullptr;

        // Remove any existing backup (overwrite).
        DeleteFileA(backupPath);

        // Rename current log -> backup.
        if (!MoveFileA(logPath, backupPath)) {
            // Rename failed (file may have been deleted/moved); try std::rename.
            std::rename(logPath, backupPath);
        }

        // Reopen a fresh log.
        fopen_s(&file_, logPath, "a");
        if (file_ == nullptr) {
            return false;  // logging disabled
        }

        if (!startup) {
            writeLine(INFO, "=== NetSDRStation Log Rotated (size cap) ===");
        }
        return true;
    }

    // Writes one timestamped `[HH:MM:SS.mmm] [LEVEL] message` line. Only called
    // from the constructor (once) and the logger thread (never the hot path).
    void writeLine(Level level, const char* message) {
        if (!file_) {
            return;
        }
        const auto now = std::chrono::system_clock::now();
        const auto t = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) % 1000;

        struct tm timeinfo {};
        localtime_s(&timeinfo, &t);

        const char* levelStr = (level == INFO) ? "INFO " : "DEBUG";
        std::fprintf(file_, "[%02d:%02d:%02d.%03d] [%s] %s\n",
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                     static_cast<int>(ms.count()), levelStr, message);
    }

    static constexpr std::size_t kSlotCount = 2048;
    static constexpr std::size_t kSlotSize = 256;
    static constexpr std::size_t kMaxLogBytes = 10 * 1024 * 1024;  // 10 MB cap

    std::array<char, kSlotCount * kSlotSize> buffer_{};
    std::atomic<std::size_t> writePos_{0};  // producer-only
    std::atomic<std::size_t> readPos_{0};   // consumer-only
    std::atomic<bool> stop_{false};
    std::thread thread_;
    std::FILE* file_ = nullptr;
    std::string logPath_;
    std::string backupPath_;
};

// Convenience macros
#define NETSDR_LOG_INFO(...) \
    netsdr::FileLogger::instance().log(netsdr::FileLogger::INFO, __VA_ARGS__)

// DEBUG is a compile-time no-op in Release so the audio thread does zero work.
#ifdef NDEBUG
#define NETSDR_LOG_DEBUG(...) ((void)0)
#else
#define NETSDR_LOG_DEBUG(...) \
    netsdr::FileLogger::instance().log(netsdr::FileLogger::DEBUG, __VA_ARGS__)
#endif

// Legacy compatibility (maps to INFO)
#define NETSDR_LOG(...) NETSDR_LOG_INFO(__VA_ARGS__)

} // namespace netsdr
