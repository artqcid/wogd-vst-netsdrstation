#pragma once
// Minimal diagnostic logging for the NetSDRStation plugin.
//
// Appends a line to %TEMP%\netsdrstation_diag.log (one process-wide mutex) so
// the plugin's startup/UI/network path can be traced WITHOUT a debugger. Used
// to diagnose the "Connect button does not react" issue (BUG-03). Thread-safe
// and allocation-light; only ever called from non-real-time threads.
//
// Usage: #include "common/diag.h" then netsdr::diagLog("attach: ok url=%s", s);

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace netsdr {

inline void diagLog(const char* fmt, ...) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);

    char buf[2048] = {};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

#ifdef _WIN32
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");

    wchar_t tmp[MAX_PATH] = {};
    if (GetTempPathW(MAX_PATH, tmp) == 0) {
        return;
    }
    const std::wstring path = std::wstring(tmp) + L"netsdrstation_diag.log";
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"a") != 0 || f == nullptr) {
        return;
    }
    fputs(buf, f);
    fputc('\n', f);
    fclose(f);
#else
    (void)buf;
#endif
}

} // namespace netsdr
