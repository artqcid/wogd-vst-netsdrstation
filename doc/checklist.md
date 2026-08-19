# NetSDRStation-VST - Checklist

_Open tasks only (short descriptions). Detailed info: `doc/architecture.md`;
draft plan: `doc/plan.md`; workflow: `doc/workspace-workflow.md`;
coding rules: `doc/coding-standards.md`._

## Milestone M1 - Generic VST foundation (forkable checkpoint)

- [ ] **M1.1** Cross-platform CMake scaffold (`CMakeLists.txt` + `CMakePresets.json`)
- [ ] **M1.2** Modular VST shell (entry point, processor/controller base, parameter registry)
- [ ] **M1.3** Threading: audio thread lock-free + message/worker thread separation
- [ ] **M1.4** VST3 processor: sine oscillator (phase accumulator)
- [ ] **M1.5** VST3 edit controller: params `freq`, `volume`, `mute`
- [ ] **M1.6** Vue 3 + Vite GUI scaffold (mirror `wogd-juce-template-gui-vue`)
- [ ] **M1.7** webview/webview editor + `pluginService.ts` bridge (no JUCE)
- [ ] **M1.8** UI: frequency knob, volume knob, mute button
- [ ] **M1.9** Unit tests for DSP core (CCD yellow) + static analysis
- [ ] **M1.10** Debug host (`editorhost` / `pluginval`) + load the `.vst3`
- [ ] **M1.11** Verify HMR: edit Vue component -> live update in plugin
- [ ] **M1.12** Git checkpoint: forkable foundation for new VSTs (all platforms)

## Milestone M2 - KiwiSDR integration (project-specific)

- [ ] **M2.1** WebSocket connection to KiwiSDR (IXWebSocket, port 8073)
- [ ] **M2.2** Handshake + `SET` commands (`user`, `inert`, `agc`, `freq`)
- [ ] **M2.3** IMA ADPCM decoding in C++
- [ ] **M2.4** Lock-free SPSC queue (moodycamel) network -> DSP
- [ ] **M2.5** Sample-accurate parameter modulation (frequency via automation/LFO)
- [ ] **M2.6** LFO rate-limiting (max 20 Hz)
- [ ] **M2.7** Sample-rate conversion (libsamplerate)
- [ ] **M2.8** Jitter buffer (100-150 ms)
- [ ] **M2.9** Bidirectional JSON communication (UI <-> EditController <-> DSP)
- [ ] **M2.10** UI controls the live receiver frequency

## Workflow Goals (standing requirements, see `doc/workspace-workflow.md`)

- [ ] **W1** Cross-platform build (mac/linux/win); Windows tested locally
- [ ] **W2** VST host debugging available at any time
- [ ] **W3** Vue UI debugging + hot reload available at any time

## Licensing (standing constraint, see `doc/framework-licensing.md`)

- [x] **L1** Framework/DSP research: only license-free, closed-source-sellable
      - VST3 SDK (MIT), CLAP (MIT), iPlug2 (zlib), DPF (ISC) OK; JUCE/KFR/HISE excluded
- [ ] **L2** Keep every added dependency permissive (no GPL/paid licenses)
- [ ] **L3** JUCE orientation: ideas/architecture only, never copy code (see framework-licensing.md)

## Coding rules (standing, see `doc/coding-standards.md`)

- [ ] **C1** Follow all Clean Code Developer (CCD) rules (red..white)
- [ ] **C2** Justify any CCD rule violation in the task summary

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
