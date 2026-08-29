# NetSDRStation-VST — Chronological Log

_Append-only, newest first. Parseable with `grep "^## "`. Entries use
`**Creation**`, `**Update**` or `**Deprecation**` prefix + linked concept file._

---

## 2026-08-29 — LLM-Wiki refactoring design doc archived

**Update:** [`LLM-WIKI-Refactoring.md`](./archive/LLM-WIKI-Refactoring.md) — Moved from `doc/` to `doc/archive/` as all 5 phases are complete

**Update:** [`index.md`](./index.md) — Link updated to `./archive/LLM-WIKI-Refactoring.md`

**Update:** [`checklist.md`](./checklist.md) — Reference updated to archive path

Outcome: The refactoring design document is archived. The wiki structure (Karpathy LLM-Wiki + Google OKF v0.2) is now the permanent standard — no further structural changes needed unless the design doc is revisited.

## 2026-08-29 — LLM-Wiki Phase 4+5: Lint-Workflow + NotebookLM-Sync deployed

**Update:** [`AGENTS.md`](../AGENTS.md) — Added Wiki Lint Workflow section (6 checks: orphans, duplicates, stale claims, contradictions, cross-refs, gleanings) + first lint pass completed; Knowledge-Sync section rewritten with deterministic roles and weekly NotebookLM sync workflow; lint now runs **automatically** as part of every Post-Task Sync

**Update:** [`index.md`](./index.md) — Fixed orphan page (`archive/plan-history.md` added), removed duplicate entry (`M4c.7-bugs.md` listed twice)

**Update:** [`ui-architecture.md`](./ui-architecture.md) — Resolved contradiction: TagPopup status changed from ❌ to ✅ (component `TagPopup.vue` exists; remaining data issues noted under Bug 4)

**Update:** [`checklist.md`](./checklist.md) — W8 (NotebookLM sync) and W9 (Lint workflow) marked done; phase-4/5-deferred note removed

**Update:** [`LLM-WIKI-Refactoring.md`](./LLM-WIKI-Refactoring.md) — Status changed from "Plan only" to "IMPLEMENTED — all phases complete"

Outcome: All 5 phases of the LLM-Wiki refactoring are now complete:
- Phase 1 (OKF schema): ✅ deployed 2026-08-29
- Phase 2 (entanglement): ✅ deployed 2026-08-29
- Phase 3 (RAG extension): ✅ deployed 2026-08-29
- Phase 4 (Lint workflow): ✅ deployed now
- Phase 5 (NotebookLM sync): ✅ deployed now
- A lint script `doc/lint.ps1` implements checks 1–5

## 2026-08-29 — M4c.7 Bug-Manifest: UI-Paritäts-Gap erfasst

**Update:** [`checklist.md`](./checklist.md) — M4c.7 Bug-Katalog: 6 Bugs (Tip-Panel, Spektrometer, Frequenzband-Leiste, DX-Tags, kHz-Lineal, Bedienpanel) + E2E-Test-Qualitätsanalyse + Extensions-Planung als separate M-Phase
**Update:** [`plan.md`](./plan.md) — M4 erweitert um M4c.7 (6 Analyse-Tasks + Fix-Phasen) + Extensions-Phase skizziert
**Creation:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Vollständiges Bug-Manifest mit Referenz-Daten-Abgleich, E2E-Lückenanalyse, 1:1-Paritäts-Strategie
**Update:** [`index.md`](./index.md) — M4c.7-Eintrag hinzugefügt

## 2026-08-29 — LLM-Wiki refactoring deployed

**Creation:** [`index.md`](./index.md) — Karpathy-style knowledge catalog (1-line summaries, categories)
**Creation:** [`log.md`](./log.md) — This chronological changelog (append-only, newest first)
**Update:** All `doc/*.md` concept files — Added OKF YAML frontmatter (`type`, `title`, `description`, `status`, `generated`, `sources`, `verified`, `stale_after` for time-sensitive files)
**Update:** [`plan.md`](./plan.md) — Historical status blocks moved to [`archive/plan-history.md`](./archive/plan-history.md) (Single Source of Truth; plan.md now describes only current state)
**Update:** [`netsdr_mcp_server.py`](../netsdr_mcp_server.py) — YAML frontmatter awareness: `_parse_frontmatter()`, `_chunk_markdown` supports frontmatter chunks, wiki generation shows Type/Status/Stale-after per concept file (RAG schema v6)
**Update:** [`AGENTS.md`](../AGENTS.md) — Primary navigation now `doc/index.md` first, then detailed docs; Quick facts updated
**Update:** [`WORKSPACE_AGENT_PROMPT.md`](../WORKSPACE_AGENT_PROMPT.md) — MCP-First workflow updated: `doc/index.md` as primary entry point, Knowledge-Sync references all wiki artifacts
**Creation:** [`archive/plan-history.md`](./archive/plan-history.md) — Archived M2/M3 historical status blocks from plan.md

