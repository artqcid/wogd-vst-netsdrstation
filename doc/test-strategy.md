---
type: Test Strategy
title: Test Strategy — NetSDRStation-VST
description: Automated-first test strategy, CCD yellow/green, coverage targets, test pyramid
status: stable
generated:
  by: human:marku
  at: 2026-07
verified:
  by: human:marku
  at: 2026-08-29
tags: [testing, ccd, coverage, ci, ctest, vitest, playwright]
---

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

## 1a. UI Visual Parity — Research-First Rule (mandatory for ALL UI tasks)

Every UI component that must visually match the KiwiSDR reference **requires
visual research BEFORE any implementation or test writing**. This rule applies
to all M4/M5 UI tasks without exception.

### What "visual research" means

1. **Fetch the KiwiSDR source code** from `https://github.com/jks-prv/KiwiSDR`
   — specifically `web/openwebrx/openwebrx.js`, `web/kiwi/waterfall.js`, and
   any CSS files. Find the exact draw calls, event handlers, and constants for
   the component being implemented.
2. **Inspect the live KiwiSDR instance** at `http://kiwisdr.areg.org.au:8073/`
   (WebUI port 8073 — NOT 8072 which is the API port). Capture the DOM, CSS,
   and visual state for each component state (e.g. cursor yellow vs. lime).
3. **Document the findings** as a verbatim "Research-Ergebnis" block in the
   corresponding bug/task entry in `doc/M4c.7-bugs.md` (or the relevant
   milestone doc). Include: exact geometry (coordinates, pixel values),
   exact colors (hex), exact interaction zones (pixel widths), exact state
   transition conditions (with code references).

### Research deliverable (required before any implementation)

A structured spec block in the bug doc containing:
- Source file + line number for every draw call / event handler
- Exact pixel coordinates and dimensions
- Exact colors (hex, not names)
- Exact interaction zone widths (px)
- Exact state transition condition (quoted from source)

### Why this rule exists

Without prior visual research, agents write tests that verify the (wrong)
implementation instead of the (correct) KiwiSDR reference behavior. This
was the root cause of Bug 7: the cursor shape (trapezoid with outward-slanting
flanks), color switch condition (50px pixel width threshold, NOT zoom level),
and hit zones were all wrong because no source-code research was done first.

## 1b. UI E2E Tests — Visual Assertion Rules (mandatory)

E2E tests for UI components with visual parity requirements MUST follow
test-first (CCD blue): write the test against the KiwiSDR SOLL-state FIRST,
then implement until the test is green.

### Mandatory assertions for visual components

Every E2E test for a visual UI component MUST include at minimum:

1. **Color assertion:** verify the correct color is rendered (e.g. `lime` vs
   `yellow`) under each state condition. Use `page.evaluate()` to read canvas
   pixel colors, or `toHaveCSS()` for CSS-based components.
2. **Shape/geometry assertion:** verify the rendered shape matches the
   KiwiSDR reference geometry. For canvas-drawn components: sample key pixels
   (corners, center, flanks) and assert expected colors. For SVG: assert
   `d`/`points` attribute values.
3. **Interaction assertion:** for each drag zone, simulate `dragTo()` and
   assert the correct parameter changes (e.g. dragging left flank changes
   only `lowCut`, not `freqKhz`).
4. **State transition assertion:** assert the visual state (color/shape)
   changes at the correct trigger condition (e.g. passband width crossing
   the 50px threshold), not at an approximate or wrong condition.

### What is NOT sufficient

- `toBeVisible()` alone — does not verify visual correctness.
- `toHaveText()` on a label — does not verify the shape behind it.
- Unit tests on logic functions — do not verify rendering.
- Tests written AFTER implementation — verify the implementation, not the spec.

### Baseline screenshot workflow

For complex components (frequency ruler cursor, waterfall, band scale):
1. Capture a reference screenshot of the live KiwiSDR instance (stored in
   `ui/e2e/reference/kiwisdr-visual/`).
2. Write a Playwright `toHaveScreenshot()` test comparing the plugin render
   to the reference (with a tolerance for color/font differences).
3. The test FAILS until the implementation matches. This is correct — it is
   the CCD blue state.

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
- **UI parity (M4.8):** side-by-side against `kphsdr.com:8072` — every
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
| clangd-based C++ semantic MCP | done (M3.6) | since M3 | true call graph, types, templates, macros, thread audit | MIT (lsp-mcp-server) |
| Playwright MCP | done (M3.6) | since M3 | interactive DOM/UI verification + screenshots | Apache-2.0 |

Rationale:

- **clangd MCP (implemented 2026-08-22):** `lsp-mcp-server` (MIT) registered
  as `clangd_mcp` in `opencode.json`; bridges to `clangd --background-index`.
  Needs `compile_commands.json` (`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`), now
  generated by the `win-clangd` preset (Ninja + clang-cl, hosting examples
  off). Complements `netsdr_rag` (structural) with semantic precision.
- **Playwright (implemented 2026-08-22):** committed E2E smoke test
  (`ui/e2e/smoke.spec.ts` + `ui/playwright.config.ts`) runs headless against
  the Vite dev server and asserts the station input, frequency field and mode
  select are visible. Browser tests only cover the UI layer (bridge is mocked);
  DSP integration still needs the real host. Vitest + committed Playwright
  test scripts remain the primary UI tests.

