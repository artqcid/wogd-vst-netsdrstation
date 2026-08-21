# M1 Open Tasks — Abarbeitungsreihenfolge für Coding Agents

_Alle offenen Punkte aus Milestone M1. Vollständige Beschreibungen und
Akzeptanzkriterien: `doc/checklist.md` (IDs sind identisch)._

**Sortierung nach Effizienz und Kontextlokalität**, nicht nach Priorität:
- Zusammengehörige Dateien werden in einer Sitzung abgearbeitet.
- Abhängigkeiten (A muss vor B fertig sein) sind durch Pfeile markiert: `→`.
- Jede Gruppe kann von einem Agent als zusammenhängende Session erledigt werden.

---

## Gruppe 1 — CMake-Fundament (keine Abhängigkeiten nach oben)

Alle Fixes in `CMakeLists.txt`, `source/entry/CMakeLists.txt` und
`CMakePresets.json`. Gemeinsamer Kontext: ein Agent liest beide Dateien
einmal und erledigt alle Punkte in einer Session.

| ID | Datei | Änderung |
|----|-------|----------|
| [x] **FIX-26** | `CMakeLists.txt:46-49` | `VST3_SDK_ROOT` Hard-coded-Default entfernen |
| [x] **FIX-27** | `CMakeLists.txt:85,157-158` + `source/entry/CMakeLists.txt:89` | `WEBVIEW2_SDK_ROOT` auf eine Definition konsolidieren, `WV2_SRC` löschen |
| [x] **FIX-28** | `CMakePresets.json` (`win-analyze`) | `"generator": "Ninja"` hinzufügen |
| [x] **FIX-29** | `source/entry/CMakeLists.txt:65-70` | Toten `if(DEFINED webview2_sdk_SOURCE_DIR)` Block entfernen |

_Nach Gruppe 1: `cmake --preset win-msvc` + `cmake --preset win-analyze` müssen
sauber konfigurieren._

---

## Gruppe 2 — Tooling & DX (unabhängig, ein-Zeilen-Fixes)

Keine Laufzeit-Auswirkung, kein Abhängigkeitsbaum. Schnell erledigt.

| ID | Datei | Änderung |
|----|-------|----------|
| [x] **FIX-23** | `CMakePresets.json` + `compile_commands.json` (Symlink) | Ninja-Preset `win-clangd` anlegen, Symlink in Root |
| [x] **FIX-24** | `.vscode/tasks.json:57-103` + `.vscode/launch.json` | `editorhost`-Tasks + Launch-Config entfernen; TestHost-Attach-Config anlegen |
| [x] **FIX-25** | `doc/workspace-workflow.md:63-77` | VST3PluginTestHost als primären Windows-Debug-Host dokumentieren |

_FIX-23 und FIX-24 sind unabhängig voneinander, aber beide ändern Tooling-Dateien
→ gemeinsame Session spart einen Re-Index._

---

## Gruppe 3 — C++ Ein-Zeilen-Fixes (wenig Kontext, kein Runtime-Risiko)

Jeder Fix betrifft eine Stelle, benötigt wenig Kontext, hat kein Abhängigkeitsrisiko.

| ID | Datei | Änderung |
|----|-------|----------|
| [x] **FIX-31** | `source/vst/common/pluginids.h:13-16` | `static const FUID` → `inline const FUID` |
| [x] **FIX-30** | `source/webview/webview_editor.cpp:27` | `debug=true` → `#ifndef NDEBUG`-Guard |

---

## Gruppe 4 — WebView2-Deployment-Chain

**Reihenfolge ist zwingend** (jeder Fix baut auf dem vorherigen auf):

```
FIX-WV2-D  →  FIX-WV2-C  →  FIX-WV2-B1  →  FIX-WV2-A  →  FIX-WV2-B2  →  FIX-33
```

| Schritt | ID | Datei | Änderung | Voraussetzung |
|---------|-----|-------|----------|---------------|
| 1 | [x] **FIX-WV2-D** | `source/entry/CMakeLists.txt:86-106` | `SMTG_PLUGIN_PACKAGE_PATH` statt hardcoded Bundle-Pfad | — |
| 2 | [x] **FIX-WV2-C** | `source/entry/CMakeLists.txt:86` | `"Release"` → `$<CONFIG>` im POST_BUILD-Pfad | FIX-WV2-D |
| 3 | [x] **FIX-WV2-B1** | `source/entry/CMakeLists.txt` | `ui/dist/` per POST_BUILD in Bundle kopieren | FIX-WV2-D, FIX-WV2-C |
| 4 | [x] **FIX-WV2-A** | `source/webview/webview_editor.cpp:18-31` | `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` vor Webview-Konstruktion setzen | FIX-WV2-B1 (Bundle-Pfad muss stimmen) |
| 5 | [x] **FIX-WV2-B2** | `source/editor/plugin_editor.cpp:24-33` | Runtime-URL aus Modul-Pfad ermitteln (`GetModuleFileNameW`) | FIX-WV2-B1 (ui/ im Bundle) |
| 6 | [x] **FIX-33** | `source/entry/CMakeLists.txt:42-50` + `plugin_editor.cpp:25-33` | `NS_UI_DIST_URL` toten CMake-Block + `#ifdef`-Branch entfernen | FIX-WV2-B2 fertig |

_Danach: manueller Test M1.20 (Release-GUI ohne System-WebView2)._

---

## Gruppe 5 — C++ Tests: `plugin_processor_tests.cpp` (neue Datei)

