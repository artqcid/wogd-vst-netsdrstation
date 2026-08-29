---
type: Design Document
title: LLM-Wiki Refactoring — NetSDRStation-VST
description: Design document for the LLM-Wiki restructuring following Karpathy's LLM-Wiki and Google's OKF v0.2 principles
status: deprecated
generated:
  by: human:marku
  at: 2026-08-29
verified:
  by: human:marku
  at: 2026-08-29
tags: [wiki, llm-wiki, karpathy, okf, refactoring, meta]
sources:
  - title: Karpathy LLM-Wiki
    url: https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f
  - title: Google Open Knowledge Format v0.2
    url: https://github.com/GoogleCloudPlatform/open-knowledge-format
---

# LLM-Wiki Refactoring — NetSDRStation-VST

_Geplanter Umbau von `doc/` + RAG/MCP + NotebookLM nach den Prinzipien des
LLM-Wiki (Karpathy) und des Open Knowledge Format (OKF v0.2) von Google._

_Status: IMPLEMENTED — alle Phasen abgeschlossen (2026-08-29). `AGENTS.md` enthält
Lint-Workflow und deterministisches Knowledge-Sync (inkl. NotebookLM-Rolle)._

---

## 0. Quellen

| Quelle | URL | Kernidee |
|--------|-----|----------|
| Karpathy "LLM-Wiki" | https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f | Statt RAG-zur-Query-Zeit baut die LLM-Instanz einen **persistenten, verlinkten Markdown-Wiki** als kompiliertes Artefakt. `index.md` (Katalog) + `log.md` (Chronik) + Lint. |
| Google "Open Knowledge Format" (OKF v0.2) | https://github.com/GoogleCloudPlatform/open-knowledge-format | Jedes Wissens-Konzept = **1 Markdown-Datei mit YAML-frontmatter**. Provenance, Trust, Freshness, Lifecycle sind first-class (`sources`, `generated`, `verified`, `status`, `stale_after`). Gleiche Reserved-Namen `index.md` + `log.md`. |

---

## 1. Kurzfassung der angewendeten Prinzipien

### Aus Karpathy (LLM-Wiki)
- **Kern:** Wissen wird einmal kompiliert und dann *gepflegt* — nicht bei jeder
  Frage neu abgeleitet ("compounding artifact", kein "Redisco").
- **3 Schichten:** Rohquellen (unveränderlich) / Wiki (LLM-geschrieben, kompiliert)
  / Schema (Regeln in AGENTS.md, die den LLM zum disziplinierten Wiki-Manager machen).
- **2 Spezialnavigationen:**
  - `index.md` — katalogorientiert: pro Seite Link + 1-Zeilen-Summary, Kategorien,
    wird bei jedem Ingest aktualisiert.
  - `log.md` — chronologisch, append-only, parsebar mit `grep "^## \["`.
- **Operationen:** Ingest / Query / **Lint** (Widersprüche, stale Claims, Orphan-Seiten,
  fehlende Cross-Refs).
- **Gute Antworten zurück ins Wiki fileden** (Komparungen/Analysen compounden).
- Team-Scale-Lessons (Kommentare): Platzhalter-Reservierung bei concurrent ingest;
  Trust/Provenance/Freshness first-class; Index ist lossy → BM25-Union; Human-Korrekturen
  als **Pins**; Review des generierten Artefakts, nicht des Plans.

### Aus Google OKF v0.2
- Jedes Konzept = Markdown-Datei mit YAML-frontmatter.
- `type` (REQUIRED) + `title`, `description`, `tags`, `resource` (recommended).
- **Provenance/Trust/Lifecycle in frontmatter:**
  - `sources:` — mit Kredibilitäts-Signalen `author`, `usage_count`, `last_modified`.
  - `generated: { by, at }` — wer/wann erzeugt.
  - `verified:` — wer/wann bestätigt → Trust-Tier (unverified / machine / human).
  - `status:` — `draft | stable | deprecated`.
  - `stale_after:` — absoluter Zeitpunkt, ab dem das Konzept als stale gilt.
- Reserved-Namen: `index.md` + `log.md` (identisch mit Karpathy).
- **Cross-Linking:** absolute bundle-relative Links `/pfad/zu/konzept.md` (stabil beim
  Verschieben); `references/`-Konvention.
- **Actor-Convention:** `producer/version`, `human:<id>`, `process:<id>`.
- Attested Computation (optional, für berechenbare Werte) — hier nur tangential relevant.

---

## 2. Ist-Zustand (Befund)

### `doc/` — Stärken
- Gut aufgeteilt: `architecture.md`, `plan.md`, `checklist.md`, `ui-architecture.md`,
  `reference-matrix.md`, Implementation-Plans (M3–M6), `coding-standards.md`,
  `test-strategy.md`, `logging-strategy.md`, `framework-licensing.md`, `station-list.md`,
  Bug-/Audit-Dateien.
- Enge Cross-Verweise zwischen den Dateien (relativ, z. B. "siehe `doc/architecture.md` §6").
- Deutsche + englische Inhalte gemischt (einige Dateien sind deutsch).

