---
type: Implementation Plan
title: M3 Implementation Plan — Integration & Ship
description: Step-by-step plan for M3: pipeline wiring, full parameter set, RT safety, manual acceptance
status: done
generated:
  by: human:marku
  at: 2026-08-22
verified:
  by: human:marku
  at: 2026-08-27
tags: [m3, integration, pipeline, rt-safety, deployment]
resource: doc/architecture.md doc/checklist.md
---

# M3 Implementation Plan — Integration & Ship

_Cross-reference: `doc/checklist.md` M3 · `doc/architecture.md` §5–§8 ·
`doc/ui-architecture.md` §3–§4 · `doc/coding-standards.md`_

## Overview

M3 wires the fully unit-tested M2 components into the plugin. After M3 the
shipped `.vst3` connects to a real KiwiSDR server, decodes the audio, and
exposes the full parameter set to the DAW. The M1 sine oscillator is retired.

**Prerequisite state:**
- All M2 components are built and tested in isolation (`netsdr_network`,
  `netsdr_dsp`, `netsdr_threading` library targets).
- The plugin (`netsdrstation`) links only `netsdr_vst`, `netsdr_webview`,
  `webview::core_static` — the network/DSP libraries are **not yet linked**.
- `source/vst/common/paramdefinitions.h` contains only the three M1 params
  (`kParamFreq`, `kParamVolume`, `kParamMute`).

---

## Chronological implementation order

### Step 1 — M3.2: Full parameter model (do first, all other steps depend on it)

**Why first?** Every subsequent step (processor, UI, commands) must reference
the complete parameter IDs and ranges. Define them before writing any wiring
code.

#### 1a. Extend `paramids.h`

Add all 28 KiwiSDR parameters. Suggested grouping:

```
// Core
kParamStation    // host:port string index (or direct string param — see note)
kParamMode       // enum 0..17 (AM, AMN, AMW, USB, USN, LSB, LSN, CW, CWN,
                 //             NBFM, NNFM, IQ, DRM, SAM, SAU, SAL, SAS, QAM)
kParamFreqKhz    // double, 0.001 .. 30000.0 kHz
kParamLowCut     // int, Hz, -8000 .. 0
kParamHighCut    // int, Hz, 0 .. 8000

// AGC
kParamAgcOn      // bool (stepCount=1)
kParamAgcHang    // bool (stepCount=1)
kParamAgcThresh  // int dB, -130 .. 0
kParamAgcSlope   // int dB, 0 .. 10
kParamAgcDecay   // int ms, 20 .. 5000
kParamAgcManGain // int dB, 0 .. 120

// Audio
kParamVolume     // 0.0 .. 1.0 (already exists, keep same ID)
kParamMute       // bool (already exists)
kParamSquelchOn  // bool (stepCount=1)
kParamSquelchThr // 0.0 .. 1.0
kParamNbOn       // bool (stepCount=1)
kParamNbThresh   // 0.0 .. 1.0
kParamNrOn       // bool (stepCount=1)
kParamDeempOn    // bool (stepCount=1)
kParamCompOn     // bool (stepCount=1)

// Display / Waterfall
kParamWfOn       // bool (stepCount=1)
kParamWfSpeed    // enum 0..3 (Pause, Slow, Med, Fast)
kParamWfZoom     // int 0..14
kParamWfMaxDb    // int dBFS, -10 .. 0
kParamWfMinDb    // int dBFS, -160 .. -60
kParamWfComp     // bool (stepCount=1) — CIC compensation
kParamArOn       // bool — aperture auto-range
kParamOvOn       // bool — spectrum overlap
```

> **Note on `kParamStation`:** VST3 parameters are `double`-typed; a host:port
> string cannot be a parameter. Two options:
> - **Option A (recommended):** Store the active station URL as plugin state
>   (serialized in `getState`/`setState`) but NOT as a VST3 parameter. The UI
>   sends a `setStation` message (not a `setParameter` message) via the bridge.
>   This is the cleanest architecture.
> - **Option B:** Maintain an indexed list of known stations; `kParamStation`
>   is a 0-based index. Simpler DAW automation, but breaks if the list changes.
>
> Use **Option A** for M3; Option B (presets/bookmarks) can be added in M5.

#### 1b. Extend `paramdefinitions.h`

Create `ParameterDefinition` entries for all new params. Keep the existing
`kParamFreq` (sine synth legacy) **or** replace with `kParamFreqKhz` — do not
keep both. Cleanest: remove `kParamFreq` (Hz) and replace with `kParamFreqKhz`
(kHz, range 0.001–30000). Update `processor_state.h` accordingly.

#### 1c. Resolve FIX-35 — O(1) registry lookup

Replace the `std::vector` linear scan in `ParameterRegistry` with an
`std::unordered_map<uint32_t, size_t>` index built in the constructor. With 28
parameters, O(n) is measurable on the audio thread during automation sweeps.

**Files:**
- `source/vst/common/paramids.h`
- `source/vst/common/paramdefinitions.h`
- `source/vst/common/parameter_registry.h/.cpp`
- `source/vst/common/processor_state.h`
- `source/vst/controller/plugin_controller.cpp` (re-register params)
- `source/network/kiwi_commands.h/.cpp` (add squelch/NB/NR command builders)

**Tests:**
- Unit test: all 28 params register, correct range/default/step count.
- Unit test: `toNormalized`/`toPlain` roundtrip for each group.
- Unit test: O(1) lookup (insert 1000 params, measure iteration count == 1).

---

### Step 2 — M3.1: Processor integration (audio pipeline)

Wire the M2 pipeline into `PluginProcessor`. This is the core of M3.

#### 2a. Link `netsdr_network` into the plugin

In `source/entry/CMakeLists.txt`, add `netsdr_network` and `netsdr_dsp` to
`target_link_libraries(netsdrstation ...)`. This is a one-line CMake change but
triggers a full recompile.

