cmake_minimum_required(VERSION 3.25)

project(NetSDRStationVST
    VERSION 0.1.0.0
    DESCRIPTION "Generic cross-platform VST3 foundation with a sine-synth proof (Milestone M1)"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(NS_ENABLE_VST3        "Build the VST3 plugin"                    ON)
option(NS_ENABLE_TESTS       "Build and register C++ unit tests"        ON)
option(NS_ENABLE_UI          "Configure the Vue 3 UI project"           ON)
option(NS_VENDOR_VST3_SDK    "Vendor the VST3 SDK as a git submodule"   OFF)
option(NS_ENABLE_COVERAGE    "Enable code coverage (GCC/Clang only)"    OFF)

if(NS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "NS_ENABLE_COVERAGE only supported with GCC/Clang")
    endif()
endif()

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy / clangd MCP (test-strategy.md §9)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# VST3 SDK
# ---------------------------------------------------------------------------
# The VST3 SDK is an external dependency (Steinberg, MIT). Point VST3_SDK_ROOT
# at a local checkout (defaults to a sibling directory for forkability).
if(NOT DEFINED VST3_SDK_ROOT)
    if(DEFINED ENV{VST3_SDK_ROOT})
        set(VST3_SDK_ROOT "$ENV{VST3_SDK_ROOT}" CACHE PATH "Path to the Steinberg VST3 SDK checkout")
    elseif(NS_VENDOR_VST3_SDK)
        set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vst3sdk")
    else()
        message(FATAL_ERROR
            "VST3_SDK_ROOT not set.\n"
            "  - Set the VST3_SDK_ROOT environment variable, or\n"
            "  - Pass -DVST3_SDK_ROOT=<path> at configure time, or\n"
            "  - Enable NS_VENDOR_VST3_SDK to use a git submodule.")
    endif()
endif()

if(NOT VST3_SDK_ROOT OR NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "VST3 SDK not found. Set -DVST3_SDK_ROOT=<path> to the VST3 SDK "
        "(e.g. C:/Users/<you>/vst3sdk), or enable NS_VENDOR_VST3_SDK.")
endif()

# ---------------------------------------------------------------------------
# Global generator / toolchain configuration
# ---------------------------------------------------------------------------
# The VST3 SDK libraries are built with the default MSVC runtime (/MD); the
# plugin must match, so we do not override CMAKE_MSVC_RUNTIME_LIBRARY here.

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
# The VST3 SDK provides the SMTG cmake modules and the `sdk`/`validator` targets.
#
# SMTG_CREATE_PLUGIN_LINK: the SDK otherwise creates a symlink/junction to the
# standard VST3 install folder as a post-build step, which requires admin rights
# or Windows Developer Mode. Deployment is handled explicitly (see M1.10).
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "Create a link to the standard VST3 folder" FORCE)
# Keep the SDK example plugins out of every configuration (not just the presets).
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "Build VST3 SDK example plugins" FORCE)
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")

# ---------------------------------------------------------------------------
# VSTGUI (Steinberg VST3 SDK, third-party): suppress benign MSVC warnings
# ---------------------------------------------------------------------------
# The VSTGUI sources are vendored third-party code and not part of the
# NetSDRStation codebase. Under MSVC/clang-cl they emit a few warnings that
# are not actionable in our own code:
#   C4996 - deprecated onWheel API (overridden in ctextlabel.h/cviewcontainer.h)
#   C4388 - signed/unsigned comparison (cshadowviewcontainer.cpp)
#   C4267 - size_t narrowing conversion (ctexteditor.cpp)
# They are suppressed for the VSTGUI library targets only, never for our own
# sources (which stay warning-clean).
if(MSVC)
    foreach(vstgui_target IN ITEMS vstgui vstgui_support vstgui_uidescription
                                  vstgui_uiscripting vstgui_standalone
                                  VST3Inspector editorhost)
        if(TARGET ${vstgui_target})
            target_compile_options(${vstgui_target} PRIVATE /wd4996 /wd4388 /wd4267)
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# webview/webview (MIT) - OS-native WebView for the plugin editor.
# Fetched from GitHub; the editor module links webview::core_static.
# WebView2 SDK is provided via workspace folder (thirdParty/WebView2SDK).
# ---------------------------------------------------------------------------
if(NS_ENABLE_VST3)
    include(FetchContent)

    # Use local WebView2 SDK from workspace (env var or -D flag, no hard-coded default)
    if(NOT DEFINED WEBVIEW2_SDK_ROOT)
        if(DEFINED ENV{WEBVIEW2_SDK_ROOT})
            set(WEBVIEW2_SDK_ROOT "$ENV{WEBVIEW2_SDK_ROOT}" CACHE PATH "Path to WebView2 SDK")
        else()
            message(FATAL_ERROR
                "WEBVIEW2_SDK_ROOT not set.\n"
                "  - Set the WEBVIEW2_SDK_ROOT environment variable, or\n"
                "  - Pass -DWEBVIEW2_SDK_ROOT=<path> at configure time.")
        endif()
    endif()
    
    if(NOT EXISTS "${WEBVIEW2_SDK_ROOT}/build/native/include/WebView2.h")
        message(FATAL_ERROR "WebView2 SDK headers not found at ${WEBVIEW2_SDK_ROOT}/build/native/include")
    endif()
    message(STATUS "WebView2 SDK headers found at: ${WEBVIEW2_SDK_ROOT}/build/native/include")

    # Create interface library for WebView2 headers
    add_library(netsdr_webview2_headers INTERFACE)
    target_include_directories(netsdr_webview2_headers INTERFACE
        "${WEBVIEW2_SDK_ROOT}/build/native/include"
    )

    # webview library
    FetchContent_Declare(
        webview
        GIT_REPOSITORY https://github.com/webview/webview.git
        GIT_TAG        0.12.0
        GIT_SHALLOW    TRUE
    )
    set(WEBVIEW_BUILD ON CACHE INTERNAL "Build webview core")
    set(WEBVIEW_BUILD_TESTS OFF CACHE INTERNAL "")
    set(WEBVIEW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(WEBVIEW_BUILD_DOCS OFF CACHE INTERNAL "")
    set(WEBVIEW_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "")
    set(WEBVIEW_BUILD_SHARED_LIBRARY OFF CACHE INTERNAL "")
    set(WEBVIEW_INSTALL_TARGETS OFF CACHE INTERNAL "")
    set(WEBVIEW_ENABLE_PACKAGING OFF CACHE INTERNAL "")

    # Point webview to use the local WebView2 SDK headers
    set(WEBVIEW_DEPENDENCIES netsdr_webview2_headers CACHE INTERNAL "")
    # Set MSWebView2_ROOT for webview's FindMSWebView2.cmake - it looks for build/native/include under this path
    set(MSWebView2_ROOT "${WEBVIEW2_SDK_ROOT}" CACHE INTERNAL "")

    FetchContent_MakeAvailable(webview)
endif()

if(NS_ENABLE_VST3)
    add_subdirectory(source/entry)
endif()

# ---------------------------------------------------------------------------
# IXWebSocket (BSD-3-Clause) - WebSocket client library for KiwiSDR connection
# ---------------------------------------------------------------------------
# Fetch IXWebSocket regardless of NS_ENABLE_VST3 because the test target
# also needs it. Options are set to keep the build lean: no TLS, no zlib,
# no install, no demos — the KiwiSDR connection is plain ws:// and does
# not need compression or TLS.
include(FetchContent)

set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_DEMO OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.6
)
FetchContent_MakeAvailable(ixwebsocket)

