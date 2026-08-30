---
type: Architecture
title: NetSDRStation-VST — Architecture
description: Complete system architecture: problem statement, tech stack, threading model, protocol handshake, audio pipeline, conventions
status: stable
generated:
  by: human:marku
  at: 2026-07
verified:
  by: human:marku
  at: 2026-08-29
tags: [architecture, vst3, kiwisdr, threading, webview2, audio-pipeline]
---

# NetSDRStation-VST - Architecture

_Detailed architecture knowledge. Source: NotebookLM "NetSDRStation-VST"
(project concept V3). Manually maintained; complements the auto-generated
[`doc/code_wiki.md`](./code_wiki.md) (symbol index, MCP-only),
[`doc/index.md`](./index.md) (LLM-Wiki catalog) and
[`doc/plan.md`](./plan.md) (draft plan)._

## 1. Problem Statement

Browser-based SDR clients (e.g. KiwiSDR) suffer from high audio latency and
no real-time modulatability. It is impossible to route the audio stream of a
web receiver directly into a DAW, or to modulate the receiver frequency using
DAW-native LFOs / automation. This blocks the creative integration of global
shortwave signals into music production and sound design.

## 2. Objective

Develop a **browserless, native VST3/AU/CLAP plugin** that:

- establishes a direct, low-latency connection to KiwiSDR servers,
- decodes the audio,
- exposes it as an audio source in the DAW.

## 3. Key Features

- Direct WebSocket connection to KiwiSDR servers.
- Native IMA ADPCM decoding in C++.
- Real-time frequency control via DAW parameters (automation, LFOs).
- High-quality resampler (12 kHz / 24 kHz -> DAW sample rate).
- Integrated OS-native WebView UI (built with Vue 3).

## 4. Tech Stack (free / open-source)

| Component | Library / Source | License | Architectural function |
|-----------|------------------|---------|------------------------|
| VST3 SDK | Steinberg SDK | MIT (2026) | DAW integration, audio thread, parameter management |
| WebView | webview/webview | MIT | Bind OS-native WebView to the VST window handle |
| UI Framework | Vue.js 3 / Vite | MIT | Reactive UI (static HTML/JS/CSS bundle) |
| WebSocket | IXWebSocket | BSD-3-Clause | Boost-free C++ WebSocket client |
| Thread Queue | ReaderWriterQueue | BSD-2-Clause | Lock-free, wait-free SPSC queue (moodycamel) |
| Resampler | libsamplerate | BSD-2-Clause | High-quality SRC (Secret Rabbit Code) |
| SDR Client | KiwiSDR Client | Python / C++ | Reference implementation of the KiwiSDR protocol |

**Licensing constraint:** only license-free frameworks/libraries may be used,
and the plugin must be sellable without open-sourcing the code. The stack
above is fully permissive. See `doc/framework-licensing.md` for the full
analysis (JUCE/KFR/HISE are excluded; iPlug2/DPF are permissive alternatives).

## 5. Communication / Bidirectional Binding

```
[ Vue.js UI (WebView) ]
        │
        │ (window.vstHost.setParameter)
        ▼
[ C++ EditController ] ──(vst3:performEdit)──► [ DAW Host (Automation/LFO) ]
        │                                            │
        │ (window.updateVueState)                    │ (vst3:process)
        ▼                                            ▼
[ Vue.js UI (WebView) ] ◄──(SPSC Queue)─── [ C++ AudioProcessor (DSP) ]
```

- **UI -> DSP:** Vue UI interaction -> JS-injected C++ function -> update VST3
  parameter -> sent to DSP.
- **DSP/Host -> UI:** DAW modulates parameter -> EditController notified ->
  execute JS in WebView to update Vue state.

### Station / Status channel (M3, implemented)

In addition to the parameter channel above, M3 adds two extra message paths:

**setStation (UI → Processor):**
```
PluginEditor::onJavaScriptMessage(type=setStation)
  → PluginController::setStation(host, port)
    → IConnectionPoint::sendMessage("NetSDRStation:SetStation", attr "HostPort")
      → PluginProcessor::notify()
        → workerThread_.post(connectToStation)
          → KiwiClient::connect(host, port)
```