#### 2b. Add owned pipeline members to `PluginProcessor`

```cpp
// source/vst/processor/plugin_processor.h
std::unique_ptr<netsdr::KiwiClient>        kiwiClient_;
std::unique_ptr<netsdr::ImaAdpcmDecoder>   adpcmDecoder_;
netsdr::AudioSampleQueue                   audioQueue_;
std::unique_ptr<netsdr::Resampler>         resampler_;
netsdr::JitterBuffer                       jitterBuffer_;
netsdr::ParameterSmoother                  freqSmoother_;
netsdr::RateLimiter                        freqLimiter_;
```

All members are initialized in `initialize()`, torn down in `terminate()`.

#### 2c. Lifecycle wiring

| VST3 call | Action |
|-----------|--------|
| `initialize()` | Construct all pipeline members; call `kiwiClient_->connect(host, port)` with default station; start keepalive timer on the network thread. |
| `setupProcessing()` | Set `resampler_.setRatio(kiwiSampleRate / setup.sampleRate)`; configure `jitterBuffer_` target. |
| `process()` | Pull from `jitterBuffer_` (non-blocking); if underflow, output silence; copy to output buffers. Apply `freqSmoother_` per sample. Apply `volume_` and `mute_`. |
| `terminate()` | `kiwiClient_->disconnect()`; destructor order guarantees cleanup. |

> **Alternative for connection management:** instead of connecting in
> `initialize()` (which runs before the host has a UI), consider connecting
> when the station parameter changes (bridge `setStation` message). This way the
> plugin is silent until the user picks a station — cleaner UX for M3, required
> for M5 anyway.

#### 2d. Remove `SineOscillator` from `process()`

Replace `oscillator_.render(...)` with the pipeline pull. Keep
`SineOscillator` as a class (it is tested) but stop using it in the processor.

#### 2e. Audio format bridge

KiwiSDR sends **12 kHz** (standard) or **24 kHz** (wideband) IMA ADPCM mono.
The `Resampler` converts to DAW sample rate. The `JitterBuffer` sits between
ADPCM decode and resample:

```
Network thread:
  KiwiClient --SND frame--> ImaAdpcmDecoder --PCM int16--> AudioSampleQueue

Audio thread (process()):
  AudioSampleQueue --pop--> JitterBuffer --pull--> Resampler --output--> DAW
```

Make sure `ImaAdpcmDecoder` outputs `float` or convert int16→float before the
queue push (avoids format conversion on the audio thread).

**Files:**
- `source/vst/processor/plugin_processor.h/.cpp`
- `source/entry/CMakeLists.txt`

**Tests:**
- Integration test: `MockKiwiServer` sends `SND` frames → processor produces
  audio → Goertzel peak at expected frequency.
- Integration test: underflow → silence output (no crash).
- Real-time audit: clang-tidy `cppcoreguidelines-avoid-non-const-global-variables`,
  custom checker for `new`/`std::mutex` in `process()`.

---

### Step 3 — M3.3: UI — KiwiSDR controls (replace M1 knobs)

Replace the M1 sine-synth Vue UI with a minimal but functional KiwiSDR control
set. This is NOT the full M4 parity — just enough to exercise the M3.1
pipeline.

#### 3a. Controls to implement in M3

| Control | Vue component | Bridge message |
|---------|---------------|----------------|
| Station input (host:port text) | `<input>` / `StationInput.vue` | `setStation` |
| Frequency (kHz) | `NumberInput.vue` + step buttons | `setParameter("freqKhz", val)` |
| Mode select | `<select>` (18 modes) | `setParameter("mode", idx)` |
| Low Cut / High Cut | `NumberInput.vue` | `setParameter("lowCut/highCut", val)` |
| AGC on/off | `Toggle.vue` | `setParameter("agcOn", 0|1)` |
| Volume | `Slider.vue` | `setParameter("volume", val)` |
| Mute | `MuteButton.vue` (already exists) | `setParameter("mute", 0|1)` |
| S-Meter | `SMeter.vue` (display only) | `onMessage("level", val)` |
| Connection status | `StatusBadge.vue` | `onMessage("status", ...)` |

#### 3b. Bridge extension

Add `setStation` handling in `plugin_editor.cpp`:
- Parse `{"type":"setStation","data":["host:port"]}`.
- Call a new `PluginProcessor::setStation(host, port)` method (thread-safe:
  post via `workerThread_`).

#### 3c. Style

Use the KiwiSDR dark-grey colour scheme (`#222`/`#333` background, white text,
green accent `#4CAF50`). This will be refined in M4 but establishes the
visual identity early.

**Files:**
- `ui/src/views/PluginView.vue` (replace content)
- `ui/src/components/` (new: `NumberInput.vue`, `Toggle.vue`, `Slider.vue`,
  `StationInput.vue`, `SMeter.vue`, `StatusBadge.vue`)
- `ui/src/services/pluginService.ts` (add `setStation`, `onLevel`, `onStatus`)
- `source/editor/plugin_editor.cpp` (add `setStation` dispatch)
- `source/vst/processor/plugin_processor.h/.cpp` (add `setStation`)

**Tests:**
- Vitest: each component emits the correct value on interaction.
- Vitest: `pluginService.setStation` sends the correct bridge envelope.
- Integration: bridge roundtrip for `setStation` → `KiwiClient::connect` (mock).

---

### Step 4 — M3.4: Real-time safety audit + performance

Run the full audit **after** M3.1 and M3.3 are both wired, so the audit covers
the actual production code path.

#### Checklist

