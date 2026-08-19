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
    if(NS_VENDOR_VST3_SDK)
        set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vst3sdk")
    else()
        set(VST3_SDK_ROOT "" CACHE PATH "Path to the Steinberg VST3 SDK checkout")
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
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")

# ---------------------------------------------------------------------------
# webview/webview (MIT) - OS-native WebView for the plugin editor.
# Fetched from GitHub; the editor module links webview::core_static.
# ---------------------------------------------------------------------------
if(NS_ENABLE_VST3)
    include(FetchContent)
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

    # On Windows webview compiles against the WebView2 SDK headers. Point it at
    # a local WebView2 SDK (vendor) or rely on the webview default discovery.
    if(WIN32 AND DEFINED WEBVIEW2_SDK_ROOT AND EXISTS "${WEBVIEW2_SDK_ROOT}/include/WebView2.h")
        add_library(netsdr_webview2_headers INTERFACE)
        target_include_directories(netsdr_webview2_headers INTERFACE "${WEBVIEW2_SDK_ROOT}/include")
        set(WEBVIEW_DEPENDENCIES netsdr_webview2_headers CACHE INTERNAL "")
    endif()

    FetchContent_MakeAvailable(webview)
endif()

if(NS_ENABLE_VST3)
    add_subdirectory(source/entry)
endif()

if(NS_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
