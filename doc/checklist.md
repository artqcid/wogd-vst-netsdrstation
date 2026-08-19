# NetSDRStation-VST - Checklist

_Open tasks only (short descriptions). Detailed info: `doc/architecture.md`;
draft plan: `doc/plan.md`; workflow: `doc/workspace-workflow.md`;
coding rules: `doc/coding-standards.md`; test strategy: `doc/test-strategy.md`._

## Milestone M1 - Generic VST foundation (forkable checkpoint)

- [x] **M1.1** Cross-platform CMake scaffold (`CMakeLists.txt` + `CMakePresets.json`)
  - Test: CI build matrix (win/mac/linux) configures + builds; `ctest` runs.
- [x] **M1.2** Modular VST shell (entry point, processor/controller base, parameter registry)
  - Test: unit test that base classes instantiate; parameter registry add/get roundtrip.
- [x] **M1.3** Threading: audio thread lock-free + message/worker thread separation
  - Test: SPSC queue stress test (order/no loss/no corruption); clang-tidy (no lock/alloc in process).
- [x] **M1.4** VST3 processor: sine oscillator (phase accumulator)
  - Test: unit test Goertzel peak at expected freq; amplitude == volume; phase-continuous across blocks.
- [x] **M1.5** VST3 edit controller: params `freq`, `volume`, `mute`
  - Test: unit test param ranges/defaults/IDs; `validator` exercises automation.
- [x] **M1.6** Vue 3 + Vite GUI scaffold (mirror `wogd-juce-template-gui-vue`)
  - Test: Vitest smoke (App renders); `vue-tsc` type-check clean.
- [x] **M1.7** webview/webview editor + `pluginService.ts` bridge (no JUCE)
  - Test: Vitest for pluginService with mocked `window.vstHost` (setParameter -> message, onMessage -> callback).
- [x] **M1.8** UI: frequency knob, volume knob, mute button
  - Test: Vitest component tests (knob emits value, mute toggles state).
- [x] **M1.9** Unit tests for DSP core (CCD yellow) + static analysis
  - Test: `ctest` all green; coverage >= 90%; clang-tidy clean.
- [x] **M1.10** Debug host (`editorhost` / `pluginval`) + load the `.vst3`
  - Test: VST3 SDK `validator`/`hostchecker` passes headlessly on all platforms.
- [x] **M1.11** Verify HMR: edit Vue component -> live update in plugin
  - Test: manual (documented) - edit component, change appears live in dev mode.
- [x] **M1.12** Git checkpoint: forkable foundation for new VSTs (all platforms)
  - Test: fresh clone/fork builds on win/mac/linux (CI) + loads in a host.
## M1 Corrections (found in quality review, 2026-08-19)

> Severity: **Critical** = wrong runtime behaviour / broken feature;
> **Important** = quality/reliability risk; **Minor** = cleanup/polish.

### Critical – must fix before any real-use or M2 work

- [x] **FIX-01** `PluginProcessor::setupProcessing` does not update `SineOscillator::sampleRate_`
  → oscillator produces wrong pitch at every sample rate other than 48 kHz.
  _File: `source/vst/processor/plugin_processor.cpp:123`_
  _Fix: call `oscillator_.setSampleRate(newSetup.sampleRate)` (or reset with new rate) in `setupProcessing`._

- [x] **FIX-02** JS→C++ bridge message format broken: webview/webview bind passes arguments as
  a JSON **array** (`["<payload>"]`); `dispatchMessage` wraps it again into
  `{"type":...,"data":["<payload>"]}`. The C++ parser in `onJavaScriptMessage` then
  searches for `"id":"` in the outer envelope, but the actual id/value are
  double-serialized inside an escaped string → `idMark`/`valueMark` are always `nullptr`.
  Fallthrough then calls `setParamNormalized(kParamVolume, 0.0)` for **every** parameter
  change → volume is forced to 0 on any UI interaction.
  _File: `source/editor/plugin_editor.cpp:145-172`, `source/webview/webview_editor.cpp:30-41`_
  _Fix: redesign bridge protocol – pass id/value as separate native args, or parse the
  unwrapped array correctly; add a C++ integration test for `onJavaScriptMessage`._

