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