| Check | Tool | Target |
|-------|------|--------|
| No `new`/`delete` in `process()` | clang-tidy `cppcoreguidelines-no-malloc` | 0 violations |
| No `std::mutex` lock in `process()` | clang-tidy custom check or code review | 0 violations |
| No blocking I/O in `process()` | code review | confirmed |
| SPSC queue capacity pre-allocated | verify `AudioSampleQueue` constructor | capacity ≥ 4096 frames |
| Jitter buffer tuning | manual test at `g8ure.ddns.net:8078` | 100–150 ms, no dropouts |
| Resampler CPU | perf measurement at 128-sample buffer | < 5% CPU |

#### Alternative: use LLVM SAN

If clang-tidy is insufficient, compile with `ThreadSanitizer` and `AddressSanitizer`
(`-fsanitize=thread,address`) in the `win-analyze` preset (extend `CMakePresets.json`).
Run the integration test suite under TSAN — any lock-ordering issue or race in
the SPSC path surfaces immediately.

**Files:**
- `CMakePresets.json` (add `win-tsan` preset)
- `source/vst/processor/plugin_processor.cpp` (fix any violations found)

---

### Step 5 — M3.5: Manual acceptance

Load plugin in `VST3PluginTestHost` (Release build):
1. Enter station `g8ure.ddns.net:8078` in the StationInput.
2. Set mode AM, freq 7100 kHz.
3. Confirm audio audible in DAW output.
4. Change frequency via UI → live re-tune without dropout.
5. DAW automation on `kParamFreqKhz` → frequency follows the automation lane.

Document result in `doc/workspace-workflow.md §3.6`.

---

### Step 6 — M3.6: Dev infrastructure (T1, T2)

#### T1 — clangd MCP

The `win-clangd` preset (added in FIX-23) already produces `compile_commands.json`.
Install the `clangd-mcp` or equivalent semantic C++ MCP server and verify it
resolves symbols across all `netsdr_*` targets.

> **Status (2026-08-22):** `lsp-mcp-server` (MIT) installed globally and
> registered in `opencode.json` as `clangd_mcp`. It bridges to `clangd
> --background-index` for C/C++ and discovers the project via
> `compile_commands.json` (rootPatterns). The `win-clangd` preset was fixed to
> configure under Ninja by using **clang-cl** as the compiler (VSTGUI's
> `vstgui_standalone` target requires the MSVC flag; plain clang++ fails with
> "No SOURCES given"). `SMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF` avoids the
> VST3Inspector build failure. `compile_commands.json` is symlinked into the
> workspace root.

#### T2 — Playwright MCP

Install Playwright (`npm install -D @playwright/test`) and the Playwright MCP
server. Write a smoke test:
- Start the Vite dev server.
- Open `http://localhost:5173` in Playwright headless browser.
- Assert the station input, frequency field, and mode select are visible.

This catches UI regressions without loading the VST in a host.

> **Status (2026-08-22):** `@playwright/test` installed (devDependency),
> `ui/playwright.config.ts` + `ui/e2e/smoke.spec.ts` created. Chromium
> headless-shell downloaded. Smoke test green (asserts station input,
> frequency label and mode select via `getByTestId`/`getByLabel`).
> Run with `npm run test:e2e` from `ui/`.

**Files:**
- `ui/e2e/smoke.spec.ts` (new)
- `ui/playwright.config.ts` (new)
- `package.json` (add `@playwright/test`)

---

### Step 7 — M3.7: Documentation + knowledge sync

1. Update `doc/architecture.md` §8 with the actual audio pipeline diagram.
2. Document all 28 parameters in `doc/architecture.md` (or a new
   `doc/parameter-reference.md`).
3. Mark M3 done in `doc/plan.md` and `doc/checklist.md`.
4. License audit (L2): IXWebSocket (BSD-3), libsamplerate (BSD-2),
   moodycamel ReaderWriterQueue (BSD-2) — all already cleared in M2. No new
   dependencies in M3 → L2 satisfied.
5. `index_project_code` → RAG/wiki sync.
6. NotebookLM: update **NetSDRStation-VST** notebook with M3-complete status.

---

## Post-M3 Defects (surfaced 2026-08-25)

These defects block M3.5 (manual acceptance) and must be fixed before the
TestHost can be started at all.

### BUG-04 — Clean-Build failure: Winsock include-order conflict in `netsdr_network`

**Symptom:** `cmake --build build/win-msvc --config Debug` after a clean
configure fails in `netsdr_network.vcxproj`:
```
C2011: "sockaddr": "struct" Typneudefinition
C2011: "fd_set": "struct" Typneudefinition
...100+ errors
```
**Consequence:** The `.vst3` bundle is never produced → `build\win-msvc\VST3\Debug\`
does not exist → `start-testhost-debug.ps1:37` exits with "Debug plugin folder
not found" → TestHost never starts.

**Root cause — include order in `kiwi_connection.cpp`:**

```
#include "common/diag.h"           // → <windows.h> WITHOUT WIN32_LEAN_AND_MEAN
                                   //   → winsock.h pulled in (defines sockaddr etc.)
#include <ixwebsocket/IXNetSystem.h> // → WIN32_LEAN_AND_MEAN + <winsock2.h>
                                   //   → re-defines sockaddr → C2011
```

`diag.h` (added during BUG-03 diagnostic instrumentation, 2026-08-24) includes
`<windows.h>` at line 17 without `WIN32_LEAN_AND_MEAN`. This was hidden during
incremental builds because `kiwi_connection.obj` was already cached. After a
clean build it surfaces.

**Fix (choose one):**

- **Option A — minimal:** Add a guard to `source/common/diag.h` before
  `#include <windows.h>`:
  ```cpp
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  ```

- **Option B — clean (recommended):** Replace `<windows.h>` in `diag.h` with
  `<debugapi.h>`. This header exposes only `OutputDebugStringA` /
  `IsDebuggerPresent` — no Winsock definitions, no conflict possible:
  ```cpp
  #include <debugapi.h>   // instead of <windows.h>
  ```

