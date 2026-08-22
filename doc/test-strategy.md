# Test Strategy — NetSDRStation-VST

_Complete test strategy. Automated first (CCD yellow/green), manual only where
necessary (audio quality, host-specific, cross-platform listening). Applies to
all milestones; per-item test points are listed in `doc/checklist.md`._

## 1. Principles

- **Automated first.** Every deterministic behavior is covered by automated
  tests; manual tests only where automation is impossible or impractical
  (audible quality, host quirks, macOS/Linux smoke when not in CI).
- **Test-first** (CCD blue) for new DSP/features: write the test before the
  implementation.
- **Fast + deterministic:** unit tests must run in CI without audio hardware,
  network, or real KiwiSDR servers (mock everything external).
- **Permissive tooling only** (see `doc/framework-licensing.md`).

## 2. Test levels & tooling

| Level | Tool | License | Scope |
|-------|------|---------|-------|
| C++ unit tests | Catch2 | BSL-1.0 | DSP core, parameter model, queue, decoder, resampler |
| C++ unit tests (alt.) | GoogleTest | BSD-3-Clause | equivalent option |
| Static analysis | clang-tidy / Cppcheck | Apache-2.0 / GPL-3 (Cppcheck: GPL) | lock-free/alloc-free audio thread, code quality |
| VST3 validation | VST3 SDK `validator` + `hostchecker` | MIT (VST3 SDK) | load/instantiate/process/parameter automation |
| VST3 validation (opt.) | pluginval | verify license before use | cross-format validation |
| Vue UI unit tests | Vitest + Vue Test Utils | MIT | components, bridge (pluginService), state |
| Vue type check | vue-tsc | MIT | TypeScript correctness |
| E2E (UI, standalone) | Playwright | Apache-2.0 | browser-based UI flows (dev server) |
| CI | GitHub Actions | — | build/test matrix windows/macos/linux |
| RAG MCP | Python `unittest`/pytest | — | netsdr_mcp_server.py indexing/query |

> Cppcheck core is GPL-3.0 — for a closed-source project prefer clang-tidy
> (Apache-2.0, part of LLVM). Catch2 (BSL-1.0), GoogleTest (BSD-3), Vitest
> (MIT), Playwright (Apache-2.0) are all permissive.

## 3. Test pyramid

```
          /  Manual (listening, DAW, cross-platform smoke)  \
         /   E2E (Playwright) + host validation (validator)  \
        /   Integration (WebSocket mock server, UI<->DSP)     \
       /    Unit (DSP, params, queue, decoder, resampler, Vue) \
```

- Unit: most coverage, fastest.
- Integration: WebSocket against a local mock KiwiSDR server; bridge roundtrip.
- Host validation: `validator`/`hostchecker` load the built `.vst3` headlessly.
- E2E: UI flows in a real browser against the Vite dev server.
- Manual: audible checks + macOS/Linux smoke until covered by CI.

## 4. DSP test specifics

### 4.1 Sine oscillator
- Deterministic reference: expected `sin(2*pi*f*n/fs)` per sample.
- **Frequency:** Goertzel/FFT peak at the set frequency; no significant energy
  elsewhere (THD threshold).
- **Amplitude:** output amplitude equals `volume` parameter.
- **Phase continuity:** no discontinuity across block boundaries.
- **Mute:** output is exactly zero when mute is on; resumes without click.

### 4.2 IMA ADPCM decoder
- **Reference vectors:** known IMA ADPCM encoded frames decode to expected PCM.
- **Roundtrip:** decode -> re-encode -> decode, error within tolerance.
- **Edge cases:** silence -> zero; full-scale input -> no overflow/clipping.

### 4.3 Sample-rate conversion (libsamplerate)
- Sine at 12 kHz and 24 kHz -> 44.1/48 kHz: frequency preserved, THD below
  threshold.
- **Aliasing:** input near Nyquist -> no folded artifacts above threshold.

