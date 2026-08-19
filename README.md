# NetSDRStation VST3 — Foundation (Milestone M1)

A generic, cross-platform **VST3 plugin foundation** (no project-specific
KiwiSDR code yet). Proven by a simple sine synthesizer with a Vue 3 WebView UI.
This repository is a **forkable checkpoint**: clone it as the base for a new
plugin.

## Stack (all permissive licenses)

| Component | Library | License |
|-----------|---------|---------|
| Plugin framework | Steinberg VST3 SDK | MIT |
| Build system | CMake + CMakePresets | — |
| UI | Vue 3 + Vite + TypeScript | MIT |
| WebView | webview/webview | MIT |
| Lock-free SPSC | moodycamel ReaderWriterQueue | BSD-2-Clause |
| C++ tests | Catch2 (single-header v2) | BSL-1.0 |
| UI tests | Vitest + Vue Test Utils | MIT |

See `doc/architecture.md`, `doc/plan.md`, `doc/test-strategy.md` for details.

## Prerequisites

- CMake >= 3.25
- A C++20 compiler: MSVC (Windows), AppleClang (macOS), GCC (Linux)
- Node.js >= 20 (for the UI)
- The **VST3 SDK** (`https://github.com/steinbergmedia/vst3sdk`)

## Setup

Point the build at the VST3 SDK via the `VST3_SDK_ROOT` environment variable:

```powershell
# Windows (PowerShell)
$env:VST3_SDK_ROOT = "C:\path\to\vst3sdk"

# macOS / Linux (bash)
export VST3_SDK_ROOT=/path/to/vst3sdk
```

The webview/webview library and the WebView2 SDK (Windows) are fetched
automatically during configure.

## Build & test

```powershell
# Windows (MSVC)
cmake --preset win-msvc
cmake --build build/win-msvc --config Debug
ctest --test-dir build/win-msvc -C Debug --output-on-failure
```

```bash
# macOS / Linux
cmake --preset mac-clang   # or: linux-gcc
cmake --build build/mac-clang --config Debug
ctest --test-dir build/mac-clang -C Debug --output-on-failure
```

The build produces `NetSDRStation.vst3` and runs the Steinberg **validator**
(47 tests) as a post-build step. C++ unit tests (`netsdrstation_tests`) run via
`ctest`; static analysis via `scripts/run-clang-tidy.ps1`.

### UI

```bash
cd ui
npm install
npm run test:unit   # Vitest
npm run type-check  # vue-tsc
npm run dev         # Vite dev server on http://localhost:5173 (HMR)
```

## Hot reload (HMR)

In a **Debug** build the plugin editor loads `http://localhost:5173` (the Vite
dev server). Start `npm run dev` in `ui/`, then edit a Vue component — the
change appears live in the plugin WebView. In **Release** the bundled `dist/`
is loaded instead.

## Loading the plugin in a host

The VST3 bundle is built under `build/<preset>/VST3/<Config>/NetSDRStation.vst3`.
Copy or link it to the standard VST3 folder
(`%COMMONPROGRAMFILES%\VST3` on Windows, `~/Library/Audio/Plug-Ins/VST3` on
macOS, `~/.vst3` on Linux), then load it in any VST3 host. For debugging, use
the SDK's `editorhost` or `validator` (bundled with the VST3 SDK build).

## Forking this foundation

1. Clone this repository as the base of the new plugin.
2. Rename the target (`netsdrstation`) and the package name (`NetSDRStation`)
   in `source/entry/CMakeLists.txt`, `source/entry/factory.cpp`,
   `source/entry/version.h`.
3. Replace the plugin IDs in `source/vst/common/pluginids.h`.
4. Replace the sine DSP (`source/dsp/`) with your own DSP core.
5. Update the parameter definitions in `source/vst/common/paramdefinitions.h`.

The modular layout keeps the reusable **VST shell** (entry point, processor/
controller base, parameter registry, threading, WebView editor) separate from
the **DSP core** — exactly what a fork needs to keep.