**Verification:** After the fix, `cmake --build build/win-msvc --config Debug`
must complete with `Build succeeded`. Then `Test-Path "build\win-msvc\VST3\Debug"`
must return `True`.

_File: `source/common/diag.h:17`_
_Cross-reference: `doc/checklist.md` BUG-04_

---

### BUG-05 — `start-testhost-debug` task has no `dependsOn: ["build-debug"]`

**Symptom:** Running the `start-testhost-debug` task in VSCode starts the Vite
dev server (the terminal shows the Vite output) and then immediately checks
whether `build\win-msvc\VST3\Debug` exists. If the plugin is not built (e.g.
after a clean or BUG-04), the script exits silently — the user only sees Vite
running and no TestHost window appears ("nur cmd für vite").

**Root cause:** `.vscode/tasks.json` `"start-testhost-debug"` (lines 57–68) has
no `"dependsOn"` entry. The analogous `"build-debug-clean"` task (lines 42–55)
uses `"dependsOn": ["clean","build-release"]` with `"dependsOrder": "sequence"`
as the correct pattern.

**Fix:**
```json
{
  "label": "start-testhost-debug",
  "type": "shell",
  "command": "& '${workspaceFolder}\\.vscode\\start-testhost-debug.ps1'",
  "dependsOn": ["build-debug"],
  "dependsOrder": "sequence",
  ...
}
```
Apply the same fix to `"start-testhost-release"` (add
`"dependsOn": ["build-release"]`).

_File: `.vscode/tasks.json:57–68`_
_Cross-reference: `doc/checklist.md` BUG-05_

---

### BUG-03 — remaining open point: IConnectionPoint path unverified

The BUG-03 fix (see checklist) addressed the primary root cause
(`ix::initNetSystem()`) and the secondary causes (no status feedback). One
**unverified secondary cause** remains:

> The `setStation` message is sent via `PluginController::setStation` →
> `IConnectionPoint::sendMessage` → `PluginProcessor::notify`. If the host
> does NOT connect controller↔processor via `IConnectionPoint::connect`, the
> message is silently dropped and `connectToStation` is never called.

This path is exercised by `TEST-10` (processor receives `setStation` message)
but the **end-to-end controller→processor IMessage delivery in a real host** is
untested. In VST3PluginTestHost the host must connect the two components
automatically — but if BUG-04 is not fixed and the plugin bundle does not exist,
this cannot be verified manually.

**Action:** After BUG-04 is fixed and the TestHost starts, check the diagnostic
log (`%TEMP%\netsdrstation_diag.log`) to confirm the sequence:
1. `plugin_editor: onJavaScriptMessage type=setStation` → station received.
2. `plugin_controller: setStation host=... port=...` → controller got it.
3. `plugin_processor: notify NetSDRStation:SetStation` → processor got the message.
4. `kiwi_connection: ix::initNetSystem() = 1` → net system initialized.
5. `kiwi_connection: WebSocket OPEN` or `ERROR` → connection attempt result.

If step 3 is missing, the `IConnectionPoint` is not wired by the host.

---

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| KiwiSDR `SND` frame format differs from implementation | Medium | Add a hex-dump integration test that decodes a captured real frame. |
| Audio underruns at 128-sample buffer | Medium | Tune jitter buffer; add a configurable pre-fill parameter. |
| WebSocket reconnection on disconnect | Medium | Implement exponential backoff in `KiwiClient` (defer to M3.4 or M4). |
| 28 params cause DAW parameter list to be unwieldy | Low | Group params into VST3 parameter groups (categories) in `plugin_controller.cpp`. |
| Resampler introduces audible latency | Low | Use `SRC_SINC_FASTEST` quality for live monitoring; `SRC_SINC_BEST_QUALITY` for export (expose as a param). |

---

## Implementation status (2026-08-22)

> This section records the actual state after the first M3 implementation pass.
> It is kept separate from the chronological plan above so the plan stays a
> stable reference while the status reflects reality.

### Per step

| Step | Task | State |
|------|------|-------|
| 1 | M3.2 full parameter model | ✅ done |
| 2 | M3.1 processor pipeline integration | ✅ done |
| 3 | M3.3 UI (KiwiSDR controls) | ✅ done |
| 4 | M3.4 real-time safety audit | ✅ done (known defect fixed, see below) |
| 5 | M3.5 manual acceptance (real KiwiSDR) | ✅ handshake verified (FIX-41); GUI-host test = user |
| 6 | M3.6 dev infrastructure (clangd/Playwright MCP) | ✅ done |
| 7 | M3.7 docs + knowledge sync | ✅ done |

- C++ unit + integration tests: build green, 86 test cases (676k assertions).
- M3.1 integration test (`tests/vst/plugin_processor_pipeline_tests.cpp`): mock
  KiwiSDR server → decode → resample → Goertzel peak at 1000 Hz. **No longer
  flaky** (see "RT-safety fixes" below). Stress-verified: 30/30 runs green.
- UI: Vitest 28/28 green, `vue-tsc` clean, `vite build` green,
  Playwright smoke test green (M3.6 T2).
- Release + Debug VST3 builds green.

### Technical deviations from the plan (and why)

1. **Pipeline order — jitter buffer is AFTER the resampler, not before.**
   - Plan (Step 2e): `AudioSampleQueue --pop--> JitterBuffer --pull--> Resampler`.
   - Implemented: `AudioSampleQueue --pop--> Resampler --> JitterBuffer --> out`,
     with the jitter buffer running at the **DAW sample rate** (not the 12 kHz
     source rate).
   - Why: pulling exactly `numSamples` per `process()` block is simpler and
     robust; the resampler always receives all available source input instead
     of being drip-fed by the jitter buffer. Functionally equivalent cushioning.
