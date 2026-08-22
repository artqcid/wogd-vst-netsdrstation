# NetSDRStation-VST - Draft Plan

_First draft plan (Entwurfsplan). Detailed knowledge lives in
`doc/architecture.md`; workflow (build/debug/hot-reload) in
`doc/workspace-workflow.md`; coding rules in `doc/coding-standards.md`;
licensing in `doc/framework-licensing.md`; test strategy in
`doc/test-strategy.md`; open tasks in `doc/checklist.md`._

## Goal

Native VST3/AU/CLAP plugin that connects directly to KiwiSDR servers and
exposes the decoded audio as a low-latency DAW audio source, with DAW-native
frequency modulation (automation/LFO) and a Vue 3 WebView UI.

## Architecture goals (foundation first)

- **Modular, robust architecture** is the top priority. The plugin must be a
  reusable foundation for other VSTs.
- **Milestone 1 delivers a generic VST foundation** (all platforms): a clean,
  forkable checkpoint in git from which new VSTs can always be created.
- **Only from Milestone 2 onward** are project-specific (KiwiSDR) details
  implemented.
- A fork of Milestone 1 must be a solid foundation for a synth plugin (and
  other plugin types).

## Milestone 1 - Generic VST foundation (+ sine synth proof)

A robust, modular, cross-platform VST3 foundation, proven by a sine synth.

- Cross-platform CMake scaffold (`CMakeLists.txt` + `CMakePresets.json`) for
  Windows/macOS/Linux (Windows tested locally).
- Modular structure: reusable "VST shell" (entry point, processor/controller
  base, parameter registry, threading, WebView editor) separated from the DSP
  core.
- VST3 sine synth as proof: sine oscillator (phase accumulator), params
  `freq`, `volume`, `mute`.
- Vue 3 + Vite UI (frequency knob, volume knob, mute button) via
  webview/webview, with hot reload.
- VST host debugging (`editorhost` / `pluginval`) and unit tests (CCD yellow).
- **Exit criteria:** a git checkpoint that can be forked as the base of a new
  plugin; builds on all three platforms; loads in a DAW; UI hot-reload works.

## Milestone 2 - KiwiSDR integration (project-specific)

This is where NetSDRStation-specific functionality begins.

### Phase 2.1 - CLI Prototype
- Establish KiwiSDR WebSocket connection (IXWebSocket, port 8073).
- Implement IMA ADPCM decoding in C++.
- Verify handshake + `SET` commands + continuous audio stream in a CLI app.
- **Exit criteria:** CLI receives and decodes a live KiwiSDR stream.

### Phase 2.2 - DSP Integration
- Sample-accurate parameter modulation (frequency via automation/LFO).
- LFO rate-limiting to 20 Hz (spam protection).
- Sample-rate conversion via libsamplerate (12/24 kHz -> DAW rate).
- Jitter buffer (100-150 ms) on connect.
- **Exit criteria:** frequency can be modulated from the DAW in real time.

### Phase 2.3 - WebView & Vue Integration
- Bind the Vue UI to the KiwiSDR controls (frequency, volume, mute).
- Bidirectional JSON communication (UI <-> EditController <-> DSP).
- **Exit criteria:** UI controls the live receiver frequency.

> M2 delivered the KiwiSDR building blocks as unit/integration-tested
> libraries, but did not wire them into the processor. The actual receiver
> pipeline, the full UI and the station selection follow in M3–M5 (see
> `doc/checklist.md` and `doc/ui-architecture.md`).

## Milestone 3 - Integration & Ship

- Wire the full `KiwiClient → ImaAdpcmDecoder → AudioSampleQueue → Resampler
  → JitterBuffer → process()` pipeline into `PluginProcessor`.
- Add the complete KiwiSDR parameter set (28 VST3 parameters: Core, AGC,
  Audio, Display) so every setting is DAW-automatable.
- Real-time safety audit + manual acceptance against a real KiwiSDR.
- **Exit criteria:** the VST receives and plays a live KiwiSDR stream.

## Milestone 4 - KiwiSDR UI parity (Vue)

- 1:1 re-implementation of the KiwiSDR browser interface in Vue (see
  `doc/ui-architecture.md` §3 for the complete element inventory).
- **Grundbedingung:** the editor is freely resizable by dragging the
  bottom-right corner; the UI reflows continuously at any size.
- **Exit criteria:** the VST is operable exactly like the web UI
  (`g8ure.ddns.net:8078`).

## Milestone 5 - Station selection tab

- Tab-based UI: **Tab 1 "SDR Stations"** (scrollable station directory,
  click-to-connect) and **Tab 2 "KIWI UI"** (the M4 receiver UI).
- Default is no station loaded; Tab 2 then shows only "please select station
  first".
- **Exit criteria:** picking a station connects to it and activates the
  receiver UI.

## Milestones

| Milestone | Deliverable | Scope |
|-----------|-------------|-------|
| M1 | Generic VST foundation + sine synth (forkable checkpoint) | generic |
| M2 | KiwiSDR components (network/decode/resample/DSP, unit-tested) | project |
| M3 | Integration & Ship (network→DSP pipeline + full parameter set) | project |
| M4 | KiwiSDR UI parity in Vue (1:1 web interface, resizable) | project |
| M5 | Station selection tab (SDR Stations / KIWI UI) | project |

## Open Questions / Risks

- Exact VST3 SDK integration details (vendor/entry-point) - resolve in M1.
- WebView <-> VST window-handle binding on Windows/macOS/Linux - resolve in M1.
- Resampler quality/CPU trade-off at low buffer sizes - validate in M2.
- Keep M1 fully generic (no KiwiSDR specifics) - maintain forkability.
- Waterfall/spectrum display (M4.6) requires a dedicated spectrum data stream
  from the server, not delivered by the audio-only M3 pipeline.
- Station directory endpoint/format (M5.1) to be confirmed at implementation
  time.