- [x] **FIX-03** UI sends parameter values as **plain units** (Hz, 0..1 for volume, 0/1 for mute),
  but C++ calls `controller_->setParamNormalized(tag, value)` which expects normalized [0..1].
  → freq = 440 (Hz) is clamped to 1.0 (≈ 20 kHz).
  _File: `ui/src/views/PluginView.vue:26`, `source/editor/plugin_editor.cpp:168`_
  _Fix: normalize to [0..1] in `onJavaScriptMessage` before calling `setParamNormalized`,
  **or** have the JS side send normalized values._
  _(Resolve together with FIX-02 in a bridge redesign.)_

- [x] **FIX-04** `WorkerThread::post()` acquires a `std::mutex` lock (worker_thread.cpp:29).
  The code comment and architecture claim `post()` is safe to call from the audio thread,
  but mutex locking violates real-time safety (may block).
  _File: `source/threading/worker_thread.cpp:29-35`_
  _Fix: replace `std::queue + mutex` with the moodycamel `LockFreeSPSC<Message>` already
  present in the project; use a condition variable only in the worker-side drain loop._

- [x] **FIX-05** Release build uses the Vite dev-server URL (`http://localhost:5173`) – same as
  debug. The `#else` branch has a `// TODO` comment but no actual path. Release plugin UI
  is blank wherever the dev server is not running.
  _File: `source/editor/plugin_editor.cpp:28`_
  _Fix: implement `vite build` step into CMake (e.g. custom target) and serve the `dist/`
  directory via an embedded HTTP server or file URL; set the release URL correctly._

- [x] **FIX-06** `PluginController::setComponentState` returns `kResultOk` without reading the
  state stream → controller-side parameter display is never synchronized after a
  preset/project load. Host automation values and UI display will be out of sync.
  _File: `source/vst/controller/plugin_controller.cpp:48-51`_
  _Fix: deserialize the same fields written by `PluginProcessor::getState` and call
  `setParamNormalized` for each parameter so the controller mirrors the processor state._

### Important – correctness / quality risk

- [x] **FIX-07** `mute` parameter registered with `stepCount = 0` (continuous) because its
  `isBypass` field is `false`. DAW automation lanes will show a smooth rotary knob
  instead of a discrete on/off switch.
  _File: `source/vst/controller/plugin_controller.cpp:31-36`_
  _Fix: set `stepCount = 1` for any parameter whose definition has `max – min == 1.0`
  and whose default is 0 or 1 (i.e. a binary toggle), independent of `isBypass`._

- [x] **FIX-08** `getState`/`setState` serialize `freq` and `volume` as 32-bit `float` with no
  version byte. Precision loss (~7 decimal digits) and no forward/backward compatibility
  when fields are added.
  _File: `source/vst/processor/plugin_processor.cpp:59-81`_
  _Fix: prefix state with a `uint32_t` version number (start at 1); serialize doubles or
  use the IBStreamer double methods._

- [x] **FIX-09** CI `on.push.branches` only lists `main` and `master`; the new development
  branch `NetSDRStation` is missing → pushes on `NetSDRStation` never trigger the
  build/test pipeline.
  _File: `.github/workflows/ci.yml:5`_
  _Fix: add `NetSDRStation` to the branch list, or use `branches-ignore` with exclusions._

- [x] **FIX-10** Linux CI: `gcovr --fail-under 90` covers **all** compiled objects including
  `plugin_editor.cpp` and `webview_editor.cpp` (built as part of the VST plugin, not
  covered by any test) → coverage threshold likely fails on Linux CI.
  _File: `.github/workflows/ci.yml:57`_
  _Fix: scope gcovr to the test-covered sources only
  (`--include 'source/dsp/.*' --include 'source/threading/.*' --include 'source/vst/common/.*'`),
  or lower the threshold until the editor/webview are unit-tested._