add_subdirectory(source/network)

# ---------------------------------------------------------------------------
# libsamplerate (BSD-2-Clause) - high-quality sample-rate conversion.
# Converts the KiwiSDR 12 kHz / 24 kHz audio stream to the DAW sample rate.
# The library is C99; its CMake target is `samplerate` (alias
# SampleRate::samplerate). Tests are disabled to keep the build lean.
# libsamplerate 0.2.2 declares cmake_minimum_required(3.1..3.18), which CMake
# 4.x rejects; the policy floor keeps it buildable under CMake 4.
# ---------------------------------------------------------------------------
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libsamplerate
    GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
    GIT_TAG        0.2.2
)
FetchContent_MakeAvailable(libsamplerate)

# ---------------------------------------------------------------------------
# nlohmann/json (MIT) - JSON parsing for the schema-based bridge protocol
# (M4.1.5). Header-only; used by the generated validators in
# source/vst/common/generated/bridge_schema.h.
# ---------------------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Bridge protocol codegen (M4.1.5): regenerate the C++ parsers from
# schema/bridge.schema.json (single source of truth). The generated header is
# committed so builds work without Python; this target regenerates it when the
# schema changes. Invoke manually: cmake --build <dir> --target netsdrstation_bridge_codegen
# ---------------------------------------------------------------------------
set(NS_BRIDGE_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/schema/bridge.schema.json")
set(NS_BRIDGE_GENERATED "${CMAKE_CURRENT_SOURCE_DIR}/source/vst/common/generated/bridge_schema.h")
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_custom_target(netsdrstation_bridge_codegen
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/schema/generate-cpp.py"
                "${NS_BRIDGE_SCHEMA}" --output "${NS_BRIDGE_GENERATED}"
        DEPENDS "${NS_BRIDGE_SCHEMA}"
        BYPRODUCTS "${NS_BRIDGE_GENERATED}"
        COMMENT "Regenerating bridge_schema.h from schema/bridge.schema.json"
        VERBATIM)
    message(STATUS "Bridge codegen target available: netsdrstation_bridge_codegen")
else()
    message(WARNING "Python3 not found; netsdrstation_bridge_codegen unavailable (generated header is committed anyway).")
endif()

if(NS_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Vue UI build (release bundles the UI into ui/dist)
# ---------------------------------------------------------------------------
# The release editor loads the pre-built UI bundle from ui/dist (see
# source/editor/plugin_editor.cpp and NS_UI_DIST_INDEX). This custom target
# produces that bundle; build it manually before a release build:
#   cmake --build <build-dir> --target netsdrstation_ui
if(NS_ENABLE_UI)
    if(WIN32)
        find_program(NPM_EXECUTABLE NAMES npm.cmd npm)
    else()
        find_program(NPM_EXECUTABLE NAMES npm)
    endif()
    if(NPM_EXECUTABLE)
        set(NS_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui")
        set(NS_UI_DIST "${NS_UI_DIR}/dist")
        add_custom_target(netsdrstation_ui
            COMMAND ${CMAKE_COMMAND} -E chdir "${NS_UI_DIR}" ${NPM_EXECUTABLE} run build
            BYPRODUCTS "${NS_UI_DIST}/index.html"
            COMMENT "Building the Vue UI (vite build -> ui/dist)"
            VERBATIM)
    else()
        message(WARNING "npm not found; the netsdrstation_ui target is unavailable.")
    endif()
endif()
