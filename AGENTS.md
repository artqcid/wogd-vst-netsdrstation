# AGENTS.md

## Language Rule

All agent rules, instructions, and agent-facing documentation in this workspace
**must be written in English**. This applies to AGENTS.md, skill files
(`.opencode/skills/**/SKILL.md`), agent system prompts, and any other
agent-facing configuration.

## Mandatory Workflow (single source of truth for ALL agents)

This section is the **single source of truth** for every workflow rule in this
workspace. It applies to all workspace agents: `ARCHITECT`,
`ARCHITECT_Openrouter`, `BUILD`, `BUILD_Openrouter`, `DEV`, `DEV_OpenRouter`,
`DEV_JUNIOR_Openrouter`. The global agents `build` and `plan` are NOT
workspace-specific and must never receive workspace-specific prompts or settings.

### Autopilot mode (no permission prompts)

- **No permission prompts.** All tools are allowed (edit, bash, read, glob,
  grep, list, task, todowrite, question, webfetch, websearch, skill, external_directory).
- **Work outside the workspace is always allowed** without asking.
- **Todo-first workflow (mandatory):** before starting ANY work on a task:
  1. Break the task into concrete, ordered steps.
  2. Create a todo list via `todowrite` with each step marked `pending`.
  3. Present the plan to the user in the response.
  4. **Wait for user confirmation** before executing the first step.
  5. Update todo status to `in_progress` / `completed` as work proceeds.
- **Once the plan is confirmed and execution begins**, never ask for
  permission again — proceed autonomously until the task is complete.

### MCP-First workflow (RAG / Code-Wiki)

The workspace provides a local RAG + Code-Wiki MCP server (`netsdr_rag`,
see `netsdr_mcp_server.py`). Tools: `index_project_code`, `query_code_rag`,
`query_code_wiki`, `get_rag_chunk` (called with server prefix, e.g.
`netsdr_rag_query_code_wiki`).

1. **`doc/index.md`** — first place to look: find the relevant concept file
   (architecture, plan, checklist, etc.).
2. `doc/architecture.md` — detailed architecture knowledge (read directly).
3. **`query_code_wiki("<symbol>")`** — signature, file, line number.
4. **Only if knowledge is missing:** `query_code_rag(..., format="compact")`.
5. **Only load the needed chunk:** `get_rag_chunk("<id>")`.
6. Verify in the real code (path + line).
7. **After a change:** `index_project_code` — wiki stays current.

**MCP-FIRST (no exceptions):**
- `doc/code_wiki.md` must NEVER be loaded via `read()` — query via MCP.
- Every agent with MCP access MUST use `query_code_wiki` / `query_code_rag` / `get_rag_chunk`.
- Project and SDK files should be read only with `offset`/`limit` — never whole files.
- Anything found once via MCP is never searched again.

### Subagent rules

**Delegation:**
- Escalate architectural questions upward: DEV → BUILD → ARCHITECT.
- Delegate small, focused implementation/search tasks to subagents
  (`general`, `explore`); subagents never build/test (see below).

**Build & test ownership:**
- **Subagents must NEVER build or run tests.** This is always the job of the
  primary agent.
- When a subagent has finished implementing, the **primary agent** takes over
  building and/or running the tests.
- If build or test errors occur, the fix is delegated **back to a subagent**.
- Give subagents small, focused tasks; the primary agent always verifies the
  result (code review + build + test).

**MCP access:**
- **Subagents have MCP access to `netsdr_rag`** (RAG + Code-Wiki tools:
  `query_code_wiki`, `query_code_rag`, `get_rag_chunk`). They may use these
  directly — the primary agent no longer needs to pre-fetch context.
- **Subagents have access to GitHub MCP read-only tools** (`github_search_code`,
  `github_get_file_contents`, `github_list_commits`, `github_search_repositories`,
  etc.) for analyzing code on GitHub. Write tools (create PR, push, branch, issue)
  remain primary-only.
- **`clangd_mcp` and `notebooklm_devblogs` remain primary-only** — subagents
  must not access these servers.
- **After a subagent finishes**, the primary agent runs `index_project_code`
  to keep the wiki current (subagents cannot do this themselves).

### Definition of Done

A task is complete only when ALL of the following hold:

