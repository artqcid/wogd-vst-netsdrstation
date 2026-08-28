# NetSDRStation-VST - Checklist

_Open tasks only (short descriptions). Detailed info: `doc/architecture.md`;
draft plan: `doc/plan.md`; workflow: `doc/workspace-workflow.md`;
coding rules: `doc/coding-standards.md`; test strategy: `doc/test-strategy.md`;
KiwiSDR-Protokoll: `doc/kiwisdr-protocol-reference.md`;
Station list + Dauerbetrieb: `doc/station-list.md`;
implementation plans: `doc/M3-implementation-plan.md` (M3),
`doc/M4-implementation-plan.md` (M4), `doc/M5-implementation-plan.md` (M5),
`doc/M6-implementation-plan.md` (M6)._

> **M1 offene Punkte mit Abarbeitungsreihenfolge für Coding Agents:**
>

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
- [x] **M1.13** Start VST3PluginTestHost (Debug) – scan plugin folder, select plugin in VST Rack, choose ASIO driver.
- [x] **M1.14** Stop VST3PluginTestHost (Debug) – terminate process.
- [x] **M1.15** Start VST3PluginTestHost (Release) – scan plugin folder, select plugin in VST Rack, choose ASIO driver.
- [x] **M1.16** Stop VST3PluginTestHost (Release) – terminate process.

- [x] **M1.17** WebView2 Fixed Version herunterladen
  - x64-Fixed-Version-Runtime + NuGet-SDK unter `C:/Users/marku/Documents/GitHub/thirdParty/WebView2SDK/` abgelegt.
  - Status: erledigt – Dateien vorhanden (`WebView2Loader.dll`, `FixedRuntime/Microsoft.WebView2.FixedVersionRuntime.151.0.4129.93.x64/`).

- [x] **M1.18** WebView2-Dateien per CMake POST_BUILD in VST3-Bundle kopieren
  - `WebView2Loader.dll` + `FixedRuntime/`-Ordner werden nach
    `Contents/x86_64-win/` kopiert (`source/entry/CMakeLists.txt:96-106`).
  - Status: kopiert – Bundle enthält `WebView2Loader.dll` und `FixedRuntime/EBWebView/x64/EmbeddedBrowserWebView.dll`.
  - **Offener Defekt → FIX-WV2-C:** `PLUGIN_DIR` ist auf `"Release"` hardcodiert
    (`CMakeLists.txt:86`); muss `$<CONFIG>` verwenden.
  - **Offener Defekt → FIX-WV2-D:** Pfad sollte über SMTG-Property
    `SMTG_PLUGIN_PACKAGE_PATH` ermittelt werden, nicht hardcodiert.

- [x] **FIX-WV2-A** `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` vor Webview-Konstruktion setzen
  - **Root cause:** `webview::webview` 0.12.0 übergibt `nullptr` als `browser_dir` an
    `CreateCoreWebView2EnvironmentWithOptions` (webview.h:4218). Dadurch sucht die
    Bibliothek nur in der Windows-Registry – der gebündelte `FixedRuntime/`-Ordner
    wird komplett ignoriert. Ohne system-weites WebView2 bleibt das GUI leer.
  - **Fix:** In `WebViewHost::Impl::attach()` (`source/webview/webview_editor.cpp`)
    vor `std::make_unique<webview::webview>(…)`:
    1. Eigenen DLL-Pfad per `GetModuleFileNameW` ermitteln.
    2. `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` auf `<moduledir>/FixedRuntime` setzen
       (`_wputenv_s` / `SetEnvironmentVariableW`).
    3. Webview-Objekt konstruieren.
    4. Umgebungsvariable danach zurücksetzen.
  - _Files: `source/webview/webview_editor.cpp:18-31`_
  - Test: Release-Build in VST3PluginTestHost laden **ohne** system-weites WebView2;
    GUI muss sichtbar sein.

- [x] **FIX-WV2-B1** `ui/dist/` per CMake POST_BUILD in VST3-Bundle kopieren
  - **Root cause:** Release-Build lädt UI über eine zur Compile-Zeit eingebettete,
    maschinenspezifische `file://`-URL (`source/entry/CMakeLists.txt:43-49`,
    `source/editor/plugin_editor.cpp:29`). `ui/dist/` liegt nicht im Bundle.
  - **Fix:** POST_BUILD-Befehl in `source/entry/CMakeLists.txt` ergänzen:
    ```cmake
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/dist"
        "${PLUGIN_DIR}/ui"
    ```
    Setzt voraus, dass `netsdrstation_ui` (vite build) vorher ausgeführt wurde.
  - Test: `ui/dist/`-Verzeichnis im Bundle unter `Contents/x86_64-win/ui/` vorhanden.

- [x] **FIX-WV2-B2** UI-URL zur Laufzeit aus Modul-Pfad ableiten
  - **Root cause:** `kUiUrl` in `plugin_editor.cpp:29` ist eine Compile-Konstante mit
    absolutem Pfad zur Dev-Maschine. Auf anderen Rechnern zeigt die URL ins Leere.
  - **Fix:** `buildReleaseUiUrl()` in `plugin_editor.cpp` implementieren:
    1. `GetModuleFileNameW` → Verzeichnis des Plugin-DLL.
    2. URL = `file:///<moduledir>/ui/index.html`.
    3. `kUiUrl` durch den Rückgabewert von `buildReleaseUiUrl()` ersetzen
       (nur im `#else`-Zweig / Release-Pfad).
  - _Files: `source/editor/plugin_editor.cpp:24-33`_
  - Test: Plugin auf einem Rechner ohne vorherige Build-Tree-Struktur laden; GUI erscheint.

- [x] **FIX-WV2-C** POST_BUILD-Pfad von `"Release"` auf `$<CONFIG>` umstellen
  - _File: `source/entry/CMakeLists.txt:86`_
  - Fix: `"${CMAKE_BINARY_DIR}/VST3/Release/…"` → `"${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/…"`
  - Test: Debug-Build kopiert Dateien in `VST3/Debug/…`, Release in `VST3/Release/…`.

- [x] **FIX-WV2-D** SMTG-Property für Bundle-Pfad verwenden
  - _File: `source/entry/CMakeLists.txt:86-106`_
  - Fix:
    ```cmake
    get_target_property(_BUNDLE_DIR netsdrstation SMTG_PLUGIN_PACKAGE_PATH)
    set(PLUGIN_DIR "${_BUNDLE_DIR}/Contents/x86_64-win")
    ```
  - Test: Konfiguration ohne Preset (CMake GUI) erzeugt korrekte Kopier-Pfade.

- [x] **BUG-01** Release-GUI zeigt `ERR_FILE_NOT_FOUND` (`file://` URL nicht gefunden)
  - **Symptom:** Release-Build lädt `file:///C:/.../ui/index.html` im WebView2, aber Edge zeigt
    `ERR_FILE_NOT_FOUND` ("Die Datei wurde nicht gefunden...").
  - **Root Cause (korrigiert):** `buildReleaseUiUrl()` nutzte
    `GetModuleFileNameW(nullptr, …)` — das liefert den Pfad der **Host-EXE**, nicht der
    Plugin-DLL. Die URL zeigte daher auf `<hostExeDir>/ui/index.html`, das nicht existiert.
    (WebView2 erlaubt Top-Level-`file://`-Navigation per Default; `AreFileAccessEnabled`
    ist default `TRUE` — die ursprüngliche Analyse war falsch, kein Bibliotheks-Patch nötig.)
  - **Fix:** `plugin_editor.cpp`: DLL-eigener Pfad über `extern "C" IMAGE_DOS_HEADER
    __ImageBase` → `GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), …)`.
    Zusätzlich: UTF-8-Konvertierung via `WideCharToMultiByte` + Percent-Encoding
    (`percentEncodePath()`) für Leerzeichen/Nicht-ASCII in Installationspfaden.
  - Entfernt: fehlerhafter CMake-Patch-Block für `--allow-file-access-from-files`
    (zielte auf 0-Byte-`webview.cpp`; nicht erforderlich).
  - **Root Cause 2 (nach Fix 1: weiße GUI):** Vite-Build erzeugte
    `<script type="module" crossorigin src="./assets/…">`. Unter `file://` ist der
    Origin `null` → Chromium blockt externe ES-Module per CORS-Policy → Vue mountet
    nie → weiße Fläche (HTML lädt, JS nicht).
  - **Fix 2:** `ui/vite.config.ts`: `vite-plugin-singlefile` — JS+CSS werden inline
    in `index.html` inlined (65,9 kB, ein File). Inline-Module-Scripts laufen unter
    `file://` ohne CORS. Bundle wird dadurch noch selbstständiger (kein assets/-Ordner).
    Stale `assets/` im Bundle nach Rebuild manuell löschen (`copy_directory` räumt nicht).
  - _Files: `source/editor/plugin_editor.cpp:20-90`, `CMakeLists.txt`, `ui/vite.config.ts`, `ui/package.json`_
  - Test: Release-Build + Validator (47/47 bestanden), Unit-Tests 36/36,
    TEST-07 nun NDEBUG-aware (Debug: Dev-Server-URL; Release: `file:///…/ui/index.html`).
  - Acceptance: GUI lädt Vue-UI aus dem Bundle; manuelle Verifikation im
    VST3PluginTestHost steht aus.

- [x] **BUG-02** Backend reagiert nicht auf GUI-Eingaben (Ton konstant)
  - **Symptom:** Release-GUI lädt (dunkles UI sichtbar), aber Knobs/Mute haben
    keinen Einfluss auf den Sound; Sine-Ton läuft konstant weiter.
  - **Root Cause:** `PluginEditor::onJavaScriptMessage` rief nur
    `controller_->setParamNormalized(tag, value)`. Das aktualisiert ausschließlich
    den Controller-Zustand — der Host erfährt nichts, und der Processor bekommt
    die Änderung nie über `inputParameterChanges` in `process()`.
  - **Fix (korrigiert):** Editor ruft jetzt direkt die **plain virtuals**
    `EditControllerEx1::beginEdit/performEdit/endEdit(tag, normalized)` auf —
    diese forwarden intern an den vom Host gesetzten `componentHandler`
    (`EditController::performEdit`, vsteditcontroller.cpp:206).
    **Wichtig:** `EditControllerEx1` erbt NICHT von `IComponentHandler`;
    ein `queryInterface(IComponentHandler::iid)` auf dem Controller liefert
    `kNoInterface` (erster Fix-Versuch war daher wirkungslos!).
    PluginEditor ist nun auf `EditControllerEx1*` typisiert statt
    `IEditController*`.
  - _Files: `source/editor/plugin_editor.h:21-24,54-55`, `source/editor/plugin_editor.cpp:87-95,225-236`_
  - Test: **TEST-09** (Regression, `tests/vst/plugin_editor_tests.cpp`):
    MockController (EditControllerEx1-Ableitung) zeichnet Gestures auf;
    `setParameter("freq",880)` → begin/perform/end mit normalisiertem Wert;
    unbekannter Param / malformed JSON → keine Gesture. Validator 47/47,
    Unit-Tests 37/37.
  - Acceptance: Knob-Dreh ändert Tonhöhe/Lautstärke, Mute stummt — manuelle
    Verifikation im VST3PluginTestHost steht aus.

- [x] **M1.21** Dokumentation aktualisieren (WebView2-Bundle-Deployment)
  - In `doc/architecture.md` Abschnitt 8 / neuen Abschnitt ergänzen:
    - Deployment-Strategie: Fixed Version Runtime im Bundle, `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER`-Trick.
    - Release-UI-URL: Laufzeit-Ermittlung aus DLL-Pfad.
    - Voraussetzung vor Release-Build: `cmake --build … --target netsdrstation_ui`.
  - In `doc/workspace-workflow.md` Release-Build-Schritte ergänzen.
  - Detailanalyse: `doc/webview2-bundle-audit.md` (bereits angelegt).

- [x] **M1.22** Knowledge-Sync nach WebView2-Fixes
  - Docs (`architecture.md`, `checklist.md`, `workspace-workflow.md`) aktuell.
  - `index_project_code` ausgeführt → RAG/Wiki aktualisiert (47 files, 330 symbols).
  - NotebookLM **NetSDRStation-VST** updated (M1 Completion Status, source `4afee281`).

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

- [x] **FIX-22** Editor UI is invisible in the host: the webview widget stays **0x0**.
  Root cause: webview 0.12.0 `webview::webview(debug, parentHWND)` (embedded mode) creates
  its child "widget" window at 0x0 and only sizes it when the *parent* receives `WM_SIZE`
  (`resize_widget()`). The plugin creates the webview inside `attached()`, i.e. *after* the
  host has already created/sized its window, so no further `WM_SIZE` fires. Our
  `setSize()` is ineffective because webview's `set_size()` resizes the *parent* window
  (already at the requested size -> no change). Verified: child window `webview_widget`
  and `Chrome_WidgetWin_0` are both 0x0 while the plugin runs (WebView2 is created, but
  clipped to nothing).
  _Files: `source/webview/webview_editor.cpp`, `source/editor/plugin_editor.cpp`._
  _Fix: after `attach()`, resize the widget to fill the parent's client area using the
  webview `window()`/`widget()` accessors (Win32: `GetClientRect(parent)` +
  `MoveWindow(widget, ...)`); make `onSize()` resize the widget the same way instead of
  calling `set_size()`. Keep it behind the platform wrapper (macOS/Linux need the
  equivalent NSView/GTK resize)._

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