**Status feedback (Processor → UI):**
```
KiwiClient onOpen/onError/onClose callback
  → PluginProcessor::emitStatus("Connected"|"Error"|"Disconnected")
    → workerThread_.post(IMessage "NetSDRStation:Status")
      → PluginController::notify()
        → statusSink_(statusString)
          → PluginEditor::pushStatus()
            → webView_.eval("window.updateVueState({type:'status', data:'...'})") 
```

The `statusSink_` is a `std::function<void(std::string)>` registered by the
editor in its constructor and cleared in its destructor.
`KiwiClient::StateCallback` (`setOnOpen`, `setOnError`, `setOnClose`) is wired
in `PluginProcessor::connectToStation`.

_Files: `source/network/kiwi_client.h` (StateCallback), `source/vst/processor/plugin_processor.cpp` (emitStatus, connectToStation), `source/vst/controller/plugin_controller.cpp` (statusSink\_), `source/editor/plugin_editor.cpp` (pushStatus)_

## 6. Protocol / Handshake

> **Vollständige Protokoll-Referenz:** `doc/kiwisdr-protocol-reference.md`
> (aus KiwiSDR-Server-Quellcode + gr-kiwisdr/supersdr/kiwiclient abgeleitet).
> Enthält: `CMD_SND_ALL`-Gate, Kick-Mechanismen (`too_busy`,
> `connection_hang`), SND-Frame-Format, Keepalive-Regeln und die
> Server-Policy-Analyse des ~11-s-Kicks.

Connection via WebSocket (the default API-ready test station is
`kphsdr.com:8073`; the port is configurable via
`setServer()`, e.g. some Kiwis use non-standard ports). Note: `kphsdr.com:8072`
was the original UI default and worked until 2026-08-27 (verified FIX-43), but
the server no longer answers on 8072 (TCP timeout, 2026-08-30); 8073 answers
with HTTP 200. M5 will derive the port from a dynamic station list. The protocol
is ASCII `SET ...` text frames (reference: `jks-prv/kiwiclient` and the KiwiSDR
server `rx/rx_cmd.cpp`).

**Phase 1 (on open):**
- `SET options=1` — marks the connection as external/non-local. **Must be sent
  before auth** (kiwiclient `open()`).
- **Authentication (anonymous):** `SET auth t=kiwi p=`. No user name / password
  required, so the plugin works without any user configuration.