Alle vier Tests leben in **einer neuen Datei** (`tests/vst/plugin_processor_tests.cpp`).
Alle grün, `ctest` passed.

| ID | Testfall | Status |
|----|----------|--------|
| [x] **TEST-01** | `setupProcessing` ruft `setSampleRate` auf (FIX-01 Regression) | ✅ |
| [x] **TEST-02** | `setState`/`getState` IBStream-Roundtrip | ✅ |
| [x] **TEST-03** | `applyParamValue` schreibt korrekte Plain-Werte in Atomics | ✅ |
| [x] **TEST-04** | `process()` setzt `silenceFlags` korrekt iff muted (FIX-12 Regression) | ✅ |

_Danach: `tests/CMakeLists.txt` um `plugin_processor_tests.cpp` erweitern,
`ctest` grün._

---

## Gruppe 6 — C++ Tests: `plugin_controller_tests.cpp` ✅

| ID | Testfall | Status |
|----|----------|--------|
| [x] **TEST-05** | `setComponentState` spiegelt Processor-State in Params (FIX-06 Regression) | ✅ |

---

## Gruppe 7 — C++ Tests: `plugin_editor_tests.cpp` ✅

| ID | Testfall | Status |
|----|----------|--------|
| [x] **TEST-06** | `onJavaScriptMessage` dispatcht korrekt normalisiert (Mock-Controller) | ✅ |
| [x] **TEST-08** | `checkSizeConstraint` klemmt auf Mindestgröße | ✅ |
| [x] **TEST-07** | `uiUrl()` gibt Debug- vs. Release-URL zurück | ✅ |

---

## Gruppe 8 — Python-Fix (separater Kontext)

Isolierter Fix an einer Funktion in `netsdr_mcp_server.py`.

| ID | Datei | Änderung |
|----|-------|----------|
| [x] **FIX-32** | `netsdr_mcp_server.py:169-186` | `_cosine_similarity` Norm-Bug nach Dict-Swap beheben |
| [x] **FIX-34** | `netsdr_mcp_server.py:1` | Shebang `python` → `python3` |

_Beide Fixes in einer Session, danach MCP-Server neu starten._

---

## Gruppe 9 — Dokumentation & Knowledge-Sync (alles vorher abgeschlossen)

Erst wenn Gruppe 4 (WebView2) und alle Tests grün sind.

| ID | Aufgabe |
|----|---------|
| [x] **M1.21** | `doc/architecture.md` + `doc/workspace-workflow.md`: WebView2-Bundle-Deployment dokumentieren | ✅ |
| [x] **M1.22** | Knowledge-Sync: `index_project_code` → RAG/Wiki; NotebookLM **NetSDRStation-VST** updaten | ✅ (NBML auth expired → manuell) |

---

## Deferred (kein M1-Blocking)

| ID | Grund |
|----|-------|
| **FIX-35** | `ParameterRegistry` O(n) Scan → erst relevant wenn mehr Parameter (M2) |
| **M1.20** | Manueller Test (Release-GUI ohne System-WebView2) — nach Gruppe 4 |

---

## Nachtrag — BUG-01 (Release-GUI `ERR_FILE_NOT_FOUND`) ✅

| ID | Datei | Änderung |
|----|-------|----------|
| [x] **BUG-01** | `source/editor/plugin_editor.cpp`, `ui/vite.config.ts` | Root Cause 1: `GetModuleFileNameW(nullptr,…)` lieferte Host-EXE-Pfad statt Plugin-DLL-Pfad → Fix: DLL-Pfad via `extern "C" IMAGE_DOS_HEADER __ImageBase` + UTF-8/Percent-Encoding. Root Cause 2 (weiße GUI): externe ES-Module unter `file://` durch CORS blockiert → Fix: `vite-plugin-singlefile` inlined JS+CSS in ein einziges `ui/index.html`. Fehlender CMake-`--allow-file-access-from-files`-Patch entfernt (nicht nötig). TEST-07 NDEBUG-aware. Validator 47/47, Unit-Tests 36/36. |
| [x] **BUG-02** | `source/editor/plugin_editor.h/.cpp` | GUI-Eingaben erreichten den Processor nie: Editor rief nur `setParamNormalized` (Controller-Zustand). Fix: direkte Aufrufe der plain virtuals `EditControllerEx1::beginEdit/performEdit/endEdit` (forwarden intern zum Host-`componentHandler`) → Host queued in `inputParameterChanges`. Achtung: `EditControllerEx1` erbt NICHT `IComponentHandler` — queryInterface war ein No-op. Editor jetzt auf `EditControllerEx1*` typisiert. Regressionstest TEST-09. Validator 47/47, Unit-Tests 37/37. |

---

## Abhängigkeitsgraph (kompakt)

```
Gruppe 1 (CMake)
    └─→ Gruppe 2 (Tooling/DX)    ← unabhängig, kann parallel
    └─→ Gruppe 3 (C++ Minors)    ← unabhängig, kann parallel
    └─→ Gruppe 4 (WebView2-Chain)
            FIX-WV2-D → FIX-WV2-C → FIX-WV2-B1 → FIX-WV2-A
                                                  └─→ FIX-WV2-B2 → FIX-33
                                                  └─→ M1.20 (manual test)
Gruppe 5/6/7 (Tests)             ← unabhängig von Gruppe 4, kann parallel
    TEST-07 wartet auf FIX-WV2-B2
Gruppe 8 (Python)                ← vollständig unabhängig
Gruppe 9 (Docs & Sync)           ← wartet auf alle obigen
```