- [x] **FIX-11** `.clang-tidy`: `readability-identifier-naming.*` `CheckOptions` entries are
  defined but the `readability-*` check family is **not listed in `Checks:`** → all
  naming-convention rules are silently inactive.
  _File: `.clang-tidy:6-15`_
  _Fix: add `readability-identifier-naming` to the `Checks:` line._

- [x] **FIX-12** `process()` sets `data.outputs[0].silenceFlags = 0` unconditionally, even when
  `mute_ == true`. The host cannot skip the silent output block, wasting CPU.
  _File: `source/vst/processor/plugin_processor.cpp:194`_
  _Fix: set `silenceFlags = (1 << numChannels) - 1` when the oscillator is muted._

- [x] **FIX-13** `Knob.vue:11` has `aria-label="label"` (string literal) instead of
  `:aria-label="label"` (bound prop). All knobs report the accessibility label "label".
  _File: `ui/src/components/Knob.vue:11`_
  _Fix: change to `:aria-label="label"`._

### Minor – cleanup and design hygiene

- [x] **FIX-14** `pluginids.h:24-25`: `kPluginProcessorCID` and `kPluginControllerCID` are
  unused aliases of `kProcessorUID`/`kControllerUID` (dead code). Remove them.

- [x] **FIX-15** `pluginids.h:11-13`: includes `ivstcomponent.h`, `ivstaudioprocessor.h`,
  `ivsteditcontroller.h` unnecessarily – only `funknown.h` is needed for `FUID`.
  Remove the extra includes to reduce compilation time.

- [x] **FIX-16** `WebViewHost` uses raw `new`/`delete` for the pimpl `Impl*`.
  Replace with `std::unique_ptr<Impl>` (pimpl idiom, exception-safe).
  _Files: `source/webview/webview_editor.h:46`, `source/webview/webview_editor.cpp:108-109`_

- [x] **FIX-17** `factory.cpp:31`: subcategory is `"Instrument"` but VST3 convention for a
  synthesizer is `"Instrument|Synth"`. Affects DAW plugin-browser categorisation.

- [x] **FIX-18** `process()` takes only the **last** parameter point per parameter queue,
  ignoring all earlier sub-block sample-accurate automation points.
  _File: `source/vst/processor/plugin_processor.cpp:136-142`_
  _Note: acceptable for M1 sine synth; must be fixed before any sample-accurate
  modulation work (M2.5). Documented in code as deferred to M2.5._

- [x] **FIX-19** `LockFreeSPSC` header comment claims "no allocation after construction" but
  the underlying moodycamel queue **can** reallocate when capacity is exceeded. The
  comment creates a false real-time safety guarantee.
  _File: `source/threading/lock_free_spsc.h:6-8`_
  _Fix: either document the capacity contract clearly, or call `enqueue_or_die` /
  pre-fill capacity._

- [x] **FIX-20** `CMakeLists.txt:14`: option `NS_ENABLE_UI` is declared but never consumed in
  the file – it is dead code. Add a `find_program(NPM npm)` / custom-target block, or
  remove the option.

- [x] **FIX-21** `CMakeLists.txt`: `SMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF` is only set inside
  `CMakePresets.json` (the hidden `base` preset), not in `CMakeLists.txt`. Configuring
  without presets (e.g. CMake GUI or IDE without preset support) builds all SDK examples.
  _Fix: add `set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)` near
  `SMTG_CREATE_PLUGIN_LINK`._


## Milestone M2 - KiwiSDR integration (project-specific)

- [ ] **M2.1** WebSocket connection to KiwiSDR (IXWebSocket, port 8073)
  - Test: integration test against a local mock KiwiSDR server; connect succeeds.
- [ ] **M2.2** Handshake + `SET` commands (`user`, `inert`, `agc`, `freq`)
  - Test: unit test command serialization; mock server asserts received frames.
- [ ] **M2.3** IMA ADPCM decoding in C++
  - Test: unit test known reference vectors + roundtrip (decode/re-encode) within tolerance.
- [ ] **M2.4** Lock-free SPSC queue (moodycamel) network -> DSP
  - Test: unit test queue under stress (order/no loss/no corruption); underflow graceful.