**Phase 2 (triggered by the server's `audio_rate=<rate>` MSG):**
- `SET AR OK in=<rate> out=<rate>` — **this is the command that activates the
  SND audio stream.** `in`/`out` are parsed from the advertised `audio_rate=`
  (typically 12000), never hard-coded. `out` == `in` requests the raw source
  rate (no server-side resampling) because the plugin's own Resampler converts
  source rate → DAW rate.
- Optional `SET ident_user=<name>` (only when a user name is configured).
- `SET squelch=0 max=0`, `SET genattn=0`, `SET gen=0 mix=-1` (init squelch off,
  generator off — reference sequence).
- **Tuning:** `SET mod=<mode> low_cut=<lc> high_cut=<hc> freq=<kHz>` (e.g.
  `SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000`).
- **AGC:** `SET agc=<0|1> hang=<0|1> thresh=<dB> slope=<dB> decay=<ms> manGain=<dB>`.

**Steady state:**
- **Keepalive:** `SET keepalive` (periodic, 1 Hz; the server drops idle
  connections). Do NOT send it faster (server treats it as spam and drops).
- **Audio stream:** receive binary `SND` frames with IMA ADPCM-encoded
  sub-frames, decode them and write to the audio buffer.

> Note: the earlier draft mentioned `SET user`/`SET inert`; those are not part
> of the real protocol.

<!-- Vermerk 2026-08-27 (FIX-41, ENDGÜLTIG): -->
> **Operational note — `n_snd=0` was a probe bug, not a client/server bug.**
> The ~10-s `CLOSE 1005` and `n_snd=0` were caused by a bug in the **Python
> probe** (`probe_full.py`): the server sends `sample_rate=` **before**
> `audio_rate=`, and the probe's phase-2 latch swallowed the `SET AR OK` command
> (which activates the SND stream). The C++ client was functionally correct and
> has since been hardened to the reference sequence (options=1 before auth,
> parsed AR OK rate, squelch/gen init, bogus `SERVER DE CLIENT` frame removed).
> Live probe against kphsdr.com:8073 confirms SND frames flow. See
> `doc/checklist.md` FIX-41.

## 7. Multi-Threaded Architecture (Real-Time Safety)

To prevent DAW dropouts (clicks) and crashes, the system is strictly split
into three threads:

1. **GUI Thread (WebView)** - user interaction + rendering; non-blocking OS
   webview.
2. **Audio/DSP Thread (VST3)** - strict real-time constraints; must be
   completely **lock-free, allocation-free, network-free**. Retrieves decoded
   samples via moodycamel lock-free SPSC queue.
3. **Network/Worker Thread** - manages the WebSocket connection, receives
   packets, decodes ADPCM, pushes sample frames into the SPSC queue.

### Safety mechanisms

- **Jitter buffer:** collect 100-150 ms of audio on connect to cushion network
  jitter.
- **LFO rate-limiting:** cap parameter updates sent to the KiwiSDR server at
  20 Hz (spam protection).

## 8. Implementation Phases

1. **Phase 1 - CLI Prototype:** network connection + ADPCM decoding via
   IXWebSocket.
2. **Phase 2 - VST3 Skeleton:** basic VST3 plugin structure + integrate the
   lock-free SPSC queue.
3. **Phase 3 - DSP Integration:** sample-accurate parameter modulation,
   rate-limiting, sample-rate conversion (libsamplerate).
4. **Phase 4 - WebView & Vue Integration:** embed the webview container into
   the VST3 editor window + bidirectional JSON communication.
5. **Phase 5 (M3) - Integration & Ship:** wire the full
   `KiwiClient → ImaAdpcmDecoder → AudioSampleQueue → Resampler →
   JitterBuffer → process()` pipeline into the processor; add the complete
   KiwiSDR parameter set (28 VST3 parameters).
6. **Phase 6 (M4) - UI parity:** 1:1 re-implementation of the KiwiSDR browser
   UI in Vue; freely resizable editor window (drag bottom-right corner).
7. **Phase 7 (M5) - Station selection:** tab-based UI ("SDR Stations" /
   "KIWI UI") with a scrollable station directory and click-to-connect.

> Phases 1–4 correspond to M1/M2; phases 5–7 to M3/M4/M5 (see
> `doc/plan.md` / `doc/checklist.md`).

## 9. Conventions

- All agent rules / documentation in English.
- Knowledge is synced across: `doc/` <-> RAG/Wiki MCP (`netsdr_rag`) <->
  NotebookLM (NetSDRStation-VST).
- Work MCP-first (see `AGENTS.md` / `doc/index.md`).
- Cross-platform build, VST host debugging, and Vue hot-reload workflow:
  see `doc/workspace-workflow.md`.
- Coding rules: Clean Code Developer (CCD), see `doc/coding-standards.md`.
  Violations must be justified in the task summary.
- Licensing: see `doc/framework-licensing.md` (license-free frameworks only;
  JUCE orientation allowed for ideas/architecture, code never copied).
- UI inventory & design: see `doc/ui-architecture.md` (complete KiwiSDR web
  UI element list, VST3 parameter mapping, scope decisions, tab layout).
- Implementation plans: `doc/M3-implementation-plan.md` (integration & ship),
  `doc/M4-implementation-plan.md` (UI parity), `doc/M5-implementation-plan.md`
  (station selection). Each contains the chronological step order, alternatives
  and risk registers for its milestone.
- First milestone: generic VST foundation (forkable) + sine synth proof, see
  `doc/plan.md`.

## 10. Architecture goals

- **Modular, robust architecture** that can serve as a foundation for other
  VSTs (all platforms).
- Milestone 1 produces a **generic VST shell** (entry point, processor/
  controller base, parameter registry, threading, WebView editor) fully
  separated from the DSP core; it is a forkable git checkpoint.
- Project-specific (KiwiSDR) details start only at Milestone 2.
- Follow CCD (component orientation, separation of concerns, SRP, DIP, etc.).

## 11. Audio Pipeline (Milestone M3, implemented)

> Cross-reference: `doc/M3-implementation-plan.md` (implementation status,
> deviations and fixes), `doc/checklist.md` M3.

After M3 the shipped `.vst3` connects to a real KiwiSDR server, decodes the
audio and exposes the full parameter set to the DAW. The M1 sine oscillator is
retired from `process()` (the class remains for its tests).

### Pipeline (as implemented)

```
Network thread (IXWebSocket):
  KiwiClient --SND binary frame--> ImaAdpcmDecoder --int16 PCM-->
    AudioSampleQueue (lock-free SPSC, bounded tryPush, drop-newest)

Audio thread (process()/renderPipeline):
  AudioSampleQueue --pop--> Resampler (12 kHz -> DAW rate, bounded staging)
    --> JitterBuffer (pre-allocated ring, drop-oldest, 100 ms prefill latch)
    --> output (silence on underflow, volume/mute applied)
```

Two **deviations** from the original plan (rationale in
`doc/M3-implementation-plan.md` §"Technical deviations"):

1. The jitter buffer runs at the **DAW sample rate AFTER the resampler** (not
   before), so `process()` pulls exactly `numSamples` per block.
2. Connection is **lazy**: the processor connects only when a station is set
   (`setState`/`applyState` or a `setStation` bridge message), never on
   `initialize()`.

### Sample rate: must ALWAYS come from the DAW (JUCE reference, 2026-08-27)

`doc/plan.md` §M3 status entry FIX-40 documents the "sample rate always pulled
from the DAW" rule. This subsection records the framework analysis behind it.

**Reference: JUCE (`thirdParty/JUCE`, `juce_audio_processors`).**

- `AudioProcessor::prepareToPlay(double sampleRate, int maxSamplesPerBlock)` is
  the **pure virtual host→plugin callback** the host uses to hand the session
  sample rate to the plugin
  (`modules/juce_audio_processors_headless/processors/juce_AudioProcessor.h:139`).
- The host stores the rate internally via
  `AudioProcessor::setRateAndBufferSizeDetails(newSampleRate, newBlockSize)`
  (`juce_AudioProcessor.cpp:376-380`), which assigns
  `currentSampleRate = newSampleRate`.
- Inside `processBlock()`, the plugin reads the current rate with
  `double AudioProcessor::getSampleRate() const noexcept { return currentSampleRate; }`
  (`juce_AudioProcessor.h:825`). **The doc comment explicitly states it is only
  guaranteed valid inside `processBlock` and may return 0 otherwise** — i.e. the
  sample rate is fundamentally a callback-time value supplied by the DAW, never a
  hard-coded project constant.
- Host side, JUCE's own non-DAW host (`AudioProcessorPlayer`) re-pulls the rate on
  every audio-device start: `audioDeviceAboutToStart` reads
  `device->getCurrentSampleRate()` and calls `prepareToPlay(sampleRate, blockSize)`
  (`modules/juce_audio_utils/players/juce_AudioProcessorPlayer.cpp:344-355,180`).
  In a real VST3 host the same rate flows into the plugin through the host's
  `IAudioProcessor::setAudioProcessor`/`setupProcessing` handshake.
- **Dynamic change:** when the user changes the sample rate in the DAW mid-session,
  the host re-invokes the setup path with the new rate (JUCE re-calls
  `prepareToPlay`; VST3 hosts call `setupProcessing` with a new `ProcessSetup`).
  The plugin must therefore treat its output sample rate as **re-initialisable**
  and rebuild any rate-dependent state (resampler, jitter buffer, clock-drift
  baseline) when that happens.

**Reference: VST3 SDK (`thirdParty/vst3sdk`).**

- `IAudioProcessor::setupProcessing(ProcessSetup& setup)` is the VST3 equivalent
  of JUCE's `prepareToPlay`. `ProcessSetup` carries
  `int32 maxSamplesPerBlock` and `SampleRate sampleRate`
  (`pluginterfaces/vst/ivstaudioprocessor.h:174-182`); `SampleRate` is
  `typedef double` (`vsttypes.h:112`). The host calls `setupProcessing` when the
  processing config (sample rate / max block) changes.

**Our plugin (already correct, verify on any reconnect/setup re-entry):**

- `PluginProcessor::setupProcessing` re-creates the `Resampler` and `JitterBuffer`
  from `newSetup.sampleRate` and re-baselines `smoothedRatio_` to
  `newSetup.sampleRate / 12000.0` (`source/vst/processor/plugin_processor.cpp:612-625`).
  Because the KiwiSDR stream is fixed at 12 kHz, the DAW output rate is the only
  free variable and it is taken from the host.

**Open question (FIX-40):** whether `setupProcessing` is guaranteed to be
re-invoked by every host on a mid-session sample-rate change, and whether the
clock-drift `smoothedRatio_` (which the M3.6 log-analysis showed drifting to
~4.0 instead of nominal ~3.675) is reset reliably on reconnect — see
`doc/checklist.md` FIX-40 and `doc/plan.md` §M3 status.

### Real-time safety (M3.4 audit)

- Audio thread is **lock-free, allocation-free, network-free**:
  - `JitterBuffer` = pre-allocated ring buffer; `push`/`pull`/`reset`
    allocation-free (constructor allocates once).
  - `Resampler` = bounded staging buffer (`kMaxStagingFrames = 4096`, reserved
    in the constructor); in-place `memmove` compaction; oversized inputs are
    processed in an internal chunked loop.
  - `AudioSampleQueue::pop` = moodycamel lock-free SPSC.
  - `ParameterRegistry::setValue` = lock-free O(1) `unordered_map` lookup
    (FIX-35 resolved).
- Jitter buffer pre-fill is a **start latch**: it only gates the first pull;
  once playing, `pull` returns whatever is available (0 on underflow) so a
  mid-stream dip below the 100 ms target never re-arms a burst of silence.

### Parameter set (27 DAW-automatable VST3 parameters)

`kParamStation` is **not** a VST3 parameter (VST3 params are double-typed); the
station is plugin state serialized in `getState`/`setState` and set via the
`setStation` bridge message (Option A). Groups:

- **Core:** `mode` (enum 0..17), `freqKhz` (0.001..30000 kHz), `lowCut`
  (-8000..0 Hz), `highCut` (0..8000 Hz).
- **AGC:** `agcOn`, `agcHang`, `agcThresh` (-130..0 dB), `agcSlope` (0..10 dB),
  `agcDecay` (20..5000 ms), `agcManGain` (0..120 dB).
- **Audio:** `volume` (0..1), `mute`, `squelchOn`, `squelchThr` (0..1), `nbOn`,
  `nbThresh` (0..1), `nrOn`, `deempOn`, `compOn`.
- **Display/Waterfall:** `wfOn`, `wfSpeed` (enum 0..3), `wfZoom` (0..14),
  `wfMaxDb` (-10..0), `wfMinDb` (-160..-60), `wfComp`, `arOn`, `ovOn`.

Full definitions: `source/vst/common/paramdefinitions.h`, `paramids.h`.

## 12. WebView2 Bundle Deployment

The plugin is self-contained — it works without any system-wide WebView2 installation.
All WebView2 dependencies are bundled inside the VST3 package.

### Fixed Version Runtime

The WebView2 Fixed Version Runtime (`Microsoft.WebView2.FixedVersionRuntime.151.0.4129.93.x64/`)
and `WebView2Loader.dll` are copied into the VST3 bundle (`Contents/x86_64-win/`)
at build time via CMake POST_BUILD commands (`source/entry/CMakeLists.txt`).

### Runtime Discovery

On plugin load, `WebViewHost::Impl::attach()` (`source/webview/webview_editor.cpp`):
1. Resolves the **plugin DLL** directory via
   `GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), ...)`.
   Using `__ImageBase` (the plugin DLL's own image base) is critical — passing
   `nullptr` would return the **host executable** path, pointing the runtime
   discovery at the wrong directory (BUG-01 root cause, fixed).
2. Sets the environment variable `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` to
   `<moduledir>/FixedRuntime`
3. Constructs the `webview::webview` object — `WebView2Loader.dll` reads the
   env var to locate the bundled runtime instead of the Windows Registry
4. Resets the env var after construction

### Release UI URL

In release builds, the editor derives the UI URL at runtime from the module path:
- `PluginEditor::buildReleaseUiUrl()` (`source/editor/plugin_editor.cpp`)
  calls `GetModuleFileNameW`, strips the filename, and builds
  `file:///<moduledir>/ui/index.html`

### Pre-build Requirement

The Vue UI must be built before the VST3 release build:
```
cmake --build <build-dir> --target netsdrstation_ui
```
This target runs `vite build` and produces `ui/dist/`, which CMake copies into
the VST3 bundle via POST_BUILD.
