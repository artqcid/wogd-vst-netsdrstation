# Workspace Workflow — wogd-vst-netsdrstation

_Cross-platform build, VST host debugging, and Vue UI debugging workflow.
Applies to all agents. Detailed architecture: `doc/architecture.md`; draft
plan: `doc/plan.md`; open tasks: `doc/checklist.md`._

## 1. Standing Requirements (must hold at all times)

- **R1 — Cross-platform build:** the plugin must be buildable for macOS,
  Linux and Windows at any time. On this development machine, **only Windows
  is tested for now** (mac/linux builds are kept green via CI or are verified
  on demand).
- **R2 — VST host debugging:** it must always be possible to connect the VST3
  plugin to a VST host in order to debug it (load, step through, inspect).
- **R3 — Vue UI debugging + hot reload:** it must always be possible to debug
  the Vue UI and to fix the hot-reload setup (edit UI, see change live).

## 2. Analysis: How to achieve it + technologies

### 2.1 Cross-platform build (R1)

**Technologies (all cross-platform):**

| Component | Library | Platforms |
|-----------|---------|-----------|
| Plugin framework | VST3 SDK (Steinberg) | Win / macOS / Linux |
| Build system | CMake + CMakePresets.json | Win / macOS / Linux |
| WebView | webview/webview | WebView2 / WKWebView / WebKitGTK |
| WebSocket | IXWebSocket | Win / macOS / Linux |
| Resampler | libsamplerate | Win / macOS / Linux |
| SPSC queue | moodycamel ReaderWriterQueue (header-only) | Win / macOS / Linux |
| Compiler | MSVC / Clang / GCC (C++20) | per platform |

**Strategy:**

- Single `CMakeLists.txt`; platform differences only behind thin `#ifdef`
  wrappers (WebView backend, window handle).
- `CMakePresets.json` with one configure/build/test preset per platform
  (e.g. `win-msvc`, `mac-clang`, `linux-gcc`).
- Optional CI matrix (GitHub Actions: windows / macos / ubuntu) to guarantee
  that every platform stays buildable even though only Windows is tested
  locally.

**Planned steps:**

- `S1` Scaffold `CMakeLists.txt` + `CMakePresets.json` (per-platform presets).
- `S2` Verify the Windows build end-to-end (VS 2022/2026, x64).
- `S3` Add a CI matrix (windows/macos/linux) to keep cross-builds green.
- `S4` Document per-platform prerequisites (SDKs, WebView backends).

### 2.2 VST host debugging (R2)

**Technologies:**

- **VST3PluginTestHost** (Steinberg, part of VST3 SDK) — primary Windows debug
  host. Application-style UI: scan the plugin folder, drag the plugin into the
  VST Rack, select an ASIO driver, and the plugin loads in-process.
- VST3 SDK headless tools for CI validation:
  - `public.sdk/samples/vst-hosting/editorhost` — minimal host that loads a
    plugin editor window.
  - `public.sdk/samples/vst-hosting/validator` — automated load/instantiate
    validation.
- `pluginval` (Tracktion) — cross-platform automated validation.
- Commercial DAWs (Reaper, Ableton, Cubase) for manual testing.

**Strategy:**