- [ ] **M2.5** Sample-accurate parameter modulation (frequency via automation/LFO)
  - Test: unit test parameter ramp is monotonic + max step below threshold (no zipper).
- [ ] **M2.6** LFO rate-limiting (max 20 Hz)
  - Test: unit test that N updates in T seconds -> at most ~20/s sent.
- [ ] **M2.7** Sample-rate conversion (libsamplerate)
  - Test: unit test sine 12/24 kHz -> 44.1/48 kHz (freq preserved, THD + aliasing below threshold).
- [ ] **M2.8** Jitter buffer (100-150 ms)
  - Test: unit test absorbs configured jitter; overflow drops oldest; no crash.
- [ ] **M2.9** Bidirectional JSON communication (UI <-> EditController <-> DSP)
  - Test: integration test bridge roundtrip (UI -> param -> DSP -> state echo back to UI).
- [ ] **M2.10** UI controls the live receiver frequency
  - Test: manual (DAW listening) + automated (bridge emits `setParameter` with correct value).

## Workflow Goals (standing requirements, see `doc/workspace-workflow.md`)

- [ ] **W1** Cross-platform build (mac/linux/win); Windows tested locally
  - Test: CI matrix builds all platforms; `ctest` green.
- [ ] **W2** VST host debugging available at any time
  - Test: `validator`/`hostchecker` + debugger-attach flow documented and reproducible.
- [ ] **W3** Vue UI debugging + hot reload available at any time
  - Test: Vitest/dev-server smoke + manual HMR check.

## Licensing (standing constraint, see `doc/framework-licensing.md`)

- [x] **L1** Framework/DSP research: only license-free, closed-source-sellable
      - VST3 SDK (MIT), CLAP (MIT), iPlug2 (zlib), DPF (ISC) OK; JUCE/KFR/HISE excluded
- [ ] **L2** Keep every added dependency permissive (no GPL/paid licenses)
  - Test: CI/license-check step lists all deps + licenses; no GPL/paid.
- [ ] **L3** JUCE orientation: ideas/architecture only, never copy code (see framework-licensing.md)
  - Test: review step - no JUCE-derived code; inspiration documented.

## Coding rules (standing, see `doc/coding-standards.md`)

- [ ] **C1** Follow all Clean Code Developer (CCD) rules (red..white)
  - Test: clang-tidy/static analysis + review; coverage >= 90%.
- [ ] **C2** Justify any CCD rule violation in the task summary
  - Test: review step - every violation has a justification recorded.

## AI development helpers (MCP servers, see `doc/test-strategy.md` §9)

- [ ] **T1** clangd-based C++ semantic MCP - adopt once CMake + C++ code exist (M1)
- [ ] **T2** Playwright MCP - adopt from the UI phase (M1.6)

## Workflow (mandatory for coding agents)

1. `doc/checklist.md` -> take the next open task
2. `doc/architecture.md` -> detailed architecture knowledge
3. **`query_code_wiki("<symbol>")`** -> signature, file, line number (MCP)
4. **Only if knowledge is missing:** `query_code_rag(..., format="compact")`
5. **Only load the needed chunk:** `get_rag_chunk("<id>")`
6. Verify in the real code (path + line)
7. **After a change:** `index_project_code` -> wiki stays current

**MCP-FIRST (no exceptions):**
- `doc/code_wiki.md` must NEVER be loaded via `read()` - query via MCP.
- Every agent with MCP access MUST use `query_code_wiki` / `query_code_rag` / `get_rag_chunk`.
- Project and SDK files only with `offset`/`limit` - never whole files.
- Anything found once via MCP is never searched again.

## Knowledge-Sync (Docs <-> RAG/Wiki MCP <-> NotebookLM)

After every completed task (or on manual command), sync project knowledge:

- **Docs:** update `doc/architecture.md` / `doc/plan.md` / `doc/checklist.md`.
- **RAG/Wiki MCP:** run `index_project_code` so the wiki reflects the code.
- **NotebookLM:** push relevant knowledge to the notebook
  **NetSDRStation-VST** (`notebooklm_devblogs`).

This workflow applies to **all agents**, either automatically after task
completion or on explicit user command.