### `doc/` — Schwächen (aus LLM-Wiki-Sicht)
1. **Kein `index.md`** → der Agent/LLM muss bei jeder Query raten, welche der 19+
   Dateien relevant ist; keine 1-Zeilen-Summaries, kein Katalog.
2. **Kein `log.md`** → keine chronologische Übersicht; Changelog-artige "Status"-Blöcke
   sind in `plan.md`/`checklist.md` **vermengt** statt append-only.
3. **Kein einheitliches Schema:** kein standardisierter Kopf/frontmatter. `$type`,
   Quellen, Vertrauen, Frische (staleness), Erzeuger/Datum sind **nicht maschinenlesbar**
   (z. B. Datum nur im Fließtext, Quellen nur als Fußnoten).
4. **Wissen wird ständig neu abgeleitet statt kompiliert:** `plan.md` + `checklist.md` +
   `architecture.md` wiederholen denselben Status mehrfach ("Status 2026-08-22/26/27").
5. **Kein expliziter Lint-Mechanismus:** Widersprüche werden nicht zentral aufgedeckt
   (Beispiel: `reference-matrix.md` ✅ TagPopup vs. `ui-architecture.md` §7.3 Bug 4 ❌).
6. **Kein `status`/`stale_after`** → Stale Claims sind nicht von aktuellen unterscheidbar.

### RAG/MCP (`netsdr_mcp_server.py`)
- `index_project_code` indexiert C++ + Python + Markdown (inkl. AGENTS.md,
  `doc/*.md` strukturiert nach Headings). Gut als Such-Schicht.
- `code_wiki.md` wird generiert (578 KB / 4111 Zeilen) — ein Roh-Symbol-Verzeichnis,
  **kein** kategorisierter, beschreibender Wiki-Katalog. Keine provenance/trust/freshness
  pro Konzept, keine `sources`.
- Der Indexer liest Headings, **aber kein YAML-frontmatter** → bei Einführung von
  OKF/frontmatter muss er erweitert werden (Konzept-Summary + Trust in den Index).

### NotebookLM (`notebooklm_devblogs`, Notebook "NetSDRStation-VST")
- Dient als dritter Speicher ("push relevant knowledge to notebook"). Stellt einen
  parallelen Notiz/Report-Raum dar. Sync zwischen `doc/` ↔ RAG ↔ NotebookLM ist manuell
  und nicht deterministisch → Drift-Risiko.

---

## 3. Geplante Optimierung (nur Plan — nicht implementiert)

### Grundsatzentscheidung
Wir **führen OKF-frontmatter als Schema für alle `doc/*.md`-Konzepte ein** und setzen
die **Karpathy-`index.md` + `log.md` + Lint**-Mechanik als deterministische Workflows um.
Das RAG bleibt Such-Engine; die `doc/`-Konzepte werden "kompiliert" (self-describing +
katalogisiert), statt bei jeder Frage neu abgeleitet zu werden.

### Phase 1 — Schema einführen (OKF-light)
1. **`index.md` im `doc/`-Root** erzeugen und bei jeder Änderung pflegen:
   - Pro Konzeptdatei: 1 Zeile mit relativem Link + `description` (aus frontmatter) +
     Kategorie (Architektur / Plan / Checklist / UI / Test / Protokoll / ...).
   - Deterministische "erste Navigation" für Agent UND Mensch (Karpathy-`index.md`).
2. **`log.md` im `doc/`-Root** einführen (append-only, newest first, `## YYYY-MM-DD`):
   - Einträge mit `**Creation** / **Update** / **Deprecation**`-Präfix + verlinktes Konzept.
   - Parsebar mit `grep "^## \["` / `grep "^## "`.
3. **YAML-frontmatter** zu jeder `doc/*.md`-Konzeptdatei (OKF-Familien, maßvoll):
   - `type:` — z. B. `Architecture`, `Plan`, `Checklist`, `UI Inventory`,
     `Protocol Reference`, `Test Strategy`, `Licensing Analysis`,
     `Implementation Plan (M3…M6)`, `Station List`, `Bug List (M4b)`.
   - `title`, `description` (1-Satz), `tags`, `status:` (draft/stable/deprecated),
     `generated: {by, at}`.
   - `sources:` (extern, z. B. KiwiSDR-Repo, radiom, KiwiAngel) + `verified:`
     (`human:marku`) statt Datum im Fließtext.
   - **`stale_after:`** für zeit-sensitive Dateien (`station-list.md`,
     `reference-matrix.md`, `M4b-bugs.md`) → Lint kann stale Konzepte warnen.
4. **Implementation-Plans (M3–M6)** → `type: Implementation Plan` mit `status`
   (done/in-progress) → "was ist noch offen" maschinenlesbar, statt nur Fließtext.

### Phase 2 — `checklist.md` + `plan.md` entflechten (Gegenmaßnahme "Redisco worry")
5. **`checklist.md` → reine Aufgabenliste** (offene/erledigte Tasks, kurz),
   KEINE Status-Statements.