- VST3PluginTestHost is the everyday Windows debug host: launch it via
  `.vscode/tasks.json` (`start-testhost-debug` / `start-testhost-release`),
  then attach the Visual Studio debugger (`.vscode/launch.json` -> "Attach
  VST3PluginTestHost (Debug)"). The `.vst3` loads in-process and breakpoints hit.
- `editorhost`/`pluginval` are used in CI for headless smoke testing.
- Keep the plugin install path standard so every host finds it
  (`%COMMONPROGRAMFILES%\VST3` on Windows, `~/Library/Audio/Plug-Ins/VST3` on
  macOS, `~/.vst3` on Linux).

**Planned steps:**

- `S5` Build/obtain VST3PluginTestHost from the VST3 SDK.
- `S6` Define the "attach debugger to TestHost" workflow in .vscode/tasks.json + launch.json.
- `S7` Document how to load the built `.vst3` into the TestHost (scan folder, VST Rack).

### 2.3 Vue UI debugging + hot reload (R3)

**Technologies (adapted from `juce-projects/wogd-juce-template-gui-vue`,
but WITHOUT JUCE):**

| Component | Library | Purpose |
|-----------|---------|---------|
| UI framework | Vue 3 + TypeScript + Vite | reactive UI, HMR |
| Dev tools | vite-plugin-vue-devtools | in-browser Vue inspection |
| WebView | webview/webview (not JUCE) | C++ <-> JS bridge |
| Bridge | `src/services/pluginService.ts` | typed message passing |

**Strategy (mirrors the JUCE template's dev/prod split):**

- **Debug build:** the C++ WebView navigates to `http://localhost:5173`
  (Vite dev server) -> hot reload + browser devtools work.
- **Release build:** load the bundled static `dist/index.html` (embedded
  resource or file). **Pre-build requirement:** the `netsdrstation_ui` target
  must be built before the release VST3 build (`cmake --build <build-dir>
  --target netsdrstation_ui`). This target runs `vite build` and produces
  `ui/dist/`, which CMake copies into the VST3 bundle via POST_BUILD step
  along with `WebView2Loader.dll` and `FixedRuntime/`. No external
  dependencies are needed at runtime — all WebView2 runtime components are
  bundled inside the VST3 package.
- **Bridge abstraction:** `pluginService.ts` detects the native bridge
  (`window.vstHost`) vs. browser dev mode (mock). Native side uses
  `webview::bind()` for JS -> C++ and `webview::eval()` for C++ -> JS.

**Planned steps:**

- `S8` Scaffold the Vue 3 + Vite GUI (mirroring the template structure).
- `S9` Implement the bridge: C++ `webview::bind("vstHost.*")` +
  `webview::eval("window.updateVueState(...)")`; TS `pluginService.ts`.
- `S10` C++ WebView loads the dev server in debug, `dist/` in release.
- `S11` Verify HMR: edit a Vue component, see the change live in the plugin.

## 3. First Milestone — M0 (Sine Synth)

A VST3 synth that produces a sine tone, with a Vue UI containing:

- a **frequency knob**
- a **volume knob**
- a **mute (sound-off) button**

Components:

- **Processor:** sine oscillator (phase accumulator), 3 parameters
  (`freq`, `volume`, `mute`).
- **Edit controller:** parameter definitions (freq range 20 Hz..20 kHz,
  volume 0..1, mute on/off).
- **Editor:** `webview` WebView + Vue UI (2 knobs + 1 mute button).
- **Bridge:** knob -> `setParameter('freq'|'volume')`, button ->
  `setParameter('mute')`; C++ -> UI state sync via `updateVueState`.

## 3.5 Release Build Workflow (WebView2 Bundle)

The release build produces a self-contained VST3 bundle that works without
system-wide WebView2 or dev server:

1. Build the Vue UI: `cmake --build build/win-msvc --target netsdrstation_ui`
   (runs `vite build` → `ui/dist/`)
2. Build the VST3: `cmake --build build/win-msvc --config Release`
   POST_BUILD automatically copies into the bundle:
   - `WebView2Loader.dll` and `FixedRuntime/` (from `WEBVIEW2_SDK_ROOT`)
   - `ui/dist/` → `Contents/x86_64-win/ui/`
3. At runtime, `WebViewHost` sets `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` to
   the bundled FixedRuntime; `PluginEditor` derives the release UI URL from
   the module path (`GetModuleFileNameW`).
4. No external dependencies required — the plugin loads and renders the GUI
   on any Windows machine.

Detailed architecture: `doc/architecture.md` §8.

## 3.6 M2.10 Manual Test — UI controls the live receiver frequency

_Automated part (bridge emits `setParameter` with the correct value) is covered
by the M2.9 integration tests; this section documents the manual DAW listening
check (see `doc/checklist.md` M2.10)._

**Prerequisites for the manual check:** the full network→decode→resample→DSP
audio pipeline wired into the plugin (the M2 components exist as modules:
`KiwiConnection`, `KiwiClient`, `ImaAdpcmDecoder`, `AudioSampleQueue`,
`Resampler`, `JitterBuffer`, `KiwiBridge`; the processor integration is a
follow-up milestone).

1. Build the Vue UI and the Release VST3 (see §3.5).
2. Start VST3PluginTestHost (Debug or Release, `.vscode/tasks.json`), scan the
   plugin folder, load `NetSDRStation.vst3` into the VST Rack, choose an ASIO
   driver.
3. In the plugin UI, enter/tune a frequency (e.g. 14100 kHz) via the frequency
   control and confirm the receiver retunes:
   - automated: the bridge emits `{"type":"setParameter","data":["freq",<kHz>]}`
     → rate-limited `SET mod=... freq=...` to the server (verified by tests).
   - manual: listen in the DAW — a real KiwiSDR station signal should appear /
     change when the frequency is changed; no zipper noise during retune.
4. Confirm mute/volume still behave and no clicks/dropouts occur.

**Acceptance:** the UI frequency control changes the live receiver frequency
and the audio reflects the retune.

## 4. Reference projects (analysis only, no JUCE in this project)

- `C:\Users\marku\Documents\GitHub\artqcid\juce-projects\wogd-juce-template-gui-vue`
  — Vue 3 + TS + Vite GUI with a `pluginService.ts` bridge, Vite dev/prod
  split, WebView2 integration. Used as the **Vue pattern reference**.
- `C:\Users\marku\Documents\GitHub\artqcid\juce-projects\wogd-juce-template`
  — plugin-side WebView2 loading (dev server vs. embedded binary data) and
  `WEBVIEW2_SETUP.md`. Used as the **C++ WebView loading pattern reference**.
- `C:\Users\marku\Documents\GitHub\artqcid\juce-projects\` (vanilla/react/
  angular/svelte variants) — alternative bridge patterns; same dev/prod model.

> These are reference implementations. **This project uses webview/webview +
> VST3 SDK (no JUCE).**
