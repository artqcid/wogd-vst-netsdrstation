# NetSDRStation-VST - Architecture

_Detailed architecture knowledge. Source: NotebookLM "NetSDRStation-VST"
(project concept V3). Manually maintained; complements the auto-generated
`doc/code_wiki.md` (symbol index, MCP-only) and `doc/plan.md` (draft plan)._

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

## 6. Protocol / Handshake

Connection via WebSocket on **port 8073** (the concrete test station
`g8ure.ddns.net` uses port 8078). The protocol is ASCII `SET ...` text frames
(reference: `jks-prv/kiwiclient` and the KiwiSDR server `rx/rx_cmd.cpp`).

- **Authentication (anonymous):** send `SET auth t=kiwi p=`. This needs no user
  name and no password, so the plugin works without any user configuration.
- **Optional user identity:** `SET ident_user=<name>` — only sent when a user
  name is explicitly configured.
- **Tuning:** `SET mod=<mode> low_cut=<lc> high_cut=<hc> freq=<kHz>` (e.g.
  `SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000`).
- **AGC:** `SET agc=<0|1> hang=<0|1> thresh=<dB> slope=<dB> decay=<ms> manGain=<dB>`.
- **Keepalive:** `SET keepalive` (periodic; the server drops idle connections).
- **Audio stream:** receive binary `SND` frames with IMA ADPCM-encoded
  sub-frames, decode them and write to the audio buffer.

> Note: the earlier draft mentioned `SET user`/`SET inert`; those are not part
> of the real protocol. The implemented commands are `auth`, optional
> `ident_user`, `mod ... freq`, `agc`, `keepalive`.

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

## 9. Conventions

- All agent rules / documentation in English.
- Knowledge is synced across: `doc/` <-> RAG/Wiki MCP (`netsdr_rag`) <->
  NotebookLM (NetSDRStation-VST).
- Work MCP-first (see `AGENTS.md` / `doc/checklist.md`).
- Cross-platform build, VST host debugging, and Vue hot-reload workflow:
  see `doc/workspace-workflow.md`.
- Coding rules: Clean Code Developer (CCD), see `doc/coding-standards.md`.
  Violations must be justified in the task summary.
- Licensing: see `doc/framework-licensing.md` (license-free frameworks only;
  JUCE orientation allowed for ideas/architecture, code never copied).
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

## 8. WebView2 Bundle Deployment

The plugin is self-contained — it works without any system-wide WebView2 installation.
All WebView2 dependencies are bundled inside the VST3 package.

### Fixed Version Runtime

The WebView2 Fixed Version Runtime (`Microsoft.WebView2.FixedVersionRuntime.151.0.4129.93.x64/`)
and `WebView2Loader.dll` are copied into the VST3 bundle (`Contents/x86_64-win/`)
at build time via CMake POST_BUILD commands (`source/entry/CMakeLists.txt`).

### Runtime Discovery

On plugin load, `WebViewHost::Impl::attach()` (`source/webview/webview_editor.cpp`):
1. Resolves the module directory via `GetModuleFileNameW`
2. Sets the environment variable `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` to
   `<moduledir>/FixedRuntime`
3. Constructs the `webview::webview` object — WebView2 uses the env var to
   locate the bundled runtime instead of the Windows Registry
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