6. **Historische Status-Blöcke aus `plan.md`** heraus in `log.md` (chronologisch) oder
   `archive/` — `plan.md` beschreibt danach nur noch den **aktuellen** Plan
   (Milestones + offene Punkte), nicht die Historie.
7. Ergebnis: gleiche Fakten existieren an **genau einer Stelle** (Single Source of Truth),
   andere Dateien **verlinken** darauf (OKF-`resource`/Cross-Links) statt zu duplizieren.

### Phase 3 — RAG/MCP-Erweiterung
8. **`netsdr_mcp_server.py` erweitern:** beim Indexieren von `.md` das **YAML-frontmatter
   auswerten** — `type`, `description`, `status`, `generated`, `stale_after` übernehmen,
   sodass der Wiki-Index pro Konzept **Summary + Trust/Freshness** trägt (statt nur Heading-Split).
9. `code_wiki.md` bleibt der Roh-Symbol-Index (C++/Python), wird aber ergänzt um den
   **Karpathy-`index.md`-artigen Konzeptkatalog** der `doc/`-Konzepte (kann der erzeugte
   `doc/index.md` auch selbst sein — dann ist `index.md` die deterministische Quelle).
10. Optional: **BM25-Suche über `doc/`-Konzepte** als Union zum symbolbasierten Wiki
    (Karpathy-Kommentar: "index is lossy, fix with union"). Das RAG macht heute schon FTS5 —
    sicherstellen, dass `doc/*.md`-Inhalte die FTS-Datenquelle bleiben.

### Phase 4 — Lint & Widerspruchs-Erkennung
11. **Lint-Workflow etablieren** (wiederverwendbarer Agent-Schritt / Checklist-Item):
    - Orphan-Seiten (keine Inbound-Links im `index.md`/Graph) finden.
    - **Stale Claims** (`now >= stale_after`) melden.
    - **Duplizierte Claims** über Dateien finden (z. B. "TagPopup ✅" vs "❌") und als
      konkrete Konflikt-Liste ausgeben.
    - fehlende Cross-Referenzen melden.
    - Bekannter Fund, den Lint aufdecken würde: `reference-matrix.md` (✅ TagPopup) vs.
      `ui-architecture.md` §7.3 Bug 4 (❌).
12. **Gute Query-Antworten gleich zurück ins Wiki fileden** (Karpathy): einmal getroffene
    Analysen/Entscheidungen (z. B. "Station-Directory-Endpoint ist 404 → neue Analyse")
    als eigenes Konzept/Update persistieren statt in Chat-History zu verschwinden.
    (Die M6-Analysen zu radiom/VibeSDR/KiwiAngel sind bereits ein gutes Beispiel — beibehalten.)

### Phase 5 — NotebookLM-Sync (Rollen klären)
13. **Rollen klar trennen:**
    - `doc/` = OKF-Wiki (kompilierte, verlinkte Markdown-Konzepte) — **primärer Speicher**.
    - RAG = Such-/Symbol-Schicht über `doc/` + Quellcode.
    - **NotebookLM** = curated "externer Report/Notiz-Raum" für Langzeit-Dokumente/Briefings;
      NICHT als Duplikat des Wiki, sondern als **konsumierende/verbreitende** Schicht
      (oder `references/`-ähnliche Zusatzquelle).
14. **Sync deterministischer machen:** statt manueller "push relevant knowledge" — ein klar
    benannter Post-Task-Step (existiert bereits in AGENTS.md); optional: die `log.md`-Einträge
    der Woche als **eine** aggregierte Notiz an NotebookLM pushen (weniger Drift).

---

## 4. Priorisierte Empfehlung (Reihenfolge der Umsetzung)

| # | Maßnahme | Aufwand | Nutzen (LLM-Wiki) | Phase |
|---|----------|---------|-------------------|-------|
| 1 | `index.md` + `log.md` im `doc/` einführen | gering | Navigation + Chronik, Kernidee Karpathy/OKF | 1 |
| 2 | OKF-YAML-frontmatter (`type/description/status/sources/generated/stale_after`) auf alle `doc/*.md` | mittel | Provenance/Trust/Freshness maschinenlesbar | 1 |
| 3 | Status-Historie aus `plan.md`/`checklist.md` nach `log.md`/`archive/` (Single Source of Truth) | mittel | kein Redisco, keine stale-Duplikate | 2 |
| 4 | RAG-Indexer: frontmatter-aware (Konzept-Summary + Trust in Wiki-Index) | mittel | Such-Ergebnisse tragen Kontext/Frische | 3 |
| 5 | Lint-Workflow (Orphans/Stale/Duplikate) | gering | findet die abgeleiteten Widersprüche | 4 ✅ |
| 6 | NotebookLM-Sync-Rolle klären + log-basiertes Aggregat | gering | weniger Drift zwischen den 3 Speichern | 5 ✅ |

**Minimal-Viable (wichtigste 2 Schritte):** `index.md` + `log.md` + frontmatter-Schema
(Phasen 1–2). Das allein löst die größte strukturelle Lücke (keine Navigation, kein Schema,
keine Trust/Freshness) und entspricht beiden Quellen.

---

_Created: 2026-08-29. Plan only — no filesystem changes applied yet._