Outcome: The `doc/` directory now implements the Karpathy LLM-Wiki + Google OKF v0.2 structure:
- `index.md` = deterministic first navigation (catalog → concept file)
- `log.md` = chronological append-only changelog
- Each concept file has machine-readable YAML frontmatter (type, status, provenance, freshness)
- RAG/wiki indexer is frontmatter-aware (schema v6, 19 frontmatter chunks)
- Single Source of Truth: no duplicated status blocks across files
- NotebookLM sync deferred (per user request)

## 2026-08-28 — M4 Bug audit complete

**Creation:** [`M4b-bugs.md`](./archive/M4b-bugs.md) — Bug list & analysis after user review of M4 UI state
**Creation:** [`M4-ui-replication-analysis.md`](./M4-ui-replication-analysis.md) — Full analysis of KiwiSDR browser UI for 1:1 Vue replica

## 2026-08-27 — M3 complete, M4 UI started

**Update:** [`checklist.md`](./checklist.md) M3.5 — Manual acceptance passed (connection stable, frequency/passband/volume/disconnect verified)
**Update:** [`checklist.md`](./checklist.md) M3.7 — Refactoring complete (`plugin_processor.cpp` → 3 files)
**Update:** [`plan.md`](./plan.md) — M3 status: M3.1–M3.7 done (92/92 tests green, Validator 47/47)
**Update:** [`plan.md`](./plan.md) — FIX-40/FIX-41/M3 Blocker resolved: n_snd=0 was probe bug, not client bug

## 2026-08-26 — M3 defects resolved

**Update:** [`checklist.md`](./checklist.md) — BUG-03 (Connect button), BUG-04 (Winsock include-order), BUG-05 (missing dependsOn) fixed
**Update:** [`checklist.md`](./checklist.md) — F2 KiwiSDR connection fix: auth-first handshake, Phase 2 command sequence, 20-byte SND header stripping

## 2026-08-25 — M3 Clean Build fix

**Update:** [`checklist.md`](./checklist.md) — BUG-04 (Winsock include-order) fixed; Clean-Build Debug+Release green
**Update:** [`checklist.md`](./checklist.md) — BUG-05 (start-testhost-debug missing dependsOn) fixed

## 2026-08-22 — M3 RT-safety, full test suite

**Update:** [`plan.md`](./plan.md) — M3.1–M3.4, M3.6, M3.7 done (86/86 C++ tests, 28/28 Vitest, Playwright smoke)
**Update:** [`checklist.md`](./checklist.md) — RT-safety fixes: allocation-free JitterBuffer ring buffer, bounded Resampler, prefill start latch

## 2026-08-21 — M1 quality review complete

**Creation:** [`M3-implementation-plan.md`](./M3-implementation-plan.md) — M3 integration & ship plan
**Update:** [`checklist.md`](./checklist.md) — M1 corrections FIX-01 through FIX-25 all resolved
**Update:** [`checklist.md`](./checklist.md) — TEST-01 through TEST-08 complete (37/37 unit tests)

## 2026-08-19 — M1 quality review findings

**Update:** [`checklist.md`](./checklist.md) — M1 corrections discovered: 6 Critical, 7 Important, 8 Minor issues identified

## 2026-08 — M1 completion, M2 implementation

**Update:** [`plan.md`](./plan.md) — M1 complete: generic VST foundation (forkable checkpoint), all platforms
**Update:** [`plan.md`](./plan.md) — M2 complete: KiwiSDR components (network/decode/resample/DSP, unit-tested)

## 2026-07 — Project inception

**Creation:** [`architecture.md`](./architecture.md) — Initial architecture document
**Creation:** [`plan.md`](./plan.md) — Initial draft plan with milestones M1–M5
**Creation:** [`checklist.md`](./checklist.md) — Initial task list