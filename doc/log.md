# NetSDRStation-VST — Chronological Log

_Append-only, newest first. Parseable with `grep "^## "`. Entries use
`**Creation**`, `**Update**` or `**Deprecation**` prefix + linked concept file._

## 2026-08-29 — AGENTS.md konsolidiert: Single Source of Truth für alle Agenten

**Update:** [`AGENTS.md`](./AGENTS.md) — Alle verbindlichen Workflow-Regeln
(Autopilot/Todo-first, MCP-First, Subagent-Regeln, Definition of Done) in EINE
prominente Sektion **"Mandatory Workflow"** am Dateianfang konsolidiert.
Vorher waren sie über 6 getrennte Sektionen verstreut, was bei schwächeren
Modellen zu Befolgungsverlust führte. Referenz-Sektionen (Project Overview,
Role & Delegation, Wiki Lint, Knowledge-Sync, Logging, etc.) bleiben unterhalb
erhalten.

**Löschung:** `WORKSPACE_AGENT_PROMPT.md` — duplizierte die MCP-First-Regeln
(zweite Quelle); entfernt, um die Single-Source-of-Truth-Garantie zu erzwingen.

**Update:** `netsdr_mcp_server.py` — Kommentare bereinigt (Verweise auf die
gelöschte Datei entfernt).

