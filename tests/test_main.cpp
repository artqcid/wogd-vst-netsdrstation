// Catch2 test runner entry point (Catch2 v2.13.6 single-header, BSL-1.0).
//
// IXWebSocket (used by the network integration tests) needs the OS network
// stack initialized once for the whole process. On Windows this is WSAStartup
// / WSACleanup; calling WSACleanup while socket threads are still winding down
// from a previous test intermittently crashes the runner, so the net system is
// initialized here once and never torn down (the per-test initNetSystem calls
// are harmless ref-count increments).

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <ixwebsocket/IXNetSystem.h>

#ifdef _WIN32
#include <cstdlib>
#include <crtdbg.h>
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Suppress CRT assertion popups in Debug builds — failures should be
    // reported by Catch2, not by a modal dialog the user must dismiss.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    // Disable CRT debug heap leak checking at exit.  IXWebSocket leaves
    // sockets and thread-pool handles open at process teardown, which
    // triggers the CRT leak detector and pops an assertion or aborts with
    // exit code 3 — all tests actually passed, the crash is purely cosmetic.
    _CrtSetDbgFlag(0);
#endif

    ix::initNetSystem();
    return Catch::Session().run(argc, argv);
}