1. Compiles: `cmake --preset win-msvc`, then
   `cmake --build build/win-msvc --config Debug` (and `Release`).
2. Tests green: `ctest --test-dir build/win-msvc -C Debug --output-on-failure`.
3. `index_project_code` ran (wiki current).
4. `pwsh doc/lint.ps1` ran without new issues.
5. `doc/log.md` appended with a changelog entry (newest first).

Release builds need the UI target first (`--target netsdrstation_ui`); full
build/debug/hot-reload workflow: `doc/workspace-workflow.md`.

**Post-Task Sync (after each completed task):**
- Run `index_project_code` so the wiki stays current.
- Run `pwsh doc/lint.ps1` to check for orphan pages, stale claims,
  duplicate entries and contradictions.
- If not possible (no MCP access): explicitly report that sync is pending.

## Project Overview

`wogd-vst-netsdrstation` is a native VST3/AU/CLAP audio plugin that streams SDR
audio from KiwiSDR servers (WebSocket port 8073, IMA ADPCM) into a DAW.
Three-thread model: GUI / DSP / network worker. C++20 + VST3 SDK + CMake.
Full detail: `doc/architecture.md`.

## Role & Delegation Model

Primary agents form a responsibility hierarchy:

- **ARCHITECT / ARCHITECT_Openrouter** — decides and validates architectural
  questions.
- **BUILD / BUILD_Openrouter** — senior dev; considers every change project-wide.
- **DEV / DEV_OpenRouter** — implements the assigned task, stays scoped.
- **DEV_JUNIOR_Openrouter** — small, well-defined, self-contained tasks.

## Project Main Notebook (NotebookLM devblogs)

The main notebook for this project is **NetSDRStation-VST**.

- **Notebook ID:** `8b6898aa-c3f4-4a89-8304-da9af60cf0e4`
- **URL:** https://notebook.google.com/notebook/8b6898aa-c3f4-4a89-8304-da9af60cf0e4
- **MCP Server:** `notebooklm_devblogs`
- **Source count:** 19

All NotebookLM work for this project (notes, research, reports, sources)
runs through this notebook.

## Wiki Lint Workflow (Phase 4 — automatic, runs on every Post-Task Sync)

The lint script `pwsh doc/lint.ps1` runs automatically as part of every
Post-Task Sync. It can also be run manually at any time. It checks:

1. **Orphan pages**: every file in `doc/` (excluding `archive/`) and
   `doc/archive/` should be listed in `doc/index.md`. Find unlisted files
   with: `Get-ChildItem doc/*.md, doc/archive/*.md | where { $_ -notmatch
   '(index|log|code_wiki) ' }` and cross-check against `index.md`.
2. **Duplicate index entries**: grep `index.md` for duplicate links
   (same `./file.md` appearing more than once).
3. **Stale claims**: for each file with `stale_after:` in frontmatter,
   check if `today >= stale_after`. If stale, add a `! STALE` warning to
   the entry in `index.md` and flag for human review.
4. **Contradictions**: identify claims about the SAME feature that differ
   across files (e.g. "TagPopup ✅" vs "❌"). When found, determine the
   actual truth from the code and update the outdated file.
5. **Cross-reference health**: files marked `status: deprecated` should
   have a redirect note or be moved to `archive/`.
6. **Gleanings**: after any significant analysis or debugging session,
   file the findings back into the wiki (new concept file or update to
   an existing one) — Karpathy's "good answers go back into the wiki".

**First lint pass (2026-08-29):** All orphan/duplicate/contradiction issues
resolved. The lint script `doc/lint.ps1` implements checks 1–5 above.

## Knowledge-Sync (Docs <-> RAG/Wiki MCP <-> NotebookLM)

All project knowledge is ALWAYS kept in sync across three stores with clear roles:

1. **Docs (`doc/`)** — OKF-Wiki (primary storage). `doc/index.md` (catalog),
   `doc/log.md` (changelog), individual concept files with YAML frontmatter.
   This is the **compiled knowledge artifact** — agents navigate here first.
2. **RAG/Wiki MCP (`netsdr_rag`)** — Search/symbol layer over `doc/` + source code.
   Run `index_project_code` after every change so the wiki stays current.