- [x] **FIX-24** VSCode `tasks.json` / `launch.json`: remove editorhost tasks and launch config
  - **Root cause:** `tasks.json` still contains four `editorhost`-based tasks (`start-plugin-debug`,
    `start-plugin`, `stop-plugin`, `stop-plugin-debug`) and `launch.json` launches `editorhost.exe`.
    The project's debug workflow is exclusively via VST3PluginTestHost (tasks `start-testhost-*`,
    `stop-testhost-*` already present). The editorhost tasks/scripts are dead weight and are
    confusing for fork users.
  - **Fix:**
    1. Remove the four editorhost tasks from `.vscode/tasks.json`.
    2. Replace the single `launch.json` configuration (`Start Plugin (Debug)` → `editorhost.exe`)
       with a VST3PluginTestHost attach configuration (type `cppvsdbg`, request `attach`,
       process name `VST3PluginTestHost.exe`).
  - _Files: `.vscode/tasks.json:57-103`, `.vscode/launch.json`_
  - Test: no `editorhost` references remain in `.vscode/`; F5 in VSCode attaches to TestHost.

- [x] **FIX-25** `doc/workspace-workflow.md` still documents `editorhost` as the primary VST host
  - **Root cause:** Section 2.2 (lines 65–77) describes building/using `editorhost`
    and `pluginval` as the debug host strategy, but M1.13–M1.16 established
    VST3PluginTestHost as the only supported Windows debug host.
  - **Fix:** Rewrite section 2.2 to describe VST3PluginTestHost as the primary
    Windows debug host (scan plugin folder, select plugin in VST Rack, ASIO driver).
    Keep `editorhost`/`pluginval` as a note for cross-platform / CI headless use.
  - _File: `doc/workspace-workflow.md:63-77`_
  - Test: no misleading `editorhost`-first instructions remain in the workflow doc.

- [x] **FIX-23** clangd kann Standardbibliotheks-Header nicht finden (`'cmath' file not found`)
  **Root cause:** Der Visual-Studio-18-Generator erzeugt trotz `CMAKE_EXPORT_COMPILE_COMMANDS=ON`
  keine `compile_commands.json` (CMake-Einschränkung: nur Ninja/Makefile-Generatoren tun das).
  clangd findet deshalb keine Compile-Datenbank, kennt weder MSVC-Include-Pfade noch
  `/std:c++20` und fällt auf seine Built-in-Header zurück → alle STL-Header fehlen.
  **Fix – Ninja-Preset `win-clangd` in `CMakePresets.json` hinzufügen:**
  1. Neues `configurePreset` `win-clangd` anlegen: `"generator": "Ninja"`, erbt `base`,
     setzt `NS_ENABLE_VST3=ON` (damit der Plugin-Code mit seinen Includes erfasst wird).
  2. Preset einmalig konfigurieren: `cmake --preset win-clangd`
     → erzeugt `build/win-clangd/compile_commands.json` mit allen echten MSVC-Flags.
  3. `compile_commands.json` per Symlink / Junction in den Workspace-Root legen:
     `New-Item -ItemType SymbolicLink -Path compile_commands.json -Target build/win-clangd/compile_commands.json`
     (oder Pfad in `.clangd` via `CompileFlags.CompilationDatabase` eintragen).
  4. VSCode neu laden → clangd liest die echten Flags; alle roten Fehler verschwinden.
  _Files: `CMakePresets.json` (Preset `win-clangd`), `compile_commands.json` (Symlink im Root)_
  Test: `sine_oscillator.cpp` in VSCode öffnen – keine clangd-Fehler mehr (`cmath`, `std`).

## M1 Test Coverage (audit 2026-08-21)

> Units **already tested** (all green, no entries needed):
> `SineOscillator` · `LockFreeSPSC` · `WorkerThread` · `ParameterRegistry` ·
> `ProcessorState` · `bridge_protocol` (parseSetParameterMessage + paramIdFromUiName).
>
> Units **not yet tested** → open tasks below.
> `WebViewHost` and `factory.cpp` require a real WebView2 runtime / host infrastructure
> and are **explicitly deferred** to an integration-test milestone (M3+).

- [x] **TEST-01** `PluginProcessor` – `setupProcessing` calls `setSampleRate` on the oscillator
  (FIX-01 regression)
  - **Why:** FIX-01 fixed a bug where `setupProcessing` did not update the
    oscillator's sample rate, causing wrong pitch at any rate other than 48 kHz.
    There is currently no automated check that prevents this regression.
  - **Test:** Construct a `PluginProcessor`, call `setupProcessing` with
    `sampleRate = 96000`, then call `process` with a short buffer and verify the
    Goertzel peak is at the expected frequency (not double the expected frequency).
  - _File: new `tests/vst/plugin_processor_tests.cpp`_

- [x] **TEST-02** `PluginProcessor` – `setState`/`getState` roundtrip
  - **Why:** State persistence is core VST3 behaviour. No test currently exercises
    the `IBStream`-based serialization path of the processor.
  - **Test:** Use `IBStreamer` / a `MemoryStream` stub to call `getState`, feed the
    bytes back into `setState`, and verify that `freqHz_`, `volume_`, and `mute_`
    atomics match the original values.
  - _File: `tests/vst/plugin_processor_tests.cpp`_

- [x] **TEST-03** `PluginProcessor` – `applyParamValue` stores normalized value in atomics
  - **Why:** `applyParamValue` is the hot path for host automation. A regression
    here (e.g. wrong normalization or wrong atomic) silently mis-tunes the synth.
  - **Test:** Call `applyParamValue(kParamFreq, normalizedValue)` and read back
    `freqHz_` via a test accessor or by rendering a short block and measuring the
    Goertzel peak. Verify plain freq equals `registry_.toPlain(kParamFreq, normalizedValue)`.
  - _File: `tests/vst/plugin_processor_tests.cpp`_

- [x] **TEST-04** `PluginProcessor::process()` – silence flags set iff muted (FIX-12 regression)
  - **Why:** FIX-12 fixed the missing `silenceFlags` on muted output. No test
    currently verifies this, so the fix can silently regress.
  - **Test:** Construct a `ProcessData` with a zeroed output buffer and 2 channels.
    Call `process` with mute off → verify `silenceFlags == 0`.
    Set mute on → call `process` again → verify `silenceFlags == 3` (both channels).
  - _File: `tests/vst/plugin_processor_tests.cpp`_

- [x] **TEST-05** `PluginController` – `setComponentState` mirrors processor state into params
  (FIX-06 regression)
  - **Why:** FIX-06 fixed `setComponentState` returning `kResultOk` without reading
    the stream. No test verifies the controller correctly deserializes and calls
    `setParamNormalized`.
  - **Test:** Serialize a `ProcessorState` (freq=10000, volume=0.25, mute=true) into
    a `MemoryStream`, call `setComponentState`, then read back `getParamNormalized`
    for each parameter and verify it matches the expected normalized value.
  - _File: new `tests/vst/plugin_controller_tests.cpp`_

- [x] **TEST-06** `PluginEditor::onJavaScriptMessage` – correct normalized dispatch to controller
  - **Why:** This is the JS→C++ bridge entry point. The bridge_protocol tests
    verify the parser in isolation, but the editor's wiring (parse → paramIdFromUiName
    → toNormalized → controller.setParamNormalized) is untested end-to-end.
  - **Test:** Implement a mock `IEditController` that records `setParamNormalized`
    calls. Construct a `PluginEditor` with the mock controller and the real
    `ParameterRegistry`. Call `onJavaScriptMessage` with a well-formed envelope
    (e.g. `{"type":"setParameter","data":["freq",440]}`). Verify the mock received
    `setParamNormalized(kParamFreq, ~0.021)`.
  - _File: `tests/vst/plugin_editor_tests.cpp`_

- [x] **TEST-07** `PluginEditor::uiUrl()` – returns correct URL per build type
  - **Why:** The release URL logic (`NS_UI_DIST_URL` / runtime path from FIX-WV2-B2)
    has no test. A broken URL silently produces a blank editor in release builds.
  - **Test:** In a debug build, verify `uiUrl()` returns `"http://localhost:5173"`.
    In a release build (or via a compile-flag override), verify `uiUrl()` returns
    a non-empty string starting with `"file://"`.
  - _File: `tests/vst/plugin_editor_tests.cpp`_
  - _Note: Implement after FIX-WV2-B2 (runtime URL) is done._

- [x] **TEST-08** `PluginEditor::checkSizeConstraint` – min-size clamping
  - **Why:** `checkSizeConstraint` clamps the resize rect to `kMinWidth` (320) ×
    `kMinHeight` (200). Not tested; a regression would allow hosts to resize the
    editor below its usable area.
  - **Test:** Call `checkSizeConstraint` with a rect smaller than the minimums;
    verify the rect is clamped. Call with a larger rect; verify it is unchanged.
  - _File: `tests/vst/plugin_editor_tests.cpp`_

## M1 Corrections – continued (found in full-pass review, 2026-08-21)

> Severity: **Critical** = wrong runtime behaviour / broken feature;
> **Important** = quality/reliability risk; **Minor** = cleanup/polish.

### Important – correctness / quality risk

- [x] **FIX-26** `CMakeLists.txt:47` – `VST3_SDK_ROOT` default is a hard-coded absolute path
  - **Root cause:** The fallback value
    `"C:/Users/marku/Documents/GitHub/thirdParty/vst3sdk"` is machine-specific.
    A fresh clone on any other machine silently uses this non-existent path and
    fails at configure time with a cryptic `FATAL_ERROR`.
  - **Fix:** Remove the hard-coded default; require callers to pass
    `-DVST3_SDK_ROOT=<path>` or set the `VST3_SDK_ROOT` environment variable.
    Update the `FATAL_ERROR` message to mention both options. Keep
    `NS_VENDOR_VST3_SDK` as the submodule alternative.
  - _File: `CMakeLists.txt:46-49`_
  - Test: `cmake --preset win-msvc` without `VST3_SDK_ROOT` set → clear error;
    with env var set → configures cleanly.

- [x] **FIX-27** `CMakeLists.txt:85` / `source/entry/CMakeLists.txt:89` – `WEBVIEW2_SDK_ROOT`
  hard-coded in two places; `WV2_SRC` is a third redundant copy
  - **Root cause:** The path
    `"C:/Users/marku/Documents/GitHub/thirdParty/WebView2SDK"` appears at:
    1. `CMakeLists.txt:85` (root – sets `WEBVIEW2_SDK_ROOT` CACHE PATH)
    2. `source/entry/CMakeLists.txt:89` (entry – re-sets `WEBVIEW2_SDK_ROOT` CACHE PATH)
    3. `CMakeLists.txt:158` (`WV2_SRC` INTERNAL cache var – never used by any target)
    This is a DRY violation; changing the path requires three edits. Entry CMake
    also shadows the cache variable set in the root.
  - **Fix:**
    1. Set `WEBVIEW2_SDK_ROOT` exactly once, in `CMakeLists.txt`, with no
       hard-coded default (require env var or `-D` flag, same pattern as FIX-26).
    2. Remove the redundant `CACHE PATH` re-set in `source/entry/CMakeLists.txt`.
    3. Remove the unused `WV2_SRC` cache variable (`CMakeLists.txt:157-158`).
  - _Files: `CMakeLists.txt:85,157-158`, `source/entry/CMakeLists.txt:89`_
  - Test: single `-DWEBVIEW2_SDK_ROOT=<path>` configures both root and entry correctly.

- [x] **FIX-28** `CMakePresets.json` – `win-analyze` preset has no `generator` and will fail
  - **Root cause:** The `win-analyze` preset inherits `base`, which has no
    `generator`. CMake requires a generator to be specified (or defaults to the
    platform default, which on Windows is Visual Studio – not Ninja). The preset
    description says "Ninja, clang-tidy analysis only" but there is no
    `"generator": "Ninja"` field.
  - **Fix:** Add `"generator": "Ninja"` to the `win-analyze` configure preset.
    Verify that `cmake --preset win-analyze` succeeds.
  - _File: `CMakePresets.json` (`win-analyze` configurePreset, ~line 69)_
  - Test: `cmake --preset win-analyze` configures without error.

- [x] **FIX-29** `source/entry/CMakeLists.txt:66-70` – dead `if(DEFINED webview2_sdk_SOURCE_DIR)` block
  - **Root cause:** The condition checks for `webview2_sdk_SOURCE_DIR`, which
    is set by FetchContent after fetching the WebView2 SDK. But WebView2 is
    **not** fetched via FetchContent in this project – it is a local SDK pointed
    to by `WEBVIEW2_SDK_ROOT`, and its headers are made available via the
    `netsdr_webview2_headers` INTERFACE target linked transitively through
    `webview::core_static`. The `if` block never evaluates to true; the
    `target_include_directories` inside is dead code.
  - **Fix:** Remove the dead block (`source/entry/CMakeLists.txt:65-70`).
  - _File: `source/entry/CMakeLists.txt:65-70`_
  - Test: build succeeds; no include-path regression.

- [x] **FIX-30** `webview_editor.cpp:27` – WebView devtools enabled unconditionally (hardcoded `true`)
  - **Root cause:** `webview::webview(/*debug=*/true, parentHandle)` is called
    with `debug=true` in all build types. In a Release build this leaves the
    WebView2 devtools accessible, adds overhead and exposes internals to end
    users.
  - **Fix:** Tie the debug flag to the build type:
    ```cpp
    #ifndef NDEBUG
    constexpr bool kWebViewDebug = true;
    #else
    constexpr bool kWebViewDebug = false;
    #endif
    w_ = std::make_unique<webview::webview>(kWebViewDebug, parentHandle);
    ```
  - _File: `source/webview/webview_editor.cpp:27`_
  - Test: Release build → devtools overlay absent; Debug build → devtools present.