2. **Connection timing — lazy, not on `initialize()`.**
   - Plan (Step 2c, primary): "Start WebSocket on `initialize()`".
   - Implemented: connect only when a station is set (via `setState`/`applyState`
     or a `setStation` bridge message) — i.e. the plan's "Alternative".
   - Why: (a) hermetic unit tests (no real network on plugin construction),
     (b) no surprise network I/O on load, (c) matches M5's "no station loaded by
     default" UX.
3. **Parameter count — 27 VST3 params + station-as-state, not 28.**
   - The plan lists 28 with `kParamStation` as a parameter (Option B).
   - Implemented Option A (station as plugin state + `setStation` message), as
     the plan itself recommends. 27 DAW-automatable params remain.
4. **`setStation` transport — VST3 `IMessage`, not a direct method call.**
   - Plan (Step 3b): "Call a new `PluginProcessor::setStation(...)`".
   - Implemented: editor → `PluginController::setStation` → `IConnectionPoint`
     `sendMessage` (`"NetSDRStation:SetStation"` / `"HostPort"` attr) →
     `PluginProcessor::notify()` → worker thread `connectToStation()`.
   - Why: the editor/controller have **no direct pointer** to the processor in
     VST3; `IMessage` over the host connection proxy is the standard channel.
5. **Mock server for the M3.1 test needed unplanned fixes.**
   - The IXWebSocket server-side socket is not sendable from the connection
     callback (fires before `connectToSocket`); the test polls `getClients()`
     instead. The ADPCM encoder had to be **stateful across frames** (a
     per-frame reset glitched the decoder at every boundary).

### Known defect (production, surfaced by the flaky test) — FIXED

The M3.1 integration test occasionally reported a Goertzel magnitude < 0.05
(output dominated by silence). This had two independent root causes, both
fixed:

1. **Real-time safety violations in the production audio path:**
   - `JitterBuffer::push/pull` used `std::vector` `insert`/`erase` (allocation +
     O(n) shifting) **on the audio thread**.
   - `Resampler::process` used `std::vector` `insert`/`resize`/`std::move`
     **on the audio thread**.
   - **Fix (2026-08-22):**
     - `JitterBuffer` is now a **pre-allocated fixed-capacity ring buffer**
       (head index + count, wrap-around memcpy, drop-oldest on overflow). All
       allocation happens in the constructor; `push`/`pull`/`reset` are
       allocation-free and lock-free. `source/dsp/jitter_buffer.h/.cpp`.
     - `Resampler` now uses a **bounded staging buffer** (`kMaxStagingFrames =
       4096`, reserved once in the constructor) with in-place `memmove`
       compaction; inputs larger than the staging capacity are processed in an
       internal chunked loop. `process()`/`reset()` never reallocate.
       `source/dsp/resampler.h/.cpp`.
     - The jitter buffer's pre-fill gate is now a **start latch**: it only
       gates the first pull; once audio has started, pull() returns whatever is
       available (0 on underflow) instead of re-arming the 100 ms target
       mid-stream. This removes bursts of silence under CPU contention
       (JUCE-style "always fill").
     - Unit tests updated to the new latch semantics
       (`tests/dsp/jitter_buffer_tests.cpp`).

2. **Test artifact — mock server frame wrap corrupts the ADPCM stream:**
   - The mock `SineStreamServer` sent the SAME ~46 frames cyclically. The IMA
     ADPCM decoder is stateful (predictor + step index); after the wrap (frame
     45 → frame 0) the decoder state does not match the replayed frame, so the
     decoded signal contains periodic glitches. Measured impact: Goertzel
     magnitude drops from 31819 (continuous) to 7468 (−77%).
   - **Fix:** the mock now encodes a 100 s sine (1200 k samples → 585 frames);
     the sender delivers only ~250 frames during the whole test, so no wrap
     ever occurs.

Both fixes together make the M3.1 pipeline test deterministic: 30/30 stress
runs green, full suite 86/86 green.

### Fixes already applied during this pass

- **Use-after-free / SIGSEGV** in `PluginProcessor` teardown: the worker thread
  could still run a posted `sendPendingParams()` that dereferenced `kiwiClient_`
  after it was freed. Fixed by an explicit `PluginProcessor::~PluginProcessor()`
  that calls `worker_.stop()` + `kiwiClient_->disconnect()` before members are
  destroyed.
- **Unbounded queue growth**: `AudioSampleQueue` used moodycamel `enqueue`,
  which reallocates when over capacity (FIX-19). Added `tryPush` (drop-newest at
  capacity) and used it on the network path — bounds memory to a fixed limit.
- **Test runner net-system init**: `ix::initNetSystem` is now called once in
  `tests/test_main.cpp` (CATCH_CONFIG_RUNNER); per-test `uninitNetSystem` calls
  removed to avoid `WSACleanup` racing with still-running socket threads.

### Known defect (FIXED 2026-08-24, surfaced by M3.5 manual acceptance) — BUG-03

**Symptom:** clicking the `Connect` button in `StationInput` has no visible
effect. The `StatusBadge` stays on `"Connecting..."`; neither a successful
connection nor an error is ever shown.

**Root cause (PRIMARY, found in 2nd analysis pass) — net system never initialized:**

The plugin never calls `ix::initNetSystem()`. On Windows this performs
`WSAStartup`; without it every IXWebSocket `socket()`/`connect()` fails with
`WSANOTINITIALISED`, so `socket_.start()` silently fails and the connection is
never established. The unit/integration tests pass only because
`tests/test_main.cpp` calls `ix::initNetSystem()` once for the whole test
process — but the real plugin DLL runs inside the host process, where nobody
initializes the net system. This explains "connect does nothing" in BOTH debug
and release.

**Root cause (SECONDARY) — no status feedback channel back to the UI, errors swallowed:**

