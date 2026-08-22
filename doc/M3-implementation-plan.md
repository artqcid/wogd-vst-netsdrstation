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

#### T2 — Playwright MCP

Install Playwright (`npm install -D @playwright/test`) and the Playwright MCP
server. Write a smoke test:
- Start the Vite dev server.
- Open `http://localhost:5173` in Playwright headless browser.
- Assert the station input, frequency field, and mode select are visible.

This catches UI regressions without loading the VST in a host.

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

## Risk register

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| KiwiSDR `SND` frame format differs from implementation | Medium | Add a hex-dump integration test that decodes a captured real frame. |
| Audio underruns at 128-sample buffer | Medium | Tune jitter buffer; add a configurable pre-fill parameter. |
| WebSocket reconnection on disconnect | Medium | Implement exponential backoff in `KiwiClient` (defer to M3.4 or M4). |
| 28 params cause DAW parameter list to be unwieldy | Low | Group params into VST3 parameter groups (categories) in `plugin_controller.cpp`. |
| Resampler introduces audible latency | Low | Use `SRC_SINC_FASTEST` quality for live monitoring; `SRC_SINC_BEST_QUALITY` for export (expose as a param). |