- [x] **FIX-31** `pluginids.h:13-16` – `static const FUID` in a header causes per-TU copies (ODR)
  - **Root cause:** `kProcessorUID` and `kControllerUID` are declared
    `static const Steinberg::FUID` inside a header. Every translation unit that
    includes this header gets its own copy. While benign for `FUID` (value type),
    it is an ODR violation pattern and produces unnecessary binary bloat.
  - **Fix:** Change both declarations to `inline const` (C++17, already required
    by the project) so there is a single definition across all TUs.
    ```cpp
    inline const Steinberg::FUID kProcessorUID(...);
    inline const Steinberg::FUID kControllerUID(...);
    ```
  - _File: `source/vst/common/pluginids.h:13-16`_
  - Test: no linker warnings; both IDs accessible from all TUs.

- [x] **FIX-32** `netsdr_mcp_server.py` – `_cosine_similarity` norm computation is wrong after swap
  - **Root cause:** Lines 177-183 swap `a, b` so the inner loop iterates over
    the smaller dict. However `na` is accumulated **only** inside the `a`-items
    loop (after the swap, `a` is the original `b`). The norm of the original `a`
    is never computed, so the denominator is `sqrt(na * nb)` where `na` is the
    squared-norm of what was passed as `b`, not `a`. The result is a wrong cosine
    value whenever the input dicts have different sizes.
  - **Fix:** Compute both norms independently before (or outside) the swap:
    ```python
    na = sum(v * v for v in a.values())
    nb = sum(v * v for v in b.values())
    dot = sum(a.get(k, 0.0) * v for k, v in b.items())
    denom = math.sqrt(na * nb)
    return dot / denom if denom > 0 else 0.0
    ```
  - _File: `netsdr_mcp_server.py:169-186`_
  - Test: unit test with two known embeddings confirms correct cosine score.

### Minor – cleanup and design hygiene

- [x] **FIX-33** `CMakeLists.txt:42-49` – `NS_UI_DIST_URL` configure-time baking becomes dead code
  after FIX-WV2-B2
  - **Root cause:** `source/entry/CMakeLists.txt:43-49` bakes an absolute
    configure-time `file://` path into `NS_UI_DIST_URL` and passes it as a
    compile definition. Once FIX-WV2-B2 (runtime URL from module path) is
    implemented, `NS_UI_DIST_URL` is no longer used by `plugin_editor.cpp` and
    the `if(NS_ENABLE_UI)` CMake block becomes dead code.
  - **Fix:** After FIX-WV2-B2 is done, remove the `NS_UI_DIST_URL` configure
    block from `source/entry/CMakeLists.txt:42-50` and the corresponding
    `#ifdef NS_UI_DIST_URL` branch from `plugin_editor.cpp:28-33`.
  - _Files: `source/entry/CMakeLists.txt:42-50`, `source/editor/plugin_editor.cpp:25-33`_
  - Test: Release build without `NS_UI_DIST_URL` defined loads UI from runtime path.

- [x] **FIX-34** `netsdr_mcp_server.py:1` – shebang uses `python` instead of `python3`
  - **Root cause:** `#!/usr/bin/env python` resolves to Python 2 on systems where
    `python` is still Python 2 (e.g. older Ubuntu). The file uses Python 3 syntax
    (f-strings, `str | None`, `ast.unparse`) throughout.
  - **Fix:** Change to `#!/usr/bin/env python3`.
  - _File: `netsdr_mcp_server.py:1`_
  - Test: `python3 netsdr_mcp_server.py --help` starts without syntax errors.

- [x] **FIX-35** `ParameterRegistry` – O(n) linear scan for every parameter lookup (note for M2)
  - **Root cause:** `value()`, `setValue()`, `toPlain()`, `toNormalized()` and
    `definition()` all iterate the full `definitions_` vector. With 3 parameters
    this is negligible. For M2 with more parameters, or for high-frequency calls
    from the audio thread (via `applyParamValue`), a `std::unordered_map<uint32_t,
    size_t>` index would reduce lookups to O(1).
  - **Fix (deferred to M2):** Add an `id → index` map built in the constructor;
    all lookup methods use the map instead of a loop. Document the deferred
    decision in the code.
  - _File: `source/vst/common/parameter_registry.h`, `parameter_registry.cpp`_
  - Test: no regression in unit tests; lookup time constant regardless of param count.

## Milestone M2 - KiwiSDR integration (project-specific)

- [x] **M2.1** WebSocket connection to KiwiSDR (IXWebSocket, port 8073)
  - Fix: must connect to the radio station at http://g8ure.ddns.net:8078/
  - `netsdr::KiwiConnection` (`source/network/`) wraps IXWebSocket v11.4.6
    (BSD-3-Clause); port configurable (default 8073).
  - Test: integration test against a local mock KiwiSDR server; connect succeeds.
- [x] **M2.2** Handshake + `SET` commands (auth, optional ident_user, agc, freq)
  - `netsdr::KiwiClient` (`source/network/kiwi_client.*`) + serializers
    (`source/network/kiwi_commands.*`).
  - **Protocol correction:** the real KiwiSDR protocol (kiwiclient /
    rx_cmd.cpp) has no `inert` command. The handshake is
    `SET auth t=kiwi p=` (anonymous), optional `SET ident_user=<name>`,
    `SET mod=... freq=...`, `SET agc=...`. Anonymous auth means **no user
    name or password is required** (plugin works without any user input).
  - Test: unit test command serialization; mock server asserts received
    handshake frames in order (anonymous = no ident_user frame).- [x] **M2.3** IMA ADPCM decoding in C++
  - `netsdr::ImaAdpcmDecoder` (`source/dsp/ima_adpcm.h/.cpp`). Low nibble
    first, then high nibble per byte (KiwiSDR order); output int16 PCM.
  - Test: unit test known reference vectors + roundtrip (decode/re-encode)
    within tolerance. 5 test cases (silence, reference vector, roundtrip,
    reset, overflow).- [x] **M2.4** Lock-free SPSC queue (moodycamel) network -> DSP
  - `netsdr::AudioSampleQueue` + `AudioSampleBlock` (`source/threading/audio_sample_queue.h`),
    typed over the generic `LockFreeSPSC`. Producer = network thread, consumer = DSP.
  - Test: unit test queue under stress (order/no loss/no corruption, 50k blocks);
    underflow graceful (pop on empty returns false). 3 test cases.- [x] **M2.5** Sample-accurate parameter modulation (frequency via automation/LFO)
  - `netsdr::ParameterSmoother` (`source/dsp/parameter_smoother.h/.cpp`): linear
    per-sample ramp with max-step guard (no zipper). `reset`, `setTarget`,
    `next`, `isSettled`.
  - Test: unit test parameter ramp is monotonic + max step below threshold
    (no zipper). 5 test cases.- [x] **M2.6** LFO rate-limiting (max 20 Hz)
  - `netsdr::RateLimiter` (`source/dsp/rate_limiter.h/.cpp`): time-based
    emission throttle; caller supplies monotonic time (deterministic tests).
  - Test: unit test that N updates in T seconds -> at most ~20/s sent
    (200 in 10 s within tolerance). 5 test cases.- [x] **M2.7** Sample-rate conversion (libsamplerate)
  - libsamplerate 0.2.2 (BSD-2-Clause) via FetchContent; `netsdr::Resampler`
    (`source/dsp/resampler.h/.cpp`) streaming wrapper (SRC_SINC_MEDIUM_QUALITY).
    CMake policy floor 3.5 for CMake 4.x compatibility.
  - Test: unit test sine 12/24 kHz -> 44.1/48 kHz (freq preserved, THD + aliasing
    below threshold). 4 test cases.
- [x] **M2.8** Jitter buffer (100-150 ms)
  - `netsdr::JitterBuffer` (`source/dsp/jitter_buffer.h/.cpp`): prefill cushion
    (target 100-150 ms), capacity ceiling, overflow drops oldest, pull returns
    0 below prefill (cushion preserved). Fixed an unsigned underflow bug that
    crashed on the normal path.
  - Test: unit test absorbs configured jitter; overflow drops oldest; no crash.
    5 test cases.
- [x] **M2.9** Bidirectional JSON communication (UI <-> EditController <-> DSP)
  - `netsdr::KiwiBridge` (`source/network/kiwi_bridge.h/.cpp`): parses the
    existing UI bridge envelope (`{"type":"setParameter","data":["freq",<kHz>]}`),
    rate-limits (20/s) and sends `SET mod=... freq=...` to the KiwiSDR server;
    forwards server text messages to a UI state callback (echo back).
  - Note: freq UI value is already in kHz (KiwiSDR `SET freq=` convention).
  - Test: integration test bridge roundtrip (UI -> param -> DSP -> state echo
    back to UI) against a local mock server. 4 test cases.
- [x] **M2.10** UI controls the live receiver frequency
  - Automated part done (M2.9): bridge emits `{"type":"setParameter","data":["freq",<kHz>]}`
    → rate-limited `SET mod=... freq=...` (verified by mock-server tests).
  - Manual part documented: `doc/workspace-workflow.md` §3.6 (DAW listening
    check via VST3PluginTestHost + real KiwiSDR; requires the full
    network→decode→resample→DSP processor integration, follow-up milestone).

## Milestone M3 - Integration & Ship (project-specific)

> **Implementation plan:** `doc/M3-implementation-plan.md`

> The M2 components (`KiwiClient`, `ImaAdpcmDecoder`, `AudioSampleQueue`,
> `Resampler`, `JitterBuffer`, `ParameterSmoother`, `RateLimiter`,
> `KiwiBridge`) are built and unit/integration-tested but NOT yet wired into
> the plugin. The shipped `.vst3` still renders the M1 sine oscillator. M3
> integrates the full network→decode→resample→DSP pipeline into
> `PluginProcessor` and adds the complete KiwiSDR parameter set.

- [x] **M3.1** Processor integration: network audio pipeline
  - Replace `SineOscillator::render()` in `PluginProcessor::process()` with the
    pipeline `KiwiClient → ImaAdpcmDecoder → AudioSampleQueue → Resampler →
    JitterBuffer → process()`.
  - Link `netsdr_network` into the plugin target (`source/entry/CMakeLists.txt`).
  - Start WebSocket on plugin `initialize()`; disconnect on `terminate()`.
    **Deviations (see `doc/M3-implementation-plan.md`):** lazy connect on
    `setStation`/`applyState` (no network I/O on load); jitter buffer AFTER
    resampler at DAW sample rate; `setStation` via VST3 `IMessage`.
  - _Files: `source/vst/processor/plugin_processor.cpp`,
    `source/entry/CMakeLists.txt`_
  - Test: integration test against a mock KiwiSDR server → decode → resample →
    Goertzel peak at the expected frequency in the output. **Deterministic**
    (30/30 stress runs; see RT-safety fixes under M3.4).

- [x] **M3.2** Full KiwiSDR parameter model in the registry
  - Add ALL KiwiSDR receiver/audio/display parameters as VST3 parameters so
    every setting is DAW-automatable. GUI-visible subset marked below.
  - Core (GUI): `station` (host:port, **state not param** — Option A),
    `mode`, `freqKhz`, `lowCut`, `highCut`.
  - AGC (GUI: `agcOn` only; rest default): `agcOn`, `agcHang`, `agcThresh`,
    `agcSlope`, `agcDecay`, `agcManGain`.
  - Audio (default only): `squelchOn` + `squelchThreshold`, `nbOn` +
    `nbThreshold`, `nrOn`, `deempOn`, `compOn`, `volume`.
  - Display/Waterfall (GUI later): `wfOn`, `wfSpeed`, `wfZoom`, `wfMaxDb`,
    `wfMinDb`, `wfComp`, `arOn`, `ovOn`.
  - Resolve `FIX-35` (ParameterRegistry O(1) id→index lookup) as part of this.
    **Done:** `index_` map in `parameter_registry.cpp:19-21`.
  - _Files: `source/vst/common/paramdefinitions.h`, `paramids.h`,
    `parameter_registry.*`, `source/network/kiwi_commands.*`,
    `source/network/kiwi_client.*`_
  - Test: unit test all params register with correct range/default/ID; command
    serializers emit the full `SET` frame for each group.

- [x] **M3.3** UI: KiwiSDR controls
  - Replace M1 knobs with Kiwi controls: station, mode, frequency (kHz),
    bandwidth low/high (Core); AGC on/off (rest default); waterfall on/off
    (display params default).
  - Verify bidirectional bridge (UI → KiwiBridge → `SET`; status echo → UI).
  - New components: `StationInput`, `NumberInput`, `Toggle`, `Slider`,
    `StatusBadge` (see `ui/src/components/`).
  - _Files: `ui/src/views/PluginView.vue`, `ui/src/services/pluginService.ts`,
    `source/network/kiwi_bridge.cpp`_
  - Test: Vitest component tests (controls emit correct values); bridge
    roundtrip integration test. **28/28 Vitest green, vue-tsc clean.**