### 4.4 Lock-free SPSC queue (moodycamel)
- **Stress:** producer writes N blocks, consumer reads N; order preserved, no
  loss, no corruption (checksum per block).
- **Underflow:** slow consumer -> graceful degradation, no crash.

### 4.5 Parameter model / smoothing
- **Ramp:** value 0 -> 1 over N samples is monotonic; max step below threshold
  (no zipper noise).
- **Ranges/defaults/IDs:** every parameter has correct range, default, ID.

### 4.6 Rate limiter (LFO spam protection)
- Fire M updates in T seconds -> assert at most ~20 updates/second are sent.

### 4.7 Jitter buffer
- Absorbs the configured 100-150 ms of jitter without underrun; overflow drops
  the oldest data; no crash.

## 5. Integration & host validation

- **Mock KiwiSDR server (local):** accepts the WebSocket connection, verifies
  handshake (`SET user=...`, `SET inert=0`, `SET agc=1`, `SET freq=...`), then
  streams synthetic IMA ADPCM frames -> the plugin must decode to audio.
- **VST3 `validator` / `hostchecker`:** load the built `.vst3`, instantiate,
  run the audio process, exercise all parameters -> must pass with no errors.
- **UI<->DSP roundtrip:** Vitest bridge test + Playwright E2E against the Vite
  dev server (knob -> `setParameter` message -> mock echoes state update).

## 6. CI pipeline (GitHub Actions)

- Matrix: `windows-latest`, `macos-latest`, `ubuntu-latest`.
- Steps: checkout -> configure CMake -> build -> `ctest` (C++ unit tests) ->
  Vue `npm ci` + `vitest run` + `vue-tsc`.
- Host validation (`validator`/`hostchecker`) runs headless on all three.
- This guarantees R1 (cross-platform build) at all times.

## 7. Manual tests (documented, when automation is not possible)

- **Listening:** sine tone audibly clean; mute silent; volume scales; no
  clicks/dropouts on param change.
- **DAW automation:** host automation of `freq` modulates the tone in real time.
- **macOS / Linux smoke:** load the plugin in a host on each platform until CI
  covers it.
- **HMR:** edit a Vue component -> change appears live in the plugin (dev mode).
- **Resize (M4.1 Grundbedingung):** drag the editor bottom-right corner in a
  host — the UI reflows continuously at any size without clipping.
- **UI parity (M4.8):** side-by-side against `g8ure.ddns.net:8078` — every
  control present, every readout live.
- **Station flow (M5):** Tab 1 "SDR Stations" lists the directory; clicking a
  station connects and activates Tab 2 "KIWI UI"; with no station, Tab 2 shows
  "please select station first".

## 8. Coverage & exit criteria

- Unit test coverage >= 90% (CCD yellow); aim ~100% on pure DSP.
- A milestone point is "done" only when its automated test is green AND any
  required manual test is recorded.
- Static analysis (clang-tidy) runs in CI; no warnings in the audio thread.

## 9. AI development helpers (MCP servers) — decision

These are development helpers for the AI agent (VSCode already provides the
human with the same capabilities). They are NOT test infrastructure and do NOT
replace committed automated tests.

| MCP server | Adopt | When | Purpose (AI agent) | License |
|------------|-------|------|--------------------|---------|
| clangd-based C++ semantic MCP | yes (defer) | from M1 (once CMake + C++ code exist) | true call graph, types, templates, macros, thread audit | Apache-2.0 |
| Playwright MCP | yes (defer) | from M1.6 (UI phase) | interactive DOM/UI verification + screenshots | Apache-2.0 |

Rationale:

- **clangd MCP:** needs `compile_commands.json` (`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`),
  so it only pays off once real C++ code exists. Complements `netsdr_rag`
  (structural) with semantic precision; not useful before M1.
- **Playwright MCP:** interactive browser access for the AI agent. Browser
  tests only cover the UI layer (bridge is mocked); DSP integration still needs
  the real host. Vitest + committed Playwright test scripts remain the primary
  UI tests; the MCP is an interactive debugging aid.