3. **NotebookLM (`notebooklm_devblogs`, NetSDRStation-VST notebook)** — Consuming
   / disseminating layer for long-form reports, briefings, and external reference.
   NOT a duplicate of the wiki. Update via `notebooklm_add_text` with a weekly
   `log.md` digest, or push specific analysis/findings as they are completed.

**Deterministic Sync Workflow:**
- After every completed task: update `doc/log.md` + `doc/index.md` + run `index_project_code`.
- Weekly (or on explicit `sync-notebooklm`): push the week's `log.md` entries
  as an aggregated note to the NetSDRStation-VST notebook.
- If drift is detected between stores, resolve by treating `doc/` as the
  authoritative source and updating RAG + NotebookLM from it.

## Ad-hoc / scratch compilation (no artifacts in the repo root)

Ad-hoc/scratch compile checks (single-file type-checks, throwaway "probe"
programs) are allowed during debugging, but must NEVER leave compiler
artifacts (`*.obj`, `*.exe`, `*.pdb`, `*.ilk`, `*.iobj`, `*.ipdb`) in the
workspace root or under `source/`.

- Always direct compiler output to a scratch directory via `/Fo` (MSVC) or
  `-o`/`--output` (clang-cl), e.g. the pre-approved temp dir
  `C:\Users\marku\AppData\Local\Temp\opencode`.
- Run the compiler with the scratch dir as the working directory (or use the
  `workdir` parameter) so the CWD is never the repo root.
- Prefer the real CMake build (`cmake --build`) and the existing test suite
  over ad-hoc compile-only checks.
- Delete scratch sources and outputs when done.

Background: MSVC `cl.exe` / `clang-cl` write `.obj` (and on link `.exe`/`.pdb`)
into the current working directory when no output path is given. Running such a
command from the repo root dumps the artifacts there — this is how the stray
`*_probe.obj` / `<source>.obj` files in the repo root appeared during M3
debugging.

## Logging Strategy (Two-Level)

The project uses a two-level file logger (`source/util/file_logger.h`) writing
to `%TEMP%/netsdrstation.log`:

**INFO Level (always enabled, Release + Debug):**
- Connection events (connect/disconnect/error)
- Server configuration (sample rate, handshake)
- Critical errors and exceptions
- First occurrence of issues (first underrun, first overflow, etc.)
- Performance-neutral, minimal overhead

**DEBUG Level (Debug builds only, disabled in Release):**
- Frame-by-frame decoding details
- Pipeline statistics every N frames
- Clock-drift adjustments
- Buffer levels, queue depth
- Verbose diagnostics

**Usage:**
```cpp
NETSDR_LOG_INFO("Critical event: %s", message);   // Always logged
NETSDR_LOG_DEBUG("Frame %d details", frame);      // Debug builds only
```

**Debug workflow:**
- All errors and bugs MUST be analyzed in Debug builds (full logging)
- Release builds log only critical events (production-safe)
- Log file location: `C:\Users\marku\AppData\Local\Temp\netsdrstation.log`

## Quick facts

- RAG MCP server: `netsdr_mcp_server.py`; registered in `opencode.json` under
  `mcp.netsdr_rag` (venv python: `.venv\Scripts\python.exe`).
- **Primary navigation:** `doc/index.md` (LLM-Wiki catalog, first place to look).
- Checklist: `doc/checklist.md` (open tasks, short descriptions).
- Chronological log: `doc/log.md` (append-only, newest first).
- Draft plan: `doc/plan.md`.
- Detailed knowledge: `doc/architecture.md` (read directly).
- Workflow (build/debug/hot-reload): `doc/workspace-workflow.md`.
- Coding rules (Clean Code Developer): `doc/coding-standards.md`.
- Licensing / framework analysis: `doc/framework-licensing.md`.
- Test strategy: `doc/test-strategy.md`.
- Auto-generated knowledge: `doc/code_wiki.md` (ONLY via MCP, never read directly).
- Main NotebookLM notebook: **NetSDRStation-VST** (see above).
- `netsdr_rag.db` is runtime-only (`.gitignore`).
- Log file: `%TEMP%/netsdrstation.log` (INFO in Release, INFO+DEBUG in Debug).

## Global rules

- `~/.config/opencode/rules/no-auto-commit.md`: no git commits/pushes/PRs
  without explicit user request.