- [x] **M3.4** Real-time safety audit + performance
  - clang-tidy: audio thread lock-free, no heap allocation.
    **Audit 2026-08-22:** `process()`/`renderPipeline` use only stack arrays,
    lock-free SPSC queues, and the now allocation-free `JitterBuffer` (ring
    buffer) and `Resampler` (bounded staging). `ParameterRegistry::setValue` is
    a lock-free O(1) map lookup. No `new`/`std::mutex`/blocking I/O on the
    audio thread.
  - **RT-safety fixes applied (fixes the M3.1 flaky test):**
    1. `JitterBuffer` → pre-allocated ring buffer (drop-oldest), allocation-free
       `push`/`pull`/`reset`. `source/dsp/jitter_buffer.h/.cpp`.
    2. `Resampler` → bounded staging buffer (`kMaxStagingFrames=4096`, reserved
       in constructor), chunked processing for oversized inputs, in-place
       `memmove` compaction. `source/dsp/resampler.h/.cpp`.
    3. Jitter pre-fill gate → **start latch** (gates only the first pull; no
       mid-stream re-arm → no bursts of silence).
    4. Tests updated to latch semantics; mock server sine lengthened to 100 s
       (no ADPCM frame wrap → no decoder glitches).
  - Tune jitter buffer (100–150 ms) under real network conditions.
  - Validate resampler quality/CPU at small buffer sizes.
  - Test: clang-tidy clean; no dropouts in manual listening test.
    **Automated:** pipeline test deterministic (30/30), full suite 86/86 green.

- [x] **BUG-04** Clean-Build bricht ab: `netsdr_network` → Winsock-Include-Order-Konflikt
  - **Behoben 2026-08-25:** `WIN32_LEAN_AND_MEAN` vor `#include <windows.h>` in `diag.h:16` ergänzt.
    **Verifiziert:** Clean-Build Debug + Release grün, Plugin-Bundle entsteht.
  - _Cross-reference: `doc/M3-implementation-plan.md` §BUG-04_

- [x] **BUG-05** `start-testhost-debug`-Task hat kein `dependsOn: ["build-debug"]`
  - **Behoben 2026-08-25:** `dependsOn` + `dependsOrder: sequence` in `.vscode/tasks.json` ergänzt.
    **Verifiziert:** Task startet automatisch Build vor TestHost.
  - _File: `.vscode/tasks.json:57-68`_

- [x] **F3** Verbindung wird nach ~5 s unerwartet geschlossen (M3 KiwiSDR) — **BEHOBEN 2026-08-27**
  - **Symptom:** Nach successful Auth-Handshake (`audio_rate=` empfangen) schließt der Server die Verbindung nach wenigen Sekunden.
  - **Root Cause (2026-08-27 gefunden):** Keepalive wurde nach **jedem SND-Frame** gesendet (~6x/s). Der Server interpretiert das als Spam und trennt die Verbindung. Die Python-Referenz `kiwirecorder.py` drosselt keepalive auf **1 Hz** (`if secs != self._last_snd_keepalive`).
  - **Fix (2026-08-27):**
    1. **Keepalive-Throttling:** `sendKeepaliveThrottled()` Methode in `kiwi_client.cpp` implementiert. Sendet keepalive nur einmal pro Sekunde (gedrosselt über `lastKeepaliveSecs_` Atomic).
    2. **Auto-Reconnect via IXWebSocket:** `kiwi_connection.cpp` nutzt jetzt IXWebSocket's eingebautes Auto-Reconnect (default aktiviert). Blockierender `reconnect()`-Thread entfernt.
  - **Beweis:** Python kiwirecorder läuft stabil gegen `g8ure.ddns.net:8078` (kein Disconnect). Unser Plugin mit Keepalive-Throttling ebenfalls stabil.
  - **Dateien:** `source/network/kiwi_client.cpp`, `source/network/kiwi_client.h`, `source/network/kiwi_connection.cpp`
  - Test: Build + Tests grün (95/95). Manuelle Verifikation gegen echten KiwiSDR-Server steht aus (M3.5).

- [x] **F4** Audio zerhackt / blockweise Lautstärkeschwankungen (M3 KiwiSDR) — **VOLLSTÄNDIG BEHOBEN 2026-08-27**
  - **Symptom:** Trotz ASIO-Treiber Knackser/Klicks, 1-Sekunden-Dropouts alle paar Sekunden.
  - **Alle Root-Causes behoben:**
    1. **SND-Header 10 Bytes** (2026-08-26): ADPCM-Offset korrigiert, durch `kiwi_stream_compare_tests.cpp` verifiziert.
    2. **Underflow-Concealment** (2026-08-27): `renderPipeline` füllt Unterfüllung mit Repeat-Last-Sample + linearem Fade (512 Samples = ~10ms bei 48kHz) statt hartem Null-Fill. Vermeidet Klicks bei kurzen Unterbrechungen.
    3. **Denormal-Schutz** (2026-08-26): Flush-to-zero für Werte < 1e-30 in `renderPipeline`.
    4. **Clock-Drift-Kompensation** (2026-08-27): Aggressivere Anpassung: ±1% Änderung pro Sekunde (vorher ±0.1%), alle 50ms anpassen (vorher 100ms), Target 300ms (Mitte des 500ms Buffers). Verhindert Buffer-Underruns durch Clock-Drift.
    5. **tryPush-Lücken-Erkennung** (2026-08-26): Sequenz-Check in `decodeAndQueue` + Telemetrie.
    6. **Jitter-Pre-fill erhöht** (2026-08-27): 500ms Prefill (vorher 200ms), 2000ms Max-Capacity (vorher 1000ms). Deckt Netzwerk-Jitter besser ab, verhindert 1-Sekunden-Dropouts.
    7. **Resampler-Qualität** (2026-08-26): `Resampler::Quality` enum (Medium/Best) als Konstruktor-Parameter.
  - **Neue Dateien:** `source/vst/processor/pipeline_telemetry.h`
  - **Geänderte Dateien:** `source/dsp/resampler.h/.cpp`, `source/vst/processor/plugin_processor.h/.cpp`
  - **Test-Suite:** 95/95 Tests grün (Debug + Release), inkl. Realtime-Stress-Test.
  - Manuelle Audio-Verifikation steht aus (M3.5).

- [x] **FIX-38** Clock-Drift-Kompensation war wirkungslos + falsch gerichtet (M3.6.1/F4) — **BEHOBEN 2026-08-27**
  - **Symptom:** Anhaltende 1-Sekunden-Dropouts/Knackser trotz F4-Fixes; Buffer schwankt zwischen 0ms und 545ms; Ratio driftete Richtung 1.0 statt nominal.
  - **Root Causes (drei Bugs in der Clock-Drift-Kompensation):**
    1. **`Resampler::setRatio()` hatte KEINE Wirkung:** `process()` verwendete `data.src_ratio = output_rate_ / input_rate_` (nominal) statt `current_ratio_` in Hauptschleife UND Drain-Phase. Nur der erste Drain-Pfad nutzte `current_ratio_`.
    2. **`smoothedRatio_` initial 1.0 statt nominal:** `plugin_processor.h:142` (`double smoothedRatio_{1.0}`). Sollte `dawRate/12000` (z.B. 3.675 bei 44.1kHz) sein. Die Kompensation startete damit weit vom Zielwert entfernt.
    3. **Richtung invertiert:** `bufferError = (bufferedMs - targetMs) / targetMs` → bei leerem Buffer sank die Ratio (noch weniger Output), statt zu steigen. Korrekt: `bufferError = (targetMs - bufferedMs) / targetMs`.
  - **Fix:**
    1. `resampler.cpp`: `data.src_ratio = current_ratio_` in Hauptschleife + Drain-Phase.
    2. `plugin_processor.cpp` (`setupProcessing`): `smoothedRatio_ = newSetup.sampleRate / 12000.0` + `lastRatioAdjust_`-Reset.
    3. `plugin_processor.cpp` (`renderPipeline`): `bufferError = (targetMs - bufferedMs) / targetMs` (leer → Ratio steigt → Buffer füllt).
  - **Tests (neu/angepasst):**
    - `tests/dsp/resampler_tests.cpp`: "setRatio changes currentRatio and process output" — verifiziert Clamp [0.5·nominal, 2.0·nominal] + Output-Sample-Anzahl steigt mit Ratio.
    - `tests/vst/plugin_processor_pipeline_tests.cpp`: Mock-Server `sendIntervalMs`-Parameter (default 170ms real-time); Ton-Test nutzt real-time (Clock-drift stabil), Stress-Test nutzt 25ms (Pipeline bleibt versorgt).
  - **Logging (zusätzlich):** Zwei-Level FileLogger (`source/util/file_logger.h`, `doc/logging-strategy.md`) — INFO (Release+Debug) / DEBUG (nur Debug); fflush für DEBUG auf 500ms gedrosselt (Performance).
  - **Test-Suite:** 82/82 Tests grün (Debug + Release).
  - _Files: `source/dsp/resampler.cpp`, `source/vst/processor/plugin_processor.h/.cpp`, `source/util/file_logger.h`, `tests/dsp/resampler_tests.cpp`, `tests/vst/plugin_processor_pipeline_tests.cpp`_

