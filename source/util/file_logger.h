#pragma once

#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>

namespace netsdr {

/// Two-level file logger for %TEMP%/netsdrstation.log
/// - INFO: minimal, performance-neutral, always enabled
/// - DEBUG: verbose, only in Debug builds
/// Thread-safe, auto-flush on crash.
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

    void log(Level level, const char* format, ...) {
        // DEBUG logs only in Debug builds
#ifdef NDEBUG
        if (level == DEBUG) return;
#endif

        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_) return;

        // Timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t_now);
        
        const char* levelStr = (level == INFO) ? "INFO " : "DEBUG";
        std::fprintf(file_, "[%02d:%02d:%02d.%03d] [%s] ",
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                     static_cast<int>(ms.count()), levelStr);

        // Message
        va_list args;
        va_start(args, format);
        std::vfprintf(file_, format, args);
        va_end(args);

        std::fprintf(file_, "\n");

        // Flush strategy: INFO always (critical, never lose), DEBUG throttled
        // (every 500ms) to avoid blocking the audio thread with disk I/O.
        if (level == INFO) {
            std::fflush(file_);
            lastFlush_ = now;
        } else {
            const auto sinceFlush = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastFlush_).count();
            if (sinceFlush >= 500) {
                std::fflush(file_);
                lastFlush_ = now;
            }
        }
    }

    ~FileLogger() {
        if (file_) {
            std::fclose(file_);
        }
    }

private:
    FileLogger() {
        // Write to %TEMP%/netsdrstation.log
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        std::string logPath = std::string(tempPath) + "netsdrstation.log";
        
        // Open in append mode
        fopen_s(&file_, logPath.c_str(), "a");
        
        if (file_) {
#ifdef NDEBUG
            log(INFO, "=== NetSDRStation Log Started (Release) ===");
#else
            log(INFO, "=== NetSDRStation Log Started (Debug) ===");
#endif
        }
    }

    FILE* file_ = nullptr;
    std::mutex mutex_;
    std::chrono::system_clock::time_point lastFlush_{std::chrono::system_clock::now()};
};

// Convenience macros
#define NETSDR_LOG_INFO(...) netsdr::FileLogger::instance().log(netsdr::FileLogger::INFO, __VA_ARGS__)
#define NETSDR_LOG_DEBUG(...) netsdr::FileLogger::instance().log(netsdr::FileLogger::DEBUG, __VA_ARGS__)

// Legacy compatibility (maps to INFO)
#define NETSDR_LOG(...) NETSDR_LOG_INFO(__VA_ARGS__)

} // namespace netsdr