1. `PluginView.vue` `onStation()` sets `status = 'Connecting...'` and calls
   `pluginService.setStation(hostPort)`. In native mode there is no code path
   that ever updates `status` again; the only branch that sets
   `'Connected (dev)'` runs when `!isInNative()` (Vite dev server).
2. The C++ side never emits a `{"type":"status",...}` message. In
   `connectToStation` (`plugin_processor.cpp:418-419`) the text-message
   callback is a no-op (`/* M3: text echoes unused */`); no hook reports the
   connection state to the UI.
3. Errors are silently swallowed: `KiwiClient::connect`
   (`kiwi_client.cpp:37-50`) wires `onError`/`onClose` as `[]() {}` no-ops and
   `onOpen` only runs the handshake. `KiwiClient` exposes no
   `onOpen`/`onError`/`onClose` hooks (only `setOnTextMessage`/
   `setOnBinaryMessage`, `kiwi_client.h:72-75`). Any unreachable station
   (wrong port, blocked network, server down) therefore produces zero visible
   output — "nothing happens".
4. (Secondary, unverified) The `setStation` transport uses VST3 `IMessage`
   (`PluginController::setStation` → `sendMessage` → `PluginProcessor::notify`,
   `plugin_controller.cpp:107-117`, `plugin_processor.cpp:161-177`). This
   end-to-end path has no test coverage (TEST-10 only asserts the no-op when no
   `PluginController` is present). If a host does not connect
   controller↔processor via `IConnectionPoint::connect`, `sendMessage` silently
   drops the message and `connectToStation` is never called — an alternative
   cause of "nothing happens".

**Fix (implemented 2026-08-24):**

1. **`KiwiConnection::Impl::connect()` now calls `ix::initNetSystem()` once**
   (static idempotent guard; no `uninitNetSystem` in the plugin)
   (`kiwi_connection.cpp:5,25-32`). **This is the actual fix** for "connect
   does nothing".
2. `KiwiClient` gained `setOnOpen`/`setOnError`/`setOnClose` (`StateCallback`,
   `kiwi_client.h:45,79-81`) and `connect()` now invokes them from the
   `KiwiConnection::Callbacks` (`kiwi_client.cpp:37-63`).
3. `PluginProcessor::connectToStation` wires the hooks
   (open→"Connected", error→"Error", close→"Disconnected") and emits
   "Connecting" before `connect()`. `emitStatus()` marshals to the worker
   thread, fires the `setOnStatus` test hook AND forwards a VST3 `IMessage`
   `"NetSDRStation:Status"` to the controller peer
   (`plugin_processor.cpp:415-432,691-712`).
4. `PluginController::notify()` receives the status message and forwards the
   string via a `statusSink_` (`plugin_controller.cpp:110-124,138-140`).
5. `PluginEditor` registers the sink (ctor) / clears it (dtor) and calls
   `pushStatus()` → `webView_.eval("window.updateVueState({...status...})")`
   (`plugin_editor.cpp:95-98,106-111,254-266`). No UI change required (the Vue
   UI already handled `status` messages).
6. Errors are no longer swallowed: `onError`/`onClose` are wired to the UI.

**Tests (added, green):** `KiwiClient: onOpen callback fires on connection`
(kiwi_client_tests.cpp); `PluginProcessor: status reports Connecting then
Connected when station connects` and `status reports Error for an unreachable
station` (plugin_processor_pipeline_tests.cpp). Debug+Release ctest green,
VST3 validator 47/47 (Debug+Release). Manual VST3PluginTestHost verification
(M3.5) still pending.

---

## Critical M3 Blocker — FIX-41 (n_snd=0, no SND audio frames) — RESOLVED

> **Status 2026-08-27:** RESOLVED. Root cause was a **Python-probe bug**, not a
> C++-client bug. The C++ client was functionally correct but has been hardened
> to be reference-faithful. See the resolution note below.

### Symptom

Client empfängt trotz vollständig gesendeter Handshake-Sequenz **KEINE SND-Audio-Frames**
(`n_snd=0`) auf ALLEN getesteten API-enabled KiwiSDR-Servern. W/F-Frames (Waterfall)
kommen an (`n_wf>0`), SND-Frames (Audio) nicht. Server trennt Verbindung nach ~8–10 s
(CLOSE 1005). Ohne SND-Stream-Empfang kein hörbarer Audio-Output → M3.5 kann nicht
abgeschlossen werden.