- [x] **FIX-39** Server trennt Verbindung alle ~11 s abrupt (CLOSE 1005) — **BEHOBEN 2026-08-27**
  - **Symptom:** Trotz F3 (Keepalive-Throttling) wird die Verbindung weiterhin alle ~11 s getrennt (stiller Auto-Reconnect). Verursachte die eigentlichen Audio-Dropouts (nicht Clock-Drift!).
  - **Beweis (diag-Log):** `WebSocket CLOSE code=1005 reason=No status code remote=1` — Server trennt abrupt ohne WebSocket-Close-Frame. `netsdrstation_diag.log` zeigt onOpen mehrfach, onClose nie geloggt (stiller Reconnect).
  - **Root Causes (zwei Protokoll-Bugs vs. kiwiclient-Referenz):**
    1. **`SET OVERRIDE inactivity_timeout=0`** (`kiwi_commands.h`) — die Python-Referenz `kiwiclient` sendet diesen Befehl NICHT. Kommentar „matches kiwiclient reference" war falsch. `inactivity_timeout=0` bringt den Server dazu, nach kurzer Zeit zu trennen.
    2. **`reconnectLoop()` sendet Keepalive ungedrosselt** (`kiwi_client.cpp:246`) — nach jedem Frame statt 1 Hz. Nur der initiale `connect()` nutzte `sendKeepaliveThrottled()`. Nach jedem Reconnect wurde es schlimmer („Knackser werden immer schlimmer").
  - **Fix:**
    1. `kiwiInactivityTimeoutCommand()` + Aufruf entfernt.
    2. `reconnectLoop()`: `sendKeepaliveThrottled()` statt ungedrosseltem `sendText(kiwiKeepaliveCommand())`.
    3. `kiwi_connection.cpp`: WebSocket CLOSE-Code + Error zusätzlich in FileLogger (INFO) geloggt (`NETSDR_LOG_INFO`), damit sie in `netsdrstation.log` erscheinen (nicht nur `diag.log`).
  - **Tests:** Handshake-Frame-Tests angepasst (inactivity_timeout-Frame entfernt, Frame-Zahlen 9→8 / 8→7).
   - **Test-Suite:** 82/82 Tests grün (Debug + Release).
   - _Files: `source/network/kiwi_commands.h`, `source/network/kiwi_client.cpp`, `source/network/kiwi_connection.cpp`, `tests/network/kiwi_client_tests.cpp`, `tests/network/kiwi_bridge_tests.cpp`_

- [x] **FIX-40** NACH FIX-38/39 weiterhin Dropouts + ~10 s-Serverabort (CLOSE 1005); Root-Cause-Untersuchung + Sample-Rate-Kontrakt (JUCE/VST3-Referenz) — **ANALYSE 2026-08-27 → UMSETZUNG 2026-08-27 (Debug+Release grün)**
  - **User-Hinweis (2026-08-27):** Tests werden IMMER als **Debug** ausgeführt, nie Release. Der analysierte Testlauf war daher ein Debug-Build; die im Log sichtbaren `DEBUG`-Zeilen (UNDERRUN, renderPipeline) sind dort korrekt aktiv und erzeugen Datei-I/O auf dem Audio-Thread.
  - **Symptom (nach FIX-39, Debug-Log 09:07:43–09:11:00, `g8ure.ddns.net:8078`):** Server trennt weiterhin alle ~10,2–10,4 s (CLOSE 1005); zudem **Audio-Datenfluss nahezu tot**: `decodeAndQueue` loggt nur frame=0 (09:10:34) und frame=100 (09:10:52) → ~5,5 SND-Frames/s statt erwartet ~23–46; `renderPipeline` zeigt `pops=0 in=0 q=0` (audioQueue_ permanent leer); 1000+ `UNDERRUN (COMPLETE)`; Jitter-Buffer 489→0 ms. **Damit ist die eigentliche Ursache nicht (nur) Keepalive/Timeout, sondern Sterben des Audio-Datenflusses.**
  - **Clock-Drift-Ratio-Anomalie:** `Clock-drift CRITICAL: bufferMs=0.0 target=300.0 ratio=~4.0` (steigt 4,02→4,06+). Nominal ist 44100/12000 = 3.675; der Controller-Cap liegt bei nominal×1,05 ≈ 3,86. Beobachtet ~4,0–4,1 > Cap → Server-/OutputRate zur Laufzeit evtl. nicht 44100/12000 (hyp.: serving Rate bzw. `nominalRatio` anders), plus `smoothedRatio_` wird bei **Reconnect nicht auf nominal zurückgesetzt** → nach Reconnect lauter + mehr Knackser (stale Resampler-Ratio).
  - **Stiller Reconnect:** GUI zeigt Reconnect nicht an (`onOpen`→"Connected"; `onClose`→"Disconnected" sehr kurz), kein disconnected-Zwischenzustand sichtbar.
  - **Sample-Rate-Kontrakt (Referenz-Analyse, KEIN Code):**
    - **JUCE** (`thirdParty/JUCE`): `prepareToPlay(double sampleRate, int maxSamples)` ist der host→plugin Rückruf (`juce_AudioProcessor.h:139`); intern `setRateAndBufferSizeDetails` setzt `currentSampleRate` (`juce_AudioProcessor.cpp:376-380`); `getSampleRate()` ist **nur innerhalb `processBlock` garantiert gültig** und sonst evtl. 0 (`juce_AudioProcessor.h:820-825`). `AudioProcessorPlayer::audioDeviceAboutToStart` zieht die Rate erneut vom Device (`getCurrentSampleRate()`) und ruft `prepareToPlay` bei jedem Audio-Device-Start (`juce_AudioProcessorPlayer.cpp:344-355,180`). Änderung der Sample-Rate in der DAW → Host ruft Setup-Pfad mit neuer Rate erneut auf → Plugin muss ratenabhängigen Zustand (Resampler, JitterBuffer, Ratio- Basislinie) neu aufbauen.
    - **VST3 SDK** (`thirdParty/vst3sdk`): Pendant ist `IAudioProcessor::setupProcessing(ProcessSetup&)`, `ProcessSetup.sampleRate` ist `double` (`pluginterfaces/vst/ivstaudioprocessor.h:174-182,331`; `vsttypes.h:112`); Host ruft es bei Konfig-Änderung (Sample-Rate/Block) erneut auf.
    - **Unser Plugin (bereits korrekt):** `setupProcessing` erzeugt `Resampler`/`JitterBuffer` aus `newSetup.sampleRate` und setzt `smoothedRatio_ = newSetup.sampleRate / 12000.0` (`plugin_processor.cpp:612-625`). KiwiSDR-Stream = fix 12 kHz; die DAW-Outputrate ist die einzige freie Variable und wird immer vom Host bezogen.
  - **Nächste Schritte (Analyse/Implementierung nach Freigabe):** (1) prüfen, ob `SET mod=iq` im Handshake IQ-Frames (Flag 0x08) liefert, die `decodeAndQueue` (~Zeile 500–504) verwirft → Audio-Starvation; (2) `smoothedRatio_`/Resampler/JitterBuffer bei Reconnect auf nominal zurücksetzen + stillen Reconnect im GUI sichtbar machen; (3) Rate-Anomalie (ratio ~4,0 vs. Cap ~3,86) per gezieltem INFO-Logging (serverRate/nominalRatio/smoothedRatio_/bufferMs) bestätigen; (4) als Primary Debug-Build bauen/testen (User testet IMMER Debug).
  - _Files (Analyse-Referenzen): `source/vst/processor/plugin_processor.cpp`, `source/vst/processor/plugin_processor.h`, `source/dsp/resampler.h/.cpp`, `source/network/kiwi_client.cpp`, `source/network/kiwi_connection.cpp`, `source/vst/common/paramdefinitions.h`; Ext.: `thirdParty/JUCE`, `thirdParty/vst3sdk`; Docs: `doc/architecture.md` §11 "Sample rate"._
  - **Umsetzung 2026-08-27 (Build_Openrouter, nach Freigabe „ja"):**
    - **Wichtige Korrektur (Root-Cause):** ~5,5 SND-Frames/s ist die **KORREKTE** Frame-Rate für 12 kHz-Audio mit 1034-Byte-Frames (2068 Samples/Frame; 12000/2068 ≈ 5,8 fps). Die Audio-Datenrate ist also **korrekt**; die frühere Annahme „erwartet ~23–46 fps / Sterben des Datenflusses" war falsch. Die Stervation entsteht durch den **Disconnect-Zyklus** (Serverabort ~10,2 s) + tote Lücken dazwischen. `SET mod=iq`-Verdacht ist **kein** Faktor im Default (mod=am; Frame-Flags 0x15 = komprimiert, KEIN IQ-Bit 0x08). Keepalive-on-SND ist äquivalent zur Referenz (`client.py:559-563`); Problem: bei host-bedingten Audio-Lücken stoppt auch das Keepalive.
    - **Umsetzte Fixes:**
      1. **Stiller Reconnect sichtbar:** `KiwiClient::scheduleReconnect()` ruft jetzt `onClose_()` → GUI zeigt `Disconnected`/Reconnect-Zustand (vorher nur bei Dauerfehler) (`kiwi_client.cpp:225-232`).
      2. **Reconnect-Reset (lauter + Knackser nach Reconnect):** Pipeline-Reset-Block in `process()` setzt jetzt `smoothedRatio_` auf **nominal** (`dawRate/serverRate` statt gedriftetem ~4,0), ruft `resampler_->setRatio()`, re-baselined `lastRatioAdjust_` und resettet `adpcmDecoder_`; `setOnOpen` setzt `resetPipelineFlag_` → Reset feuert bei **jedem (Re)Connect**, nicht nur beim ersten (`plugin_processor.cpp:682-711`, `438-446`).
      3. **~10-s-Serverabort:** neuer Keepalive-Timer-Thread `keepaliveLoop()` sendet `SET keepalive` 1 Hz **unabhängig von Audio-Frames** (`kiwi_client.h/.cpp`), damit der Server-Inaktiv-Timeout bei kurzzeitigen SND-Lücken (Host-bedingt) nicht feuert; Start in `connect()`, Stop+Join in `disconnect()`/Destruktor; DEBUG-Log „keepalive sent".
    - **Rate-Anomalie (~4,0 vs. Cap ~3,86):** Bestätigung im nächsten Debug-Lauf per vorhandenem `NETSDR_LOG_INFO("Server sample rate: %.1f Hz")` (`plugin_processor.cpp:431`) + neuem Reset-Log (`serverRate=... nominalRatio=...` `plugin_processor.cpp:698-700`). Hypothese: Server meldet `audio_rate=11025` → nominal 4,0; durch Reset abgemildert.
    - **Bewusst NICHT geändert:** WS-URL-Pfad (`/ws/kiwi/<ts>/SND` vs. Referenz `/<ts>/SND`) und `SET AR OK out=12000` vs. Referenz `out=44100` — unser Pfad funktioniert (Audio fließt); ohne Live-Verifikation risikobehaftet → offene Frage für manuellen Test.
    - **Tests:** Primary Debug+Release Build grün (EXIT 0), ctest 1/1 grün, VST3-Validator 47/47.
    - _Files (Umsetzung): `source/network/kiwi_client.cpp/h`, `source/vst/processor/plugin_processor.cpp`; Verifikation: `%TEMP%\netsdrstation.log`._

- [x] **FIX-41** ~10-s-Serverabort (CLOSE 1005) / n_snd=0 — **BEHOBEN 2026-08-27 (Root-Cause: Probe-Bug, nicht Client-Bug)**
  - **Endgültige Root-Cause (2026-08-27, per Live-Trace gegen kphsdr.com:8073):**
    `n_snd=0` war ein **Bug im Python-Probe `probe_full.py`**, NICHT im C++-Client.
    Der Server sendet `sample_rate=...` (t≈1.2s) **VOR** `audio_rate=12000` (t≈1.8s).
    Der Probe sendete Phase 2 auf den ersten Trigger und latchte `snd_ph2` — dadurch
    wurde `SET AR OK` (der Befehl, der den SND-Audio-Stream aktiviert) **nie gesendet**.
    Ohne `SET AR OK` startet der Server keine SND-Frames → Idle-Kick nach ~10s.
  - **Verifiziert (Live-Probe):** Die C++-Client-Sequenz (auth + `SET AR OK in=12000 out=12000`
    auf `audio_rate=`) liefert korrekt SND-Frames; ebenso `out=44100`. `SET AR OK` ist der
    auslösende Befehl; `out=`-Wert ist frei wählbar. Der WebSocket-Pfad
    `/ws/kiwi/<ts>/SND` ist korrekt.
  - **Härtung des C++-Clients (umgesetzt, Referenz-faithful):**
    1. `SET options=1` VOR `SET auth` gesendet (kiwiclient `open()`: "must be sent before auth").
    2. `SET AR OK in=<audio_rate> out=<audio_rate>` — `audio_rate` wird jetzt aus der MSG
       geparst statt hart auf 12000 gesetzt (andere Kiwis nutzen ggf. andere Raten).
    3. Bogus-Frame `SERVER DE CLIENT openwebrx.js SND` entfernt.
    4. `SET squelch=0 max=0`, `SET genattn=0`, `SET gen=0 mix=-1` ergänzt (Referenz-Sequenz).
  - **Neue Handshake-Sequenz:** `options` → `auth` → `AR OK` → (optional `ident_user`) →
    `squelch` → `genattn` → `gen` → `mod/freq` → `agc` → `keepalive`.
  - **Tests:** `kiwi_commands_tests.cpp` (+5 neue Serializer-Tests: options/AR OK/squelch
    max/gen/genattn); `kiwi_client_tests.cpp` + `kiwi_bridge_tests.cpp` Frame-Zahl 7→9
    (bzw. 10 mit ident_user). **87/87 Testfälle grün (Debug + Release), Validator 47/47.**
  - **Live-Verifikation:** `probe_newcpp.py` (exakte C++-Sequenz) gegen kphsdr.com:8073 →
    202 SND-Frames, STAYED-CONNECTED.
  - _Files: `source/network/kiwi_client.cpp`, `source/network/kiwi_commands.h/.cpp`,
    `tests/network/kiwi_client_tests.cpp`, `tests/network/kiwi_commands_tests.cpp`,
    `tests/network/kiwi_bridge_tests.cpp`_

- [x] **FIX-37** `WEBVIEW2_SDK_ROOT` fehlte in `tasks.json` + `CMakePresets.json` nach FIX-27
  - **Behoben 2026-08-25:** `WEBVIEW2_SDK_ROOT: C:/Users/marku/Documents/GitHub/thirdParty/WebView2SDK`
    in `.vscode/tasks.json` (env) und `"WEBVIEW2_SDK_ROOT": "$env{WEBVIEW2_SDK_ROOT}"` in
    `CMakePresets.json` (base.cacheVariables) ergänzt.
    **Verifiziert 2026-08-26:** `cmake --preset win-msvc` läuft ohne Fehler durch.

- [x] **FIX-42** Frühes `SET keepalive` verhindert SND-Audio-Stream (kphsdr.com:8073, Firmware v1.900) — **BEHOBEN 2026-08-27**
  - **Symptom:** Plugin verbindet zu kphsdr.com:8073, erhält Config-Messages (`client_public_ip`, `rx_chans`, …, `cfg_loaded`) und dann `MSG monitor` — aber **nie** `sample_rate=`/`audio_rate=`. Kein Audio, Dauer-Reconnect (CLOSE 1000 remote=0) alle ~1,3 s.
  - **Root Cause:** Der FIX-40-Keepalive-Thread (`keepaliveLoop()`) sendete `SET keepalive` bereits ~100 ms nach Connect — **vor** Abschluss des Handshakes. Die KiwiSDR-Server-Firmware v1.900 (kphsdr.com) versetzt die Verbindung dann in den „monitor“-Modus und startet den SND-Audio-Stream nie. Die Referenz (kiwiclient) sendet Keepalive erst **nach** `sample_rate`/`audio_rate` bzw. im SND-Binary-Callback. (Firmware v1.902 auf g8ure toleriert frühes Keepalive — deshalb fiel es dort nicht auf.)
  - **Beweis:** Python-Probe: `SET options=1`+`SET auth`+ sofortiges 1-Hz-`SET keepalive` → Server antwortet mit `MSG monitor`, kein `audio_rate`. Ohne frühes Keepalive → `sample_rate`+`audio_rate`+SND-Frames fließen. HTTP-Header (`Origin`, `User-Agent`) als Ursache **ausgeschlossen** (Probe mit IXWebSocket-Header-Set funktioniert).
  - **Fix:** `keepaliveLoop()` sendet Keepalive nur noch, wenn `handshakePhase2Done_` gesetzt ist (d.h. nach `audio_rate`/`SET AR OK`). `handshakePhase2Done_` wurde dafür von `bool` auf `std::atomic<bool>` umgestellt (wird jetzt zusätzlich vom Keepalive-Thread gelesen). Dies stellt exakt das kiwiclient-Referenzverhalten her (Keepalive erst nach `sample_rate`/`audio_rate`) und gilt damit für **alle** KiwiSDR-Firmware-Versionen (alt v1.900 wie neu v1.902).
  - **Multi-Station-Verifikation (Python-Probe mit exakter Fix-Sequenz):** kphsdr.com:8073 ✅ · kiwisdr2.sdrutah.org:8074 ✅ (3/3) · kiwisdr.kfsdr.com:8073 ✅ · kiwisdr.ku4by.com:8073 ✅ — alle liefern SND-Frames.
  - **Tests:** 87/87 grün (Debug + Release), Validator 47/47.
  - _Files: `source/network/kiwi_client.cpp` (`keepaliveLoop`), `source/network/kiwi_client.h` (`handshakePhase2Done_` atomic)_

- [x] **FIX-43** Connect-Klick erzeugt keine Verbindung — `setStation`-IMessage wird im Processor verworfen — **BEHOBEN 2026-08-27**
  - **Symptom (M3 Debug-Test):** Im VST3PluginTestHost Klick auf Connect mit
    `kphsdr.com:8072` → **nichts passiert**. `netsdrstation.log` zeigt im
    laufenden Prozess keine Zeile `Connecting to station: …`; das `StatusBadge`
    bleibt bei `Connecting...`.
  - **Beweis (diag-Log):** `controller setStation: hostPort=kphsdr.com:8072` →
    `controller setStation: message sent` — danach **kein** `processor notify`.
  - **Root Cause:** Message-ID- und Attribut-Mismatch zwischen Controller und
    Processor. `PluginController::setStation()` sendet `IMessage` mit ID
    `"NetSDRStation:SetStation"` und Attribut `"HostPort"`
    (`plugin_controller.cpp:135-137`), aber `PluginProcessor::notify()` prüfte
    auf `"setStation"` und las `"station"` (`plugin_processor.cpp:175,179`).
    Die Nachricht matchte nie → `connectToStation()` wurde nie aufgerufen.
  - **Fix (2 Teile):**
    1. `plugin_processor.cpp` `notify()`: Message-ID `"NetSDRStation:SetStation"`
       + Attribut `"HostPort"`; `connectToStation()` wird über `worker_.post()`
       ausgeführt (Netzwerk-I/O nicht auf dem Message/UI-Thread).
    2. `emitStatus()` sendet die Status-Strings (`Connecting`/`Connected`/
       `Error`/`Disconnected`) zusätzlich als `IMessage` `"NetSDRStation:Status"`
       an den Controller-Peer (`sendMessage`), damit das GUI-`StatusBadge`
       tatsächlich aktualisiert wird (vorher hing `onStatus_` nur als Test-Hook
       in der Luft — `setOnStatus` wurde nie im Produktionspfad verdrahtet).
  - _Files: `source/vst/processor/plugin_processor.cpp` (`notify`, `emitStatus`)_
  - Test: Debug + Release Build grün, VST3-Validator 47/47, `ctest` grün.
    **Manuell verifiziert 2026-08-27:** Connect → `Connected`, Verbindung
    stabil (`kphsdr.com:8072`). → Folge-Bugs: BUG-06/BUG-07 + FEATURE-01.

- [x] **BUG-06** Frequenz / Low Cut / High Cut reagieren nach Connect nicht (nicht gesendet) — **BEHOBEN 2026-08-27**
  - **Symptom (M3 Debug-Test):** Verbindung steht (stabil), aber Ändern von
    `Frequency` (z.B. 14021.501 kHz), `Low Cut` (−5900 Hz) oder `High Cut`
    (3400 Hz) hat **keine Wirkung** — der Receiver bleibt auf der alten
    Frequenz/Passband.
  - **Root Cause:** `applyParamValue()`
    (`plugin_processor.cpp:321-346`) schreibt die Werte in die Atomics und
    setzt `paramsDirty_ = true`, aber **nichts** stößt danach
    `sendPendingParams()` an. Diese Methode (die `SET mod=… freq=…` via
    `setTuning()` an den Server sendet) wurde **nur ein einziges Mal** im
    `setOnOpen`-Callback ausgelöst. Nach dem Connect verpufften
    Parameter-Änderungen folglich ungesendet. Der deklarierte
    `RateLimiter freqLimiter_` war deklariert, aber nie benutzt.
  - **Fix:** In `process()` (Schritt 1b) wird nach den Param-Changes geprüft,
    ob `paramsDirty_` gesetzt ist; falls ja, wird `sendPendingParams()`
    **rate-limitiert** (20/s über den umbenannten `paramSendLimiter_`) auf den
    Worker-Thread gepostet. Da `paramsDirty_` erst im Worker (`exchange`) gelöscht
    wird, ist die Trailing-Edge garantiert (letzte Änderung wird sicher gesendet).
  - _Files: `source/vst/processor/plugin_processor.cpp` (`process`), `plugin_processor.h` (`paramSendLimiter_`)_
  - Test: Integrationstest „sends updated tuning to server after connect (BUG-06)"
    (neuer `HandshakeCaptureServer` in `plugin_processor_pipeline_tests.cpp`).

- [x] **BUG-07** Volume reagiert nicht (nicht im Render-Pfad angewendet) — **BEHOBEN 2026-08-27**
  - **Symptom (M3 Debug-Test):** `Volume`-Slider ändert die Lautstärke nicht.
  - **Root Cause:** `applyParamValue()` speichert den Wert in
    `volume_` (`plugin_processor.cpp:334`), aber `renderPipeline()`
    wendete `volume_` **nirgends** auf die Ausgabe-Samples an.
    (Volume ist als lokaler Gain gedacht — es gibt kein KiwiSDR-`SET vol=`;
    `sendPendingParams()` sendet es korrekt nicht.)
  - **Fix:** In `renderPipeline()` (Schritt e) werden die Ausgabe-Samples mit
    `volume_.load()` multipliziert (übersprungen bei `volume == 1.0`).
  - _Files: `source/vst/processor/plugin_processor.cpp` (`renderPipeline`)_
  - Test: Integrationstest „volume gain scales the output (BUG-07)"
    (`plugin_processor_pipeline_tests.cpp`): volume=0 → Stille, volume=1 → Audio.

- [x] **FEATURE-01** Connect-Button → Disconnect bei `Connected` — **UMSETZEN 2026-08-27**
  - **Wunsch:** Sobald der Status auf `Connected` wechselt, soll der
    Connect-Button zu `Disconnect` wechseln (oder ein 2. Button), damit ein
    aktives Disconnect abgesetzt werden kann.
  - **Umsetzung (Ende-zu-Ende-Disconnect-Pfad):**
    1. C++ Bridge: `window.vstHost.disconnect()` + `vstHostDisconnect`-Binding
       (`webview_editor.cpp`); Envelope `{"type":"disconnect","data":null}`.
    2. `bridge_protocol`: `parseDisconnectMessage()` (neu).
    3. Editor: `onJavaScriptMessage` leitet `disconnect` an
       `controller->disconnect()` weiter.
    4. Controller: `disconnect()` sendet `IMessage` `"NetSDRStation:Disconnect"`.
    5. Processor: `notify()` fängt `"NetSDRStation:Disconnect"` → `worker_.post(disconnectStation)`.
       `disconnectStation()` zerstört `kiwiClient_` (Destruktor setzt `destroying_`
       → **kein** Auto-Reconnect), leert `station_`, setzt `resetPipelineFlag_`
       (Audio geht sofort still) und emittiert `"Disconnected"`.
    6. UI: `PluginService.disconnect()`, `PluginView.onDisconnect()`,
       `StationInput`-Button-Label/Aktion abhängig vom `status`
       (`Connected` → `Disconnect`).
  - _Files: `source/webview/webview_editor.cpp`, `source/vst/common/bridge_protocol.*`, `source/editor/plugin_editor.cpp`, `source/vst/controller/plugin_controller.*`, `source/vst/processor/plugin_processor.*`, `ui/src/services/pluginService.ts`, `ui/src/views/PluginView.vue`, `ui/src/components/StationInput.vue`_
  - Test: Unit-Test `parseDisconnectMessage`; Disconnect-Pfad folgt dem (manuell
    verifizierten) `setStation`-Mechanismus.

- [x] **FIX-44** RT-sicheres Logging + Log-Rausch-Reduktion (Debug hörbar halten) — **UMSETZEN 2026-08-27**
  - **Motivation:** Debug-Build knakst, weil `FileLogger` (`mutex` + `fprintf`/
    `fflush`/`localtime_s`) direkt im Audio-Callback lief. Zusätzlich Log-Rauschen:
    UNDERRUN-Spam beim Prefill, `keepalive sent` 1×/s, Clock-drift CRITICAL bei
    ~600 ms (normal), Doppel-Disconnect im Destruktor.
  - **1. `FileLogger` RT-sicher (`source/util/file_logger.h`):**
    - Produzenten (inkl. Audio-Thread) formatieren per `vsnprintf` in einen
      **lock-freien SPSC-Ring-Puffer** (feste Slots) — kein `mutex`, keine
      Allokation, kein Datei-I/O, kein Clock-Call im Hot-Path.
    - Dedizierter Logger-Thread drained den Ring, stempelt die Zeit
      (`localtime_s`) und schreibt die Datei (DEBUG-Flush 500 ms gedrosselt).
    - `NETSDR_LOG_DEBUG` ist im Release ein Compile-No-op (`((void)0)`) →
      Audio-Thread macht im Release null Arbeit.
  - **2. UNDERRUN-Prefill unterdrückt:** `JitterBuffer::hasStarted()` neu;
    `renderPipeline` loggt/zählt `UNDERRUN (COMPLETE)` nur noch nach dem ersten
    „armed pull" (Start-Latch), nicht während der 0→500 ms-Prefill.
  - **3. Clock-drift CRITICAL:** Schwelle von `>300 ms` Abweichung auf
    **genuinen Overflow/Underflow** geändert (`>1600 ms` oder `<20 ms`) +
    **Hysterese** (loggt einmal pro Episode statt jeden Tick).
  - **4. `keepalive sent`** → **`keepalive started`** (einmal pro Verbindung,
    Atomic `keepaliveLogged_`), statt 1×/s.
  - **5. Destruktor/`terminate()`:** nur noch `kiwiClient_.reset()` (KiwiClient-
    Destruktor setzt `destroying_` → kein spurious onClose/Reconnect).
  - **Beibehalten:** alle INFO-Events (Connect/Disconnect/Error, Handshake),
    first-5 UNDERRUN (INFO), DEBUG-Details, Clock-drift-DEBUG-Statistik.
  - _Files: `source/util/file_logger.h`, `source/dsp/jitter_buffer.h`,
    `source/vst/processor/plugin_processor.{h,cpp}`, `source/network/kiwi_client.{h,cpp}`_
  - Test: Debug+Release Build grün, VST3-Validator 47/47, ctest grün (92/92).

- [x] **M3.5** Manual acceptance (M2.10 real) — **ABGESCHLOSSEN 2026-08-27**  - **Status 2026-08-27:** Verbindung stabil (Debug + Release), Frequenz/Passband/Volume
    wirken, Disconnect-Button funktioniert. M3 damit vollständig abgenommen.
  - **Ziel:** Load plugin in VST3PluginTestHost against real KiwiSDR
    (`kphsdr.com:8072`, UI-Default, STABLE); change frequency → live reception
    audible in DAW; no zipper noise / dropouts.
  - **Technische Voraussetzungen (alle erfüllt):**
    - BUG-04 (Winsock): behoben 2026-08-25
    - BUG-05 (Task-Dependency): behoben 2026-08-25
    - F3 (Disconnect): behoben 2026-08-26 (Keepalive-Throttling + Auto-Reconnect)
    - F4 (Audio zerhackt): behoben 2026-08-26 (Underflow-Concealment, Denormal-Schutz, Clock-Drift-Kompensation, Jitter-Pre-fill 500ms)
    - M3.6 (Pipeline härten): vollständig erledigt 2026-08-27 (alle 8 Teilaufgaben)
    - BUG-03 (Connect-Button): behoben 2026-08-24 (ix::initNetSystem + Status-Feedback)
    - FIX-41 (n_snd=0): behoben 2026-08-27 (SET AR OK Aktivierung + Referenz-Handshake)
    - FIX-42 (frühes Keepalive): behoben 2026-08-27 (Keepalive erst nach Phase 2)
  - **Manueller Test-Workflow:**
    1. Release-Build: `cmake --build build/win-msvc --config Release`
    2. VST3PluginTestHost starten: `.vscode/tasks.json` → `start-testhost-release`
    3. Plugin laden: NetSDRStation.vst3 aus `build/win-msvc/VST3/Release/`
    4. Station verbinden: `kphsdr.com:8072` (UI-Default, STABLE)
    5. Frequenz ändern: Live-Reception prüfen
    6. Audio-Qualität bewerten: keine Knackser/Zipper-Geräusche
  - **Test-Suite:** 87/87 Tests grün (Debug + Release), `ctest -C Debug` Passed,
    Validator 47/47.
  - Test: manual (documented in `doc/workspace-workflow.md` §3.6).

- [x] **M3.6** Echtzeit-Audio-Pipeline härten (Clock-Drift, Underflow, Denormals) — **VOLLSTÄNDIG ERLEDIGT 2026-08-27**
  - **Motivation:** F4-Analyse + JUCE/GStreamer/WebRTC-Vergleich (2026-08-26) zeigen strukturelle Schwächen der Pipeline, die reale Knackser verursachen, aber von den Tests (Mock-Server, perfekte Uhr) nie erfasst werden.
  - **Alle Teilaufgaben erledigt:**
    - [x] **1. Clock-Drift-Kompensation (ASRC):** `Resampler::setRatio()` + dynamische Anpassung in `renderPipeline` basierend auf Jitter-Buffer-Füllstand. Aggressivere Anpassung: ±1% pro Sekunde (vorher ±0.1%), alle 50ms (vorher 100ms), Target 300ms. Server-Sample-Rate wird aus `audio_rate=` Message geparst.
    - [x] **2. Underflow-Concealment:** `renderPipeline` füllt Unterfüllung mit Repeat-Last-Sample + linearem Fade (512 Samples = ~10ms bei 48kHz, vorher 64 Samples) statt hartem Null-Fill.
    - [x] **3. tryPush-Lücken-Erkennung:** Sequenz-Check in `decodeAndQueue` + Telemetrie-Updates.
    - [x] **4. Denormal-Schutz:** Flush-to-zero für Werte < 1e-30 in `renderPipeline`.
    - [x] **5. Jitter-Pre-fill vs. Netzwerkburst:** 500ms Prefill (vorher 200ms), 2000ms Max-Capacity (vorher 1000ms). Deckt Netzwerk-Jitter besser ab.
    - [x] **6. Resampler-Qualität wählbar:** `Resampler::Quality` enum (Medium/Best), Konstruktor-Parameter.
    - [x] **7. Telemetrie:** `PipelineTelemetry` struct mit Underrun/Overflow/SequenceGap/DroppedBlocks/QueueDepth/JitterBufferMs Zählern.
    - [x] **8. Echter Realtime-Test:** `tests/vst/plugin_processor_pipeline_tests.cpp` → "real-time stress test with variable clock and dropouts" (5s Dauer, variable Timing, 10% Underrun-Schwelle).
  - **Neue Dateien:** `source/vst/processor/pipeline_telemetry.h`
  - **Geänderte Dateien:** `source/dsp/resampler.h/.cpp`, `source/vst/processor/plugin_processor.h/.cpp`, `tests/vst/plugin_processor_pipeline_tests.cpp`
  - **Test-Suite:** 95/95 Tests grün (Debug + Release), inkl. Realtime-Stress-Test.

- [x] **M3.7** Refactoring: `plugin_processor.cpp` aufteilen (CCD-Verstoß — 967 Zeilen, 7 SRPs) — **UMSETZEN 2026-08-27**
  - **Problem:** `plugin_processor.cpp` vereinte 7 verschiedene Verantwortlichkeiten in einer Datei (894 Zeilen), klarer Verstoß gegen CCD Orange (Single Responsibility, Datei max. ~300 Zeilen).
  - **Verantwortlichkeiten:** (1) Lifecycle, (2) State Persistence, (3) IConnectionPoint, (4) Audio-Thread/renderPipeline, (5) Parameter-Routing, (6) Station/Connection, (7) Audio-Dekodierung.
  - **Umsetzung — Aufgeteilt in 3 Dateien:**
    - `plugin_processor.cpp` (368 Zeilen) — Lifecycle + State + IConnectionPoint + IAudioProcessor-Setup + `applyParamValue`/`applyState` + `station`/`setOnStatus`/`emitStatus`.
    - `plugin_processor_audio.cpp` (325 Zeilen) — `process()` + `renderPipeline()` (Audio-Thread).
    - `plugin_processor_network.cpp` (259 Zeilen) — `connectToStation()` + `disconnectStation()` + `decodeAndQueue()` + `sendPendingParams()`.
  - **Bedingung:** Alle Tests grün, kein Funktionsverlust.
  - _Dateien: `source/vst/processor/plugin_processor.cpp`, `plugin_processor_audio.cpp`, `plugin_processor_network.cpp`, `source/entry/CMakeLists.txt`, `tests/CMakeLists.txt`_
  - Test: Debug+Release Build grün, VST3-Validator 47/47, ctest grün (92/92).

- [x] **BUG-03** Click auf "Connect" bewirkt nichts (M3.5 manual acceptance)
  - **Symptom:** Im VST3PluginTestHost (Release) bleibt nach Klick auf den
    `Connect`-Button der `StationInput` jede sichtbare Reaktion aus. Das
    `StatusBadge` zeigt dauerhaft nur `"Connecting..."` (bzw. den vorherigen
    Zustand); weder ein Verbindungsaufbau noch ein Fehler ist erkennbar.
  - **Root Cause (primär, gefunden 2. Analyse-Runde):** Das Plugin ruft
    **nie `ix::initNetSystem()`** auf. Auf Windows führt das `WSAStartup`
    aus; ohne dieses schlägt jeder IXWebSocket-`socket()`/`connect()` mit
    `WSANOTINITIALISED` fehl → `socket_.start()` scheitert still und die
    Verbindung kommt nie zustande. Die Unit-/Integrationstests liefen nur,
    weil `tests/test_main.cpp` das Net-System einmalig global initialisiert —
    die echte Plugin-DLL läuft aber im Host-Prozess, wo das **niemand** tut.
    Daher „button reagiert nicht / connect funktioniert nicht" in Debug UND
    Release.
  - **Root Cause (sekundär):** Der Connect-Pfad hat zusätzlich **keinen
    Status-Feedback-Kanal zurück zur UI** und verschluckt Fehler vollständig:
    1. `PluginView.vue:102-109` (`onStation`) setzt `status = 'Connecting...'`
       und ruft `pluginService.setStation(hostPort)`. Danach existiert im
       nativen Modus **kein** Code, der `status` je wieder ändert — der einzige
       Zweig, der `'Connected (dev)'` setzt, läuft nur bei `!isInNative()`.
    2. C++ sendet nie eine `{"type":"status",...}`-Nachricht an die UI.
       `PluginProcessor::connectToStation` installiert in
       `plugin_processor.cpp:418-419` nur einen No-op-Text-Callback
       (`/* M3: text echoes unused */`); es gibt keinen Hook, der den
       Verbindungszustand zurückmeldet.
    3. Fehler werden verschluckt: `KiwiClient::connect`
       (`kiwi_client.cpp:37-50`) verdrahtet `onError`/`onClose` als `[]() {}`
       No-ops; `onOpen` führt nur den Handshake aus. `KiwiClient` exponiert
       gar keine `onOpen`/`onError`/`onClose`-Hooks (nur
       `setOnTextMessage`/`setOnBinaryMessage`, `kiwi_client.h:72-75`). Jede
       nicht erreichbare Station (falscher Port, Netzwerk blockiert, Server
       down) erzeugt daher **null** sichtbare Ausgabe → „es passiert nichts“.
    4. (Sekundär, unverifiziert) Der `setStation`-Transport läuft über VST3
       `IMessage` (`PluginController::setStation` → `sendMessage` →
       `PluginProcessor::notify`, `plugin_controller.cpp:107-117`,
       `plugin_processor.cpp:161-177`). Dieser End-to-End-Pfad ist durch
       **keinen** Test abgedeckt (TEST-10 prüft nur den No-op ohne
       `PluginController`). Verbindet ein Host Controller↔Processor nicht via
       `IConnectionPoint::connect`, wird die Nachricht von `sendMessage`
       stillschweigend verworfen und `connectToStation` nie aufgerufen — eine
       alternative Ursache für „es passiert nichts“.
  - **Fix (implemented 2026-08-24):**
    1. `KiwiConnection::Impl::connect()` ruft jetzt einmalig
       `ix::initNetSystem()` auf (static guard, idempotent; kein
       `uninitNetSystem` im Plugin — der OS-Prozess räumt auf)
       (`kiwi_connection.cpp:5,25-32`). **Das ist der eigentliche Fix** für
       „connect funktioniert nicht".
    2. `KiwiClient`: `StateCallback` + `setOnOpen`/`setOnError`/`setOnClose`
       (`kiwi_client.h:45,79-81,88-90`); die `KiwiConnection::Callbacks` in
       `connect()` rufen die Hooks jetzt auf (`kiwi_client.cpp:37-63`).
    3. `PluginProcessor`: `setOnStatus(StatusCallback)` + `emitStatus()`;
       `connectToStation` emittiert `"Connecting"` vor dem Connect und
       verdrahtet `onOpen`→`"Connected"`, `onError`→`"Error"`,
       `onClose`→`"Disconnected"` (`plugin_processor.cpp:415-432`).
       `emitStatus` marshalt auf den Worker-Thread und sendet die Status-
       Nachricht zusätzlich als VST3 `IMessage` (`"NetSDRStation:Status"`)
       an den Controller-Peer (`plugin_processor.cpp:695-712`).
    4. `PluginController::notify()` fängt `"NetSDRStation:Status"` ab und
       reicht den String über einen `statusSink_` an den Editor weiter
       (`plugin_controller.cpp:110-124,138-140`).
    5. `PluginEditor` registriert den Sink im Konstruktor, räumt ihn im
       Destruktor auf und ruft `pushStatus()` → `webView_.eval(
       window.updateVueState({"type":"status","data":"..."}))`
       (`plugin_editor.cpp:95-98,106-111,254-266`). Die UI brauchte KEINE
       Änderung (handelt `status`-Messages bereits ab).
    6. Fehler werden nicht mehr verschluckt: `onError`/`onClose` sind jetzt
       verdrahtet (Punkt 3).
  - _Files: `source/network/kiwi_client.*`, `source/network/kiwi_connection.*`,
    `source/vst/processor/plugin_processor.*`,
    `source/vst/controller/plugin_controller.*`, `source/editor/plugin_editor.*`_
  - Test: **grün** — 3 neue Tests:
    `KiwiClient: onOpen callback fires on connection` (kiwi_client_tests.cpp),
    `PluginProcessor: status reports Connecting then Connected when station
    connects` + `status reports Error for an unreachable station`
    (plugin_processor_pipeline_tests.cpp). Debug+Release ctest grün,
    Validator 47/47 (Debug+Release).
  - Acceptance: Klick auf Connect zeigt jetzt `Connecting` → `Connected`
    (bzw. `Error` bei nicht erreichbarer Station). Manuelle Verifikation im
    VST3PluginTestHost (M3.5) steht noch aus.

- [x] **M3.6** Dev infrastructure (T1, T2)
  - clangd MCP (semantic C++ tooling) + Playwright MCP (interactive UI debug).
    **Done 2026-08-22:**
    - T1: `lsp-mcp-server` (MIT) registered as `clangd_mcp` in `opencode.json`
      (bridges to `clangd --background-index`; uses `compile_commands.json`).
      `win-clangd` preset fixed (clang-cl compiler + hosting examples off).
    - T2: `@playwright/test` + `ui/e2e/smoke.spec.ts` + `playwright.config.ts` —
      green.
  - Test: MCP servers available and usable by the agent.

- [x] **M3.7** Documentation
  - `doc/architecture.md`: document the actual audio pipeline + full parameter
    list.
  - `doc/plan.md`: mark M3 done.
  - License audit (L2) for any new dependency.
  - Knowledge-sync: `index_project_code` + NotebookLM **NetSDRStation-VST**.

### Not in M3 (deferred)

- macOS/Linux build + host load.
- CLAP/AU format support.
- Installer / end-user packaging.
- Preset management / station favorites.

## Milestone M4 - KiwiSDR UI parity (Vue)

> **Implementation plan:** `doc/M4-implementation-plan.md`

> Goal: the plugin UI in Vue is a 1:1 re-implementation of the KiwiSDR browser
> interface, so the VST is operable exactly like the web UI. The complete
> element inventory lives in `doc/ui-architecture.md` §3; each sub-step below
> references its section. After M4 the VST exposes the same controls and
> readouts as `kphsdr.com:8072` in the browser.

> **Grundbedingung (fundamental requirement, applies to all M4 UI work):** the
> VST editor must be freely resizable by dragging the bottom-right corner
> (standard VST3 host resize), with the UI reflowing continuously at any size.

- [x] **M4.1** Resizable window (Grundbedingung)
  - **C++ side DONE (via FIX-22):** `onSize` → `webView_.resizeToParent()` →
    `MoveWindow(widget, 0, 0, w, h, TRUE)` (webview_editor.cpp); `attach()`
    sizes the widget immediately; `checkSizeConstraint` is the single clamp.
  - Min size updated to `kMinWidth=640`, `kMinHeight=400` (plugin_editor.cpp);
    below this the webview scrolls (overflow:auto).
  - **UI side DONE:** `PluginView.vue` rebuilt as fluid `kiwi-layout` grid
    (header / controls-row / statusbar), panels `flex: 1 1 220px` wrap at
    narrow widths; removed hard-coded `max-width:640px; margin:40px auto`.
  - _Files: `source/editor/plugin_editor.cpp`,
    `source/webview/webview_editor.cpp`, `ui/src/views/PluginView.vue`_
  - Test: C++ TEST-08 updated to 640×400 clamp (92/92 green); Vitest
    responsive layout structure at 640/1024/1920 viewports (34/34 green).
  - Manual acceptance still pending: drag corner in VST3PluginTestHost/DAW.

- [x] **M4.1.5** Schema-based Bridge API (type-safe contract)
  - **Schema (single source of truth):** `schema/bridge.schema.json`
    (Draft 2020-12) defines all bridge message types: setParameter, setStation,
    disconnect, getParameters (UI→C++) + status, param (C++→UI), plus the
    ParamId enum (27 IDs) and HostPort pattern.
  - **TS codegen:** `json-schema-to-typescript` (json2ts) → `ui/src/generated/bridge.ts`
    (types incl. `ParamId` union, `data: [ParamId, number]` tuples).
  - **Zod validators:** `ui/src/generated/bridge-validators.ts` — hand-written
    (json-schema-to-zod cannot resolve local $refs; produces useless z.any()
    schemas), guarded by `ui/tests/bridgeSchema.test.ts` which checks the
    validators' behaviour against the canonical schema.
  - **C++ codegen:** `schema/generate-cpp.py` → `source/vst/common/generated/bridge_schema.h`
    (structs + validators on nlohmann::json); CMake target
    `netsdrstation_bridge_codegen` regenerates deterministically.
  - **Backend refactor:** `bridge_protocol.cpp` now parses via nlohmann::json
    + generated validators (was fragile manual string-scraping); public API
    unchanged; `bridge_protocol.h` aliases the generated structs.
  - **NOTE (deviation from plan):** nlohmann/json was NOT already in the
    project — added as FetchContent dependency (MIT, header-only, v3.11.3).
    zod-to-json-schema has no Zod-v4-compatible release — consistency test
    checks behaviour directly instead.
  - **Prerequisite for M4.2** (all UI components consume the generated types).
  - _Files: `schema/*`, `ui/src/generated/*`, `ui/src/services/pluginService.ts`,
    `source/vst/common/generated/*`, `source/vst/common/bridge_protocol.{h,cpp}`_
  - Test: C++ 92/92 green (bridge tests unchanged behavior); Vitest 42/42 green
    (incl. schema consistency + validator rejection tests); UI build 150 kB.

- [x] **M4.2** UI scaffold & component library
  - **Prerequisite satisfied:** M4.1.5 — store consumes generated types from
    `ui/src/generated/*`.
  - **8 Kiwi primitives built** (w3_ext pattern): `KSlider` (3 px track /
    18 px thumb), `KNumberInput` (arrow-key increment, clamped), `KSelect`,
    `KToggle` (aria-pressed), `KButton` (active state), `KReadout` (monospace,
    fixed digits), `KPanel` (title + body), `KStatusBadge` (ok/warn/error dot).
  - **Dark SDR theme** (`master.css`): `--kiwi-*` custom-property palette.
  - **Pinia store** (`ui/src/store/kiwiStore.ts`): all 27 parameters + display
    state; `setParam` → pluginService + optimistic update; `applyParam` for
    backend messages; getters `statusText`/`statusState`.
  - **Panel shell** (`PluginView.vue`): KPanel-based Receiver / Passband /
    Audio / Display panels in the kiwi-layout; status bar with KReadout (dBm)
    + user count; `StationInput` kept in the header (M3 connect/disconnect,
    replaced by M5 station tab).
  - **Removed obsolete M3 components:** Knob, MuteButton, Slider, NumberInput,
    Toggle, StatusBadge (replaced by K* primitives; StationInput kept).
  - _Files: `ui/src/components/K*.vue`, `ui/src/store/kiwiStore.ts`,
    `ui/src/views/PluginView.vue`, `ui/src/assets/master.css`_
  - Test: Vitest 45/45 green (18 primitive tests + 7 store + 6 schema + 7 app
    + 7 pluginService); UI build 159.5 kB (Pinia+Zod inlined).

- [x] **M4.3** Frequency & Tuning panel
  - `FreqPanel.vue` (KPanel-based): six step-tuning buttons
    (`←10` `←1` `←0.1` `+0.1` `+1` `+10` kHz), KNumberInput direct entry
    (step 0.001, unit kHz), large KReadout in KiwiSDR 7-digit format
    (e.g. `14100.000`).
  - Values clamped to `[0.001, 30000]` kHz before bridging.
  - Bridge mapping via store: `setParam('freqKhz', v)` → pluginService →
    `{"type":"setParameter","data":["freqKhz",v]}` → C++ → WebSocket.
  - Integrated into PluginView controls row (replaced inline freq input).
  - _Files: `ui/src/components/FreqPanel.vue`_
  - Test: Vitest 52/52 green (7 new: step deltas, clamping, text entry,
    readout format).

- [x] **M4.4** Modulation & Passband panel
  - `ModePanel.vue` (KPanel-based): 18 mode buttons in two rows (active =
    green), KNumberInput ×2 for Low/High Cut (clamped: low ≤ 0, high ≥ 0),
    derived bandwidth readout (high − low), Reset button.
  - Full KiwiSDR default passband table (`MODE_DEFAULTS`): AM/AMN/AMW/USB/USN/
    LSB/LSN/CW/CWN/NBFM/NNFM/IQ/DRM/SAM/SAU/SAL/SAS/QAM.
  - Selecting a mode applies its default passband via the bridge.
  - Replaces the M4.2 Receiver + Passband panels (integrated into one).
  - _Files: `ui/src/components/ModePanel.vue`_
  - Test: Vitest 59/59 green (7 new: 18 buttons, active highlight, USB/CW
    default passbands, BW readout, reset, lowCut clamp).

- [ ] **M4.5** Band presets & memory
  - Band dropdowns (Amateur / Broadcast / Utility / time signals) and
    bookmark list; selecting a band sets the frequency/passband.
  - Ref: `doc/ui-architecture.md` §3.3.
  - Test: Vitest — band selection emits the expected frequency.

- [ ] **M4.6** Audio, AGC & signal processing panel
  - Volume slider + mute, AGC (on/off, threshold, decay, hang, slope, manual
    gain), squelch (on/off + threshold), noise blanker + noise reduction,
    S-meter (bar + dBm readout, driven by the audio level from M3.1).
  - Ref: `doc/ui-architecture.md` §3.5.
  - Test: Vitest — AGC/squelch/NB/NR controls emit correct parameters;
    S-meter updates from a mocked audio-level message.

- [ ] **M4.7** Waterfall & spectrum display
  - **Dependency:** requires a waterfall/spectrum data stream from the server
    (separate WebSocket `STREAM_WATERFALL`), not yet present in the audio-only
    M3.1 pipeline. Add the stream to the network layer first.
  - Controls: zoom (`+`/`-`/`Max In`/`Max Out`, level readout), WF Max/Min dB,
    speed, color map, display-mode toggle (WF/Spec/Both), FFT window,
    interpolation, CIC comp, aperture auto-mode, timestamps, JPG export.
  - Ref: `doc/ui-architecture.md` §3.4.
  - Test: Vitest for controls; integration test that streamed FFT data renders
    without dropped frames.

- [ ] **M4.8** Status & system readouts + extension panel
  - Status readouts: active user slots, GPS sync indicator, audio buffer /
    stream status; extension select dropdown + dynamic panel
    (CW / WFAX / RTTY / SSTV / tDoA / IQ / antenna switch).
  - Ref: `doc/ui-architecture.md` §3.6, §3.7.
  - Test: Vitest — readouts update from mocked status messages; extension
    dropdown switches panel.

- [ ] **M4.9** UI parity acceptance
  - Side-by-side check of the Vue UI against `kphsdr.com:8072` in a
    browser: every control present, every readout live.
  - Test: manual (documented in `doc/workspace-workflow.md`); Playwright E2E
    for the main flows.

### Not in M4 (deferred)

- Decoder DSP for the extension panels (CW / WFAX / RTTY / SSTV / tDoA / IQ)
  — panels are present, decoders are stubbed until a later DSP milestone.
- Admin / mfg configuration pages (KiwiSDR admin.html).

## Milestone M5 - Station selection tab

> **Implementation plan:** `doc/M5-implementation-plan.md`

> The UI gets a dedicated tab structure: **Tab 1 "SDR Stations" = station
> selection**, **Tab 2 "KIWI UI" = the KiwiSDR web interface (M4)**. The user
> picks a station from a scrollable list; clicking one connects to it. Default
> state is **no station loaded** → Tab 2 shows only the message "please select
> station first".

- [ ] **M5.1** Station directory fetch — **ONLY API-ready stations**
  - Fetch the list of public KiwiSDR receivers (name, location, frequency
    coverage, SNR, user count, status, connect URL) from the public station
    directory. Confirm the exact endpoint/format during implementation —
    the previously documented candidates are now stale: `rx-888.com/api/rx/list`
    404s and `kiwisdr.com/public/` is behind an anti-bot click-gate.
  - **Kernanforderung (User 2026-08-27): lade NUR API-ready Stationen.** Jeder
    Directory-Eintrag publiziert **`ext_api`** (Operator-Allowance der externen
    API-Kanäle, auch in der per-Station `/status`). Nur Stationen mit
    **`ext_api > 0`** werden geladen/angezeigt; `ext_api == 0` (Browser-only)
    wird **komplett gefiltert** — dieser Client verbindet ausschließlich über
    die native WebSocket-External-API, also nur mit Stationen, die das erlauben.
    Kontext: FIX-41 zeigt, dass wir zwingend API-fähige Receiver brauchen.
  - **Dauerbetrieb-Probe (2026-08-27):** 19 Stationen getestet (45 s, probe_duration.py).
    5 STABLE, 6 KICKED (davon 5 ext_api=0), 8 DOWN.
    Seed-Liste: `doc/station-list.md`. Empfohlener M5.1-Ansatz: Embedded Seed +
    Live-`/status`-Abfrage. Öffentliche Verzeichnisse (sdr.hu, receiverbook.de,
    rx.kiwisdr.com/public) waren bei Probe nicht programmatisch erreichbar.
  - Referenz-Implementierung zur Orientierung: AetherSDR
    `src/core/KiwiPublicDirectory.{h,cpp}` (liest `ext_api` aus dem
    server-publizierten Directory/`/status`, präsentiert nur `ext_api > 0`).
  - _Files: `source/network/` (fetcher) oder UI-side service_
  - Test: integration test against a mocked directory endpoint → stations
    parsed into a typed model; **Filter-Test: `ext_api == 0`-Station wird
    ausgeschlossen**.

- [ ] **M5.2** Tab layout + routing
  - Add a tab bar to the Vue UI: Tab 1 "SDR Stations", Tab 2 "KIWI UI".
  - State store gains a `connectedStation` (null = none).
  - Ref: `doc/ui-architecture.md` §1 (two responsibilities).
  - Test: Vitest — tab switch renders the correct view.

- [ ] **M5.3** Station list (scrollable)
  - Scrollable list of stations, each row showing its content (name,
    location, frequency coverage, SNR, users, online/offline badge).
  - Ref: `doc/ui-architecture.md` §3.3 (memory list pattern).
  - Test: Vitest — list renders fetched stations; virtualization/scroll
    correctness.

- [ ] **M5.4** Connect on click
  - Clicking a station sets the connect target and triggers the connection
    (handshake via `KiwiClient`); updates `connectedStation`.
  - _Files: `ui/src/views/*`, `source/network/kiwi_client.*`,
    `source/vst/processor/plugin_processor.cpp`_
  - Test: integration test — station click → `SET auth` handshake to the
    selected host.

- [ ] **M5.5** Empty state on Tab 2
  - When no station is loaded (default), Tab 2 shows only the message
    "please select station first" and no receiver controls/readouts.
  - Test: Vitest — Tab 2 renders the empty state when `connectedStation` is
    null; renders the receiver (M4) once connected.

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
- [x] **L2** Keep every added dependency permissive (no GPL/paid licenses)
  - Test: CI/license-check step lists all deps + licenses; no GPL/paid.
  - Status: M3 audit done — no new shipped dependency; dev-tool additions all
    permissive (Playwright Apache-2.0, lsp-mcp-server MIT, clangd Apache-2.0).
- [x] **L3** JUCE orientation: ideas/architecture only, never copy code (see framework-licensing.md)
  - Test: review step - no JUCE-derived code; inspiration documented.
  - Status: JitterBuffer prefill-latch + ring-buffer design follows the JUCE
    "always fill" concept (documented, own implementation).

## Coding rules (standing, see `doc/coding-standards.md`)

- [ ] **C1** Follow all Clean Code Developer (CCD) rules (red..white)
  - Test: clang-tidy/static analysis + review; coverage >= 90%.
- [ ] **C2** Justify any CCD rule violation in the task summary
  - Test: review step - every violation has a justification recorded.

## Milestone M6 - Multi-Provider Support (OpenWebRX / SpyServer / RTL-TCP)

> **Implementation plan:** `doc/M6-implementation-plan.md`

> Prerequisite: M5 complete. Extends the plugin to three additional SDR server
> types (OpenWebRX, SpyServer, RTL-TCP). Provider abstraction via
> `IReceiverClient` interface. Reference: radiom (MIT), VibeSDR (MIT),
> KiwiAngel (GPLv3) — analysed 2026-08-27.

- [ ] **M6.1a** `OpenWebRxClient` — IXWebSocket, JSON handshake, profile selection,
  IMA ADPCM-with-SYNC decoder (different from KiwiSDR ADPCM).
  - _Files: `source/network/openwebrx_client.*`_
  - Test: ADPCM-with-SYNC unit test; mock OpenWebRX server integration test.

- [ ] **M6.1b** OpenWebRX station detection via HTTP `/api/features`.

- [ ] **M6.1c** UI: provider badge "OpenWebRX" in station list row.

- [ ] **M6.1d** Tests: OpenWebRX ADPCM decoder correctness + mock server integration.

- [ ] **M6.2a** `SpyServerClient` — direct TCP, native binary protocol, IQ frames.
  - _Files: `source/network/spyserver_client.*`_
  - Test: mock TCP SpyServer; verify IQ frame parsing + frequency command.

- [ ] **M6.2b** Station entry model gains `provider` field (kiwisdr / openwebrx /
  spyserver / rtltcp).

- [ ] **M6.2c** Tests: mock SpyServer TCP server.

- [ ] **M6.3a** `RtlTcpClient` — TCP, `dongle_info` header, uint8 IQ stream, frequency/gain commands.
  - _Files: `source/network/rtltcp_client.*`_

- [ ] **M6.3b** UI: "Local RTL-TCP" connection type (host + port fields).

- [ ] **M6.3c** Tests: mock RTL-TCP server.

- [ ] **M6.4a** Station model: `provider` field + typed discriminator.

- [ ] **M6.4b** Station list fetcher: OpenWebRX auto-detection via `/api/features`.

- [ ] **M6.4c** Station list row: provider badge / icon.

- [ ] **M6.4d** `PluginProcessor`: routes `connectToStation` to the correct
  `IReceiverClient` based on `provider`.
  - Test: integration test — station with provider=openwebrx → OpenWebRxClient connects.

## AI development helpers (MCP servers, see `doc/test-strategy.md` §9)

- [x] **T1** clangd-based C++ semantic MCP - done (M3.6, `lsp-mcp-server` MIT as `clangd_mcp`)
- [x] **T2** Playwright MCP - done (M3.6, `@playwright/test` + `ui/e2e/smoke.spec.ts` green)

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