Hintergrund: Commit `4b841b9` hatte die Regeln aus den Agenten-System-Prompts
entfernt und nur in AGENTS.md belassen ("System-Prompts auf Identität+Rolle
reduziert"); damit verloren sie ihre Prominenz. Die Agenten-Definitionen bleiben
bewusst global (`~/.config/opencode/agent/*.md`, wiederverwendbar), die Regeln
liegen workspace-lokal und zentral in AGENTS.md.

## 2026-08-29 — M4c.7 Bugs implementiert (6 Bugs, 13 Dateien)

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Alle 6 Bugs implementiert.

| Bug | Komponente | Änderung |
|-----|-----------|----------|
| Bug 1 | PluginView.vue + kiwiStore.ts | P1/P2-Buttons mit `@click` + `specPeak1/2`-Store-State; CSS `.kiwi-cpanel__btn--violet-active` |
| Bug 2 | Waterfall.vue | "No signal — connect to a KiwiSDR station" Overlay bei leeren Bins |
| Bug 3 | BandScaleBar.vue | `BandDef` Interface: `freq` → `startFreq/endFreq`; 26 Bänder mit Breite; `freqWidthPercent()`; `visibleBands`-Filter |
| Bug 4 | TagArea.vue + TagPopup.vue | 73 DX-Tags (statt 30); zweireihiges Layout (44px); Kollisions-Detektion; `hasExt`-Marker |
| Bug 5 | FrequencyRuler.vue | 4 sub-kHz-Stufen in `if/else`-Kette (0.1 kHz bei zoom 14); `formatFreq` gibt "Hz" für < 1 kHz |
| Bug 6 | PluginView.vue + kiwiStore.ts + bands.ts + bridge-validators.ts | 8 Sub-Bugs: Pan-Symbole, Band-Select (87 Optionen), Extension-Select (27), Zoom-Buttons, Layout (VFO/Users), Spectrum-Button (3 Modi), Audio-Symbol, RF-Tab (Attn/NB/CW) |

**Neue Datei:** `ui/src/data/bands.ts` — 87 Band-Optionen in 6 `<optgroup>`-Gruppen.

**Geänderte Dateien (13):**
- `ui/src/views/PluginView.vue` — Template + Script + CSS
- `ui/src/store/kiwiStore.ts` — 5 neue State-Felder
- `ui/src/components/FrequencyRuler.vue` — Step-Kette + formatFreq
- `ui/src/components/BandScaleBar.vue` — Interface + Daten + Funktion
- `ui/src/components/TagArea.vue` — 73 Tags + 2-Reihen-Layout
- `ui/src/components/TagPopup.vue` — `hasExt`-Feld im Interface
- `ui/src/components/Waterfall.vue` — No-Signal-Overlay
- `ui/src/generated/bridge-validators.ts` — `rfAttn` + `cwPeaks` ParamIds
- `ui/src/data/bands.ts` — NEU

**Build:** `vue-tsc --build` ✅, `vite build` ✅ (208.68 kB)
**Tests:** 15 Test-Files, 112 Tests ✅

## 2026-08-29 — Agent-Infrastruktur überarbeitet (opencode.json + AGENTS.md)

**Update:** [`AGENTS.md`](./AGENTS.md) — Project Overview, Role & Delegation
Model und Definition of Done ergänzt; Subagent-MCP-Regeln angepasst (netsdr_rag
+ GitHub read-only für Subagents); Todo-first-Workflow für alle Primary Agents;
Workspace-spezifische Agents klar von globalen (`build`/`plan`) getrennt.

**Update:** [`opencode.json`](../opencode.json) — Agenten-Definitionen bereinigt:
System-Prompts auf Identität+Rolle reduziert (gemeinsame Regeln zentral in
AGENTS.md); neue Agents `ARCHITECT`, `BUILD_Openrouter`, `DEV_JUNIOR_Openrouter`;
Rollen-Prompts ergänzt (ARCHITECT=Entscheidungen, BUILD=Senior/projektweit,
DEV=Task-Fokus, DEV_JUNIOR=kleine Tasks); Modell-Wechsel BUILD →
`opencode/nemotron-3-ultra-free`, DEV → `opencode/nemotron-3.5-lightning-free`; Context-Limits
pro Modell gesetzt (deepseek flash 250K / pro 500K, sonnet-4-6 200K, solar-pro4 200K).

**Deprecation:** [`WORKSPACE_AGENT_PROMPT.md`](../WORKSPACE_AGENT_PROMPT.md) — als
deprecated markiert; Inhalt lebt jetzt in AGENTS.md.

## 2026-08-29 — Korrektur Bug 2 Fix-Plan: kein Simulator

**Entscheidung:** "Keine Verbindung → kein Spektrogramm" ist korrekt, kein Bug.
Der zuvor geplante Dev-Mode-Simulator und C++-Fallback-Simulator werden nicht umgesetzt.
Bug 2 M4c.7-Fix-Scope: nur ein "No signal"-Overlay in `Waterfall.vue` wenn `bins.length === 0`.
Echter WF-Datenstrom (WF-WebSocket, `MSG dx_community`) kommt in M5.

**Update:** [`doc/M4c.7-bugs.md`](./M4c.7-bugs.md) — Bug 2 Fix-Plan vollständig neu geschrieben.
**Update:** [`doc/checklist.md`](./checklist.md) — M4c.7.2a als analysiert markiert, 2b Scope korrigiert.

## 2026-08-29 — Architektur-Entscheidung: DX-Tags dynamisch via WF-Socket (M5)

**Entscheidung:** DX-Tags werden in M5 dynamisch vom KiwiSDR-WF-WebSocket geladen,
nicht statisch hinterlegt. KiwiSDR sendet `MSG dx_community=[json]` über einen
separaten WebSocket-Stream (`/WF`-Pfad). M4c.7 liefert weiterhin die statische
73-Einträge-Liste als Platzhalter.

**Update:** [`doc/M4c.7-bugs.md`](./M4c.7-bugs.md) — Bug 4 Analyse + Fix-Plan um Architektur-Notiz erweitert.
**Update:** [`doc/plan.md`](./plan.md) — M5-Abschnitt um WF-Socket-Teilfeature ergänzt (Protokoll, Scope, Abhängigkeiten).
**Update:** [`doc/checklist.md`](./checklist.md) — M4c.7.4a als analysiert markiert, Scope-Hinweis ergänzt.

---

## 2026-08-29 — M4c.7 Bug-Manifest vollständig analysiert

**Task:** Analyse aller 6 Bugs in M4c.7-bugs.md + Restrukturierung als Implementation Plan.

**Ergebnisse:**
- Bug 1 (P1-Button): P1 = "Spectrum Peak Hold 1" — kein Readme-Toggle. Button funktionslos (kein @click). Fix: Store-State specPeak1, Handler, Peak-Overlay in Waterfall.
- Bug 2 (Wasserfall): Datenfluss-Kette vollständig vorhanden aber inaktiv ohne KiwiSDR-Verbindung. Fix: Dev-Mode-Simulator in pluginService.ts + C++ Debug-Fallback.
- Bug 3 (BandScaleBar): Kein width in CSS → rendert Punkte statt Blöcke. BandDef braucht startFreq/ndFreq. Vollständige Band-Liste (26 Blöcke) erarbeitet. Band-Select-Dropdown (87 Optionen) als ands.ts.
- Bug 4 (DX-Tags): 30 DEMO_TAGS → 73 echte Tags. Zweireihiges Layout: height: 44px + Kollisions-Detektion.
- Bug 5 (FrequencyRuler): stepKhz Minimum bei 10 kHz → zoom 10–14 hat fast keine Ticks. Fix: 4 Stufen ergänzen bis 0.1 kHz (100 Hz).
- Bug 6 (PluginView): 8 Sub-Bugs analysiert mit konkreten Zeilen. RF-Tab fehlt komplett. Band/Ext-Select leer. P1/P2 ohne Handler. Spectrum ist Span statt Button.

**Dateien geändert:** doc/M4c.7-bugs.md (vollständig restrukturiert — IST + Analyse + Fix-Plan pro Bug)

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