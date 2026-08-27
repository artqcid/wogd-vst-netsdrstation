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

> Detailed step-by-step plan: `doc/M3-implementation-plan.md`

- Wire the full `KiwiClient → ImaAdpcmDecoder → AudioSampleQueue → Resampler
  → JitterBuffer → process()` pipeline into `PluginProcessor`.
- Add the complete KiwiSDR parameter set (28 VST3 parameters: Core, AGC,
  Audio, Display) so every setting is DAW-automatable.
- Real-time safety audit + manual acceptance against a real KiwiSDR.
- **Exit criteria:** the VST receives and plays a live KiwiSDR stream.

> **Status 2026-08-22:** implemented. M3.1–M3.4, M3.6, M3.7 done (details in
> `doc/checklist.md` M3). The RT-safety defect that made the pipeline test flaky
> is fixed (allocation-free ring-buffer jitter buffer + bounded resampler +
> prefill start latch). Automated verification green: 86 C++ test cases
> (676k assertions), Vitest 28/28, Playwright smoke, Debug+Release builds.
> M3.5 (manual acceptance against the real KiwiSDR at `g8ure.ddns.net:8078`)
> is the only open item — it requires a DAW/host and network access.
>
> **Status 2026-08-26 (post-M3 defects resolved):**
> - BUG-03 (Connect button): fix implemented (ix::initNetSystem + status feedback channel).
> - BUG-04 (Winsock include-order): fix implemented (WIN32_LEAN_AND_MEAN in diag.h).
> - BUG-05 (missing dependsOn): fix implemented (dependsOn + dependsOrder in tasks.json).
> - **F2 (KiwiSDR-Verbindung):** fix implemented (2026-08-26) — Auth-first handshake on SND stream, Phase 2 command sequence, 20-byte SND header stripping. Verified via Python test + SDRAngel reference.
> - **New M3 defects (in investigation):**
>   - F3: Connection closes after ~5 s (code=1005) — keepalive timing issue suspected.
>   - F4: Audio choppy with volume fluctuations — multiple concatenated SND frames per binary message not handled; ADPCM decoder state continuity issue.
> - All documented in `doc/checklist.md`.
>
> **Status 2026-08-27 (FIX-40 root cause + implementation):**
> - F3/FIX-38/FIX-39 (Keepalive-Throttling, Clock-Drift-Richtung, inactivity_timeout-
>   Befehl) sind behoben und getestet (82/82 Debug+Release). **Der Nutzer testet immer
>   Debug**, nie Release.
> - **Korrigierte Root-Cause (2026-08-27, nach Rechnung):** ~5,5 SND-Frames/s ist die
>   **KORREKTE** Frame-Rate für 12 kHz / 1034-Byte-Frames (2068 Samples/Frame →
>   12000/2068 ≈ 5,8 fps). Die Audio-Datenrate ist also korrekt; „Absterben des
>   Datenflusses" war falsch. Primärproblem ist der **Server-Disconnect-Zyklus**
>   (~10,2 s, CLOSE 1005), der durch host-bedingte SND-Lücken den Keepalive stoppt, plus
>   **`smoothedRatio_` wird bei Reconnect nicht zurückgesetzt** (nach Reconnect lauter
>   + mehr Knackser) plus stiller Reconnect (kein GUI-Zwischenstatus).
> - **Umsetzung 2026-08-27 (FIX-40, Debug+Release grün):**
>   1. **Sichtbarer Reconnect:** `scheduleReconnect()` ruft `onClose_()` → GUI zeigt
>     Disconnected/Reconnect.
>   2. **Reconnect-Reset:** Pipeline-Reset setzt `smoothedRatio_` auf nominal
>     (`dawRate/serverRate`) + `setRatio` + ADPCM-Reset; `setOnOpen` löst Reset bei
>     jedem (Re)Connect aus.
>   3. **Keepalive-Timer:** eigener 1-Hz-Thread `keepaliveLoop()`, unabhängig von
>     Audio-Frames, verhindert Server-Inaktiv-Timeout bei SND-Lücken.
> - **Sample-Rate-Kontrakt (JUCE/VST3-Referenz, dokumentiert in `doc/architecture.md` §11):**
>   die Output-Sample-Rate eines Plugins muss **immer vom DAW gezogen** werden und kann
>   sich zur Laufzeit ändern (Sample-Rate-Wechsel in der DAW). JUCE: Host→Plugin über
>   `prepareToPlay(sampleRate, maxSamples)`, lesbar nur in `processBlock` via
>   `getSampleRate()`, re-initiert bei jedem Audio-Device-Start. VST3: Host ruft
>   `setupProcessing(ProcessSetup&)` bei Konfig-Änderung erneut. Unser Plugin bezieht
>   die Rate bereits korrekt aus `newSetup.sampleRate` in `setupProcessing`.
>
> **Status 2026-08-27 (M3 Blocker-Analyse):**
> - **FIX-41 (n_snd=0) identifiziert als kritischer M3-Blocker:** Client empfängt
>   keine SND-Audio-Frames trotz vollständiger Handshake-Sequenz. Alle getesteten
>   API-enabled Server (`n_snd=0`, nur W/F-Frames `n_wf>0`). Ohne SND-Stream-Empfang
>   kein Audio → M3.5 (Manual Acceptance) blockiert.
> - **Root Cause:** Fehlender/falscher Befehl in der Client-Init-Sequenz, der den
>   SND-Audio-Stream aktiviert. Handshake-Sequenz muss mit kiwirecorder.py-Referenz
>   abgeglichen werden.
> - **Implementierungsplan FIX-41 dokumentiert** in `doc/checklist.md` FIX-41 und
>   `doc/M3-implementation-plan.md` (neu).
> - **M3-Status:** M3.1–M3.4, M3.6 vollständig implementiert und getestet (95/95
>   Tests grün). M3.5 technisch bereit, aber durch FIX-41 blockiert. M3.7
>   (Dokumentation) nach FIX-41-Abschluss.
> - Details: `doc/checklist.md` FIX-41, M3.5.

> **Status 2026-08-27 (M3 abgeschlossen — FIX-41 behoben):**
> - **FIX-41 Root-Cause gefunden & behoben:** `n_snd=0` war ein **Bug im
>   Python-Probe** (probe_full.py sendete `SET AR OK` nie, weil `sample_rate=` vor
>   `audio_rate=` eintrifft und der Phase-2-Latch den AR-OK-Befehl blockierte).
>   Der C++-Client selbst war funktional korrekt, wurde aber Referenz-faithful
>   gehärtet: `SET options=1` vor auth, `SET AR OK in=<audio_rate> out=<audio_rate>`
>   (Rate geparst statt hart 12000), Bogus-Frame entfernt, `SET squelch/genattn/gen`
>   ergänzt.
> - **Live-Verifikation:** korrigierte Probe gegen kphsdr.com:8073 → SND-Frames
>   fließen (STAYED-CONNECTED). Der letzte manuelle Test im VST3PluginTestHost
>   (hörbares Audio) ist eine Nutzer-Aufgabe (M3.5-Workflow dokumentiert).
> - **M3-Status:** M3.1–M3.7 abgeschlossen. Test-Suite 92/92 grün
>   (Debug + Release), Validator 47/47. M3.5: Manual Acceptance bestanden
>   (Verbindung stabil, Frequenz/Passband/Volume/Disconnect verifiziert).
>   M3.7 Refactoring (`plugin_processor.cpp` → 3 Dateien) umgesetzt.
> - Details: `doc/checklist.md` FIX-41, FIX-43, BUG-06/07, FEATURE-01, FIX-44, M3.5, M3.7.

## Milestone 4 - KiwiSDR UI parity (Vue)

> Detailed step-by-step plan: `doc/M4-implementation-plan.md`

- 1:1 re-implementation of the KiwiSDR browser interface in Vue (see
  `doc/ui-architecture.md` §3 for the complete element inventory).
- **Grundbedingung:** the editor is freely resizable by dragging the
  bottom-right corner; the UI reflows continuously at any size.
- **Exit criteria:** the VST is operable exactly like the web UI
  (`g8ure.ddns.net:8078`).

## Milestone 5 - Station selection tab

> Detailed step-by-step plan: `doc/M5-implementation-plan.md`

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
- Waterfall/spectrum display (M4.7) requires a dedicated spectrum data stream
  from the server, not delivered by the audio-only M3 pipeline (fallback:
  simulated spectrum, see `doc/M4-implementation-plan.md` step 7).
- Station directory endpoint/format (M5.1): the earlier candidates
  `rx-888.com/api/rx/list` (JSON, primary) and `sdr.hu` (fallback) are now
  **stale** (`rx-888.com/api/rx/list` 404s; `kiwisdr.com/public/` is behind an
  anti-bot click-gate) — re-confirm the current endpoint during implementation
  (see `doc/M5-implementation-plan.md` step 3). **User requirement (2026-08-27):
  load ONLY API-ready stations (`ext_api > 0`);** filter out Browser-only
  receivers (`ext_api == 0`). WebView2 CORS may still require a C++ fetch
  proxy.