**Getestete Server (alle API-enabled, alle `n_snd=0`):**
- kphsdr.com:8073 (KPH Point Reyes, CA)
- kiwisdr2.sdrutah.org:8074 (Northern Utah #2)
- kiwisdr.kfsdr.com:8073 (KFS Half Moon Bay, CA)
- kiwisdr.ku4by.com:8073 (KU4BY Elizabeth City, NC)

### Root Cause

Fehlender/falscher Befehl (oder falsche Reihenfolge) in der Client-Init-Sequenz, der
den SND-Audio-Stream beim Server aktiviert. Der Server erwartet eine bestimmte
Befehlsfolge nach Phase 2; unsere aktuelle Sequenz erfüllt diese Anforderung nicht.

### Aktuelle Handshake-Sequenz (kiwi_client.cpp:handleTextMessage)

```
Phase 1 (onOpen):
  SET auth t=kiwi p=

Phase 2 (Trigger: audio_rate= Message):
  SET AR OK out=12000
  SET ident_user=<name>           (optional, wenn userName konfiguriert)
  SET options=1                   (client hello)
  SET mod=<mode> freq=<kHz> low_cut=<lc> high_cut=<hc>
  SET agc=<on> hang=<h> thresh=<t> slope=<s> decay=<d> manGain=<g>
  SET browser=NetSDRStation
  SET keepalive
```

### Verifiziert (korrekt)

- WebSocket-Pfad `/ws/kiwi/<ts>/SND` ist aktiv (root `/<ts>/SND` liefert keine Daten)
- Alle 5 `CMD_SND_ALL`-Bits (FREQ/MODE/PASSBAND/AGC/AR_OK) werden gesendet
- `KEEPALIVE_SEC` (60s) / `KEEPALIVE_SEC_NO_AUTH` (20s) sind nicht der Disconnect-Auslöser

### Implementierungsplan

#### 1. Referenz-Analyse

**Ziel:** Live-Trace von kiwirecorder.py (jks-prv/kiwiclient) gegen einen echten
KiwiSDR-Server mitschneiden, um die korrekte Befehlssequenz zu extrahieren.

**Methode:**
- `kiwirecorder.py --server=<host> --port=<port> --user=test --freq=14100 --modulation=am --log-level=debug`
  gegen einen der Test-Server ausführen
- Wireshark-Capture parallel laufen lassen (Filter: `tcp.port == 8073`)
- Alle `SET`-Befehle aus dem Debug-Log und/oder Wireshark extrahieren
- Chronologische Sequenz dokumentieren (Phase 1 / Phase 2 / laufender Betrieb)

**Expected Output:** Liste aller `SET`-Befehle in der Reihenfolge, wie sie von der
funktionierenden Referenz gesendet werden.

#### 2. Command-Diff

**Ziel:** Unsere Sequenz (oben) vs. kiwirecorder-Sequenz vergleichen und Abweichungen
identifizieren.

**Zu prüfende Abweichungen:**
- **Fehlende Befehle:**
  - `SET gen=...` (Generator on/off/frequency/attenuation)
  - `SET compression=...` (Audio-Kompression an/aus)
  - `SET OVERRIDE...` (Server-Override-Parameter)
  - Andere undokumentierte `SET`-Befehle
- **Reihenfolge-Abweichungen:**
  - `SET AR OK` Position (vor/nach `SET mod`?)
  - `SET keepalive` Position (vor/nach AGC?)
- **Parameter-Abweichungen:**
  - `out=12000` vs. `out=44100` (Audio-Output-Sample-Rate)
  - `mod=iq` vs. `mod=am` (Default-Modulation)
  - Andere Parameter-Werte

**Expected Output:** Konkrete Liste der Befehle, die ergänzt oder geändert werden
müssen, inklusive Position in der Sequenz.

#### 3. Server-Code-Verifikation

**Ziel:** KiwiSDR-Server-Code analysieren, um die exakte SND-Stream-Start-Bedingung
zu finden.

**Files zu prüfen (jks-prv/KiwiSDR master):**
- `rx/rx_sound.cpp` — Audio-Stream-Handler
- `rx/rx_cmd.cpp` — Command-Parser (`CMD_SND_*`-Flags)
- `kiwi.h` — Konstanten / Flags

**Suche nach:**
- Welches `CMD_*`-Flag oder welche Kombination von Flags triggert den SND-Frame-Start?
- Welche Bedingungen müssen erfüllt sein (`connection_hang` / `keepalive_expired` / ...)?
- Welche `SET`-Befehle setzen die relevanten Flags?

**Expected Output:** Exakte Code-Zeile(n), die den SND-Stream-Start auslösen, plus
die dafür erforderlichen Befehle.

#### 4. Fix implementieren

**Ziel:** Fehlende Befehle in `kiwi_client.cpp:handleTextMessage()` ergänzen und
Sequenz anpassen.

**Änderungen:**
- Neue Command-Builder in `kiwi_commands.h/.cpp` für fehlende Befehle (falls nötig)
- `handleTextMessage()` Phase-2-Block: fehlende Befehle an der richtigen Position
  einfügen
- Ggf. Parameter-Anpassungen (`out=`, `mod=`, ...)

**Files:**
- `source/network/kiwi_commands.h/.cpp`
- `source/network/kiwi_client.cpp` (handleTextMessage)

**Tests:**
- Unit-Test: neue Command-Builder produzieren korrekte `SET`-Strings
- Integration-Test: Mock-Server akzeptiert erweiterte Sequenz
- Manual-Test: Python-Probe gegen Test-Server → `n_snd > 0`

#### 5. Verifizieren

**Ziel:** Bestätigen, dass der SND-Audio-Stream jetzt empfangen wird.

**Test gegen alle 4 Test-Server:**
- kphsdr.com:8073
- kiwisdr2.sdrutah.org:8074
- kiwisdr.kfsdr.com:8073
- kiwisdr.ku4by.com:8073

**Success Criteria:**
- Python-Probe (`probe_full.py`): `n_snd > 0`, Audio-Frames werden empfangen
- Plugin (VST3PluginTestHost): Connect → Status "Connected" → hörbarer Audio-Output
- Kein Disconnect nach ~10 s

**Falls `n_snd` weiterhin 0:**
- Wireshark-Capture vom Plugin-Handshake vs. kiwirecorder-Handshake vergleichen
  (Byte-für-Byte-Diff der `SET`-Frames)
- Server-Side-Log aktivieren (falls Zugriff möglich) um Server-Perspektive zu sehen

#### 6. M3.5 Manual Acceptance durchführen

**Nach erfolgreicher FIX-41-Verifizierung:**
- M3.5-Test-Workflow ausführen (siehe `doc/checklist.md` M3.5)
- Frequency-Change → Live-Reception prüfen
- Audio-Qualität bewerten (keine Knackser/Dropouts)
- M3.5 als done markieren

---

### Lizenz-Kontext (wichtig für Closed Source)

**jks-prv/kiwiclient-Kerncode:** KEINE Lizenzdatei vorhanden. Die KiwiSDR-Familie
(Server + kiwiclient) ist implizit **GPLv3** (Server ist explizit GPLv3). GPLv3-Code
ist **NICHT in Closed Source einbettbar** (Virality-Klausel).

**Wiederverwendbar (permissive):**
- `mod_pywebsocket` (BSD-3-Clause)
- `chunk` (Python Software Foundation License)

**Lösung für dieses Projekt:**
Das WebSocket-Protokoll selbst (SET auth/AR/ident_user, MSG/SND/W/F-Layout) ist ein
**Interface** und kann eigenständig reimplementiert werden — was wir bereits tun
(`kiwi_client.cpp`, `kiwi_commands.cpp`). Die Referenz-Analyse (Schritt 1–3) nutzt
nur das **Protokoll-Verhalten** (Befehlssequenz), nicht den Code. Das ist clean-room
design und rechtlich unbedenklich.

**Dokumentiert in:** `doc/framework-licensing.md`, NotebookLM „FIX-41 Kiwi-Disconnect
Root Cause"

---

### Priority

**CRITICAL** — Ohne SND-Stream-Empfang kein Audio-Output, M3 nicht abschließbar.
Alle anderen M3-Schritte (M3.1–M3.4, M3.6) sind vollständig implementiert und
getestet. FIX-41 ist der einzige verbleibende Blocker für M3-Completion.

---

## FIX-41 Resolution (2026-08-27)

### Endgültige Root Cause

**Der `n_snd=0`-Befund war ein Bug im Python-Probe `probe_full.py`, NICHT im
C++-Client.** Live-Trace gegen kphsdr.com:8073 zeigt:

- Der Server sendet `sample_rate=11998.947054` **zuerst** (t≈1.2s), danach erst
  `audio_init=0 audio_rate=12000` (t≈1.8s).
- `probe_full.py` löste Phase 2 auf den **ersten** Trigger aus (`sample_rate=`)
  und latchte `snd_ph2` → als `audio_rate=` später eintraf, wurde `SET AR OK`
  **nie gesendet**.
- **`SET AR OK` ist der Befehl, der den SND-Audio-Stream aktiviert.** Ohne ihn
  startet der Server keine SND-Frames und kickt die Verbindung nach ~10s (Idle).

Verifiziert mit `probe_fix41.py` (AR OK auf `audio_rate=`, Rest auf `sample_rate=`):
SND-Frames fließen. Ebenso `out=44100` UND `out=12000` — der `out=`-Wert ist frei
wählbar, `SET AR OK` an sich ist entscheidend.

### C++-Client-Härtung (umgesetzt, Referenz-faithful)

Auch wenn der C++-Client funktional korrekt war (`probe_cpp.py` bestätigt SND-Frames),
wurde er an die kiwiclient-Referenz angeglichen:

1. **`SET options=1` vor `SET auth`** — kiwiclient `open()`: "must be sent before auth".
2. **`SET AR OK in=<audio_rate> out=<audio_rate>`** — `audio_rate` wird aus der MSG
   geparst statt hart auf 12000 gesetzt (andere Kiwis nutzen andere Raten).
3. **Bogus-Frame `SERVER DE CLIENT openwebrx.js SND` entfernt.**
4. **`SET squelch=0 max=0`, `SET genattn=0`, `SET gen=0 mix=-1` ergänzt.**

**Neue Handshake-Sequenz:** `options` → `auth` → `AR OK` → (optional `ident_user`)
→ `squelch` → `genattn` → `gen` → `mod/freq` → `agc` → `keepalive`.

### Verifikation

- **Unit/Integration:** `kiwi_commands_tests.cpp` (+5 Serializer-Tests),
  `kiwi_client_tests.cpp` + `kiwi_bridge_tests.cpp` Frame-Zahl 7→9 (10 mit ident_user).
  **87/87 Testfälle grün (Debug + Release), Validator 47/47.**
- **Live:** `probe_newcpp.py` (exakte C++-Sequenz) gegen kphsdr.com:8073 →
  202 SND-Frames, STAYED-CONNECTED.

### Offen (Nutzer-Aufgabe)

Der letzte manuelle Test im VST3PluginTestHost (hörbares Audio über ASIO) bleibt
eine Nutzer-Aufgabe — der Netzwerk-Handshake ist verifiziert, das GUI-gesteuerte
Anhören nicht automatisierbar (M3.5-Workflow in `doc/checklist.md`).

## Post-manual-test findings (2026-08-27)

Manueller Test im VST3PluginTestHost (Debug): **Verbindung erfolgreich & stabil**
(FIX-43 wirkt). Dabei zwei Funktionsfehler + ein Feature-Wunsch identifiziert
und anschließend **behoben** (Details + Tests in `doc/checklist.md`):

1. **BUG-06 — Frequenz/LowCut/HighCut reagieren nicht.** ✅ BEHOBEN
   `applyParamValue()` setzte `paramsDirty_`, aber `sendPendingParams()` lief
   nur einmal beim Connect. Fix: `process()` flusht `paramsDirty_` rate-limitiert
   (20/s, `paramSendLimiter_`) auf den Worker.

2. **BUG-07 — Volume reagiert nicht.** ✅ BEHOBEN
   `volume_` wurde gespeichert, aber `renderPipeline()` wendete den Gain nie an.
   Fix: Ausgabe-Samples werden mit `volume_.load()` multipliziert.

3. **FEATURE-01 — Connect-Button → Disconnect bei `Connected`.** ✅ UMSETZEN
   Ende-zu-Ende-Disconnect-Pfad: `window.vstHost.disconnect()` → `disconnect`
   Envelope → `PluginController::disconnect()` → `IMessage
   "NetSDRStation:Disconnect"` → `PluginProcessor::disconnectStation()`
   (`kiwiClient_.reset()` = kein Auto-Reconnect) → `emitStatus("Disconnected")`.
   UI-Button toggelt `Connect`/`Disconnect` anhand des Status.

4. **Default-Station** auf `kphsdr.com:8072` umgestellt (STABLE, Marconi-T):
   `ui/src/views/PluginView.vue`, `ui/src/components/StationInput.vue`.
