# Workspace Agent Prompt - wogd-vst-netsdrstation

**Role:** Implement and maintain the `wogd-vst-netsdrstation` VST (Native)
project. Work MCP-first against the local RAG + Code-Wiki index.

**Main NotebookLM notebook:** NetSDRStation-VST
(`8b6898aa-c3f4-4a89-8304-da9af60cf0e4`) via `notebooklm_devblogs`.

## 1. Tooling / RAG

Local MCP server `netsdr_mcp_server.py` is registered in `opencode.json`
under `mcp.netsdr_rag` (venv python `.venv\Scripts\python.exe`). After config
changes: restart opencode, then the `netsdr_rag_*` tools appear.

**Tools:** `index_project_code`, `query_code_rag`, `query_code_wiki`, `get_rag_chunk`.

**MCP-First Workflow (mandatory, no exceptions):**
1. **`doc/index.md`** -> Karpathy-style knowledge catalog (find the right concept file)
2. `doc/architecture.md` -> detailed architecture knowledge (manually maintained)
3. `query_code_wiki("<symbol>")` -> signature, file, line number
4. **Only if knowledge is missing:** `query_code_rag(..., format="compact")`
5. **Only load the needed chunk:** `get_rag_chunk("<id>")`
6. Verify in the real code (path + line)
7. **After a change:** `index_project_code` -> wiki stays current

**MCP-MANDATORY (no exceptions):**
- `doc/code_wiki.md` must NEVER be loaded via `read()`.
- Every agent with MCP access MUST use `query_code_wiki` / `query_code_rag` / `get_rag_chunk`.
- Project and SDK files only with `offset`/`limit` - never whole files.
- What was found once via MCP is never searched again.

**Post-Task Sync (after each completed task):**
- Run `index_project_code` after code changes.
- If not possible (no MCP access): explicitly report that sync is pending.

`netsdr_rag.db` is runtime-only (`.gitignore`). `doc/architecture.md` is
committed (manual detailed knowledge). `doc/code_wiki.md` is committed but
NEVER read directly - use `query_code_wiki` via MCP.

## Knowledge-Sync (Docs <-> RAG/Wiki MCP <-> NotebookLM)

All project knowledge is ALWAYS synced across three stores:

1. **Docs:** `doc/index.md` (LLM-Wiki catalog), `doc/architecture.md` (detailed),
   `doc/plan.md` (current plan), `doc/checklist.md` (short tasks),
   `doc/log.md` (chronological changelog).
2. **RAG/Wiki MCP (`netsdr_rag`):** `index_project_code` keeps the wiki current.
3. **NotebookLM (`notebooklm_devblogs`):** push relevant knowledge to the
   **NetSDRStation-VST** notebook.

Applies to **all agents**, automatically after a task completes or on explicit
user command.

## 2. Architecture rules

The project is a native VST3/AU/CLAP plugin connecting to KiwiSDR servers
(WebSocket port 8073, IMA ADPCM, 3-thread model: GUI / DSP / network worker).
Full detail in `doc/architecture.md`; phased plan in `doc/plan.md`.

## 3. Build / Deploy

- TBD: define CMake/VS build and deploy workflow as the project is scaffolded.
- After a build change, re-run `index_project_code` so the wiki reflects it.

## 4. Deliverables

- VST plugin source
- `netsdr_mcp_server.py` (RAG + Code-Wiki MCP)
- `AGENTS.md` (project agent pointer)
- `doc/checklist.md` (open tasks, short descriptions)
- `doc/index.md` (LLM-Wiki knowledge catalog)
- `doc/log.md` (chronological changelog)
- `doc/plan.md` (draft plan)
- `doc/architecture.md` (manual detailed architecture knowledge)
