# NetSDRStation-VST - Checklist

_Open tasks only (short descriptions). Detailed info: `doc/architecture.md`;
draft plan: `doc/plan.md`; workflow: `doc/workspace-workflow.md`;
coding rules: `doc/coding-standards.md`; test strategy: `doc/test-strategy.md`._

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

- [ ] **FIX-35** `ParameterRegistry` – O(n) linear scan for every parameter lookup (note for M2)
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

- [ ] **M3.1** Processor integration: network audio pipeline
  - Replace `SineOscillator::render()` in `PluginProcessor::process()` with the
    pipeline `KiwiClient → ImaAdpcmDecoder → AudioSampleQueue → Resampler →
    JitterBuffer → process()`.
  - Link `netsdr_network` into the plugin target (`source/entry/CMakeLists.txt`).
  - Start WebSocket on plugin `initialize()`; disconnect on `terminate()`.
  - _Files: `source/vst/processor/plugin_processor.cpp`,
    `source/entry/CMakeLists.txt`_
  - Test: integration test against a mock KiwiSDR server → decode → resample →
    Goertzel peak at the expected frequency in the output.

- [ ] **M3.2** Full KiwiSDR parameter model in the registry
  - Add ALL KiwiSDR receiver/audio/display parameters as VST3 parameters so
    every setting is DAW-automatable. GUI-visible subset marked below.
  - Core (GUI): `station` (host:port), `mode`, `freqKhz`, `lowCut`, `highCut`.
  - AGC (GUI: `agcOn` only; rest default): `agcOn`, `agcHang`, `agcThresh`,
    `agcSlope`, `agcDecay`, `agcManGain`.
  - Audio (default only): `squelchOn` + `squelchThreshold`, `nbOn` +
    `nbThreshold`, `nrOn`, `deempOn`, `compOn`, `volume`.
  - Display/Waterfall (GUI later): `wfOn`, `wfSpeed`, `wfZoom`, `wfMaxDb`,
    `wfMinDb`, `wfComp`, `arOn`, `ovOn`.
  - Resolve `FIX-35` (ParameterRegistry O(1) id→index lookup) as part of this.
  - _Files: `source/vst/common/paramdefinitions.h`, `paramids.h`,
    `parameter_registry.*`, `source/network/kiwi_commands.*`,
    `source/network/kiwi_client.*`_
  - Test: unit test all params register with correct range/default/ID; command
    serializers emit the full `SET` frame for each group.

- [ ] **M3.3** UI: KiwiSDR controls
  - Replace M1 knobs with Kiwi controls: station, mode, frequency (kHz),
    bandwidth low/high (Core); AGC on/off (rest default); waterfall on/off
    (display params default).
  - Verify bidirectional bridge (UI → KiwiBridge → `SET`; status echo → UI).
  - _Files: `ui/src/views/PluginView.vue`, `ui/src/services/pluginService.ts`,
    `source/network/kiwi_bridge.cpp`_
  - Test: Vitest component tests (controls emit correct values); bridge
    roundtrip integration test.

- [ ] **M3.4** Real-time safety audit + performance
  - clang-tidy: audio thread lock-free, no heap allocation.
  - Tune jitter buffer (100–150 ms) under real network conditions.
  - Validate resampler quality/CPU at small buffer sizes.
  - Test: clang-tidy clean; no dropouts in manual listening test.

- [ ] **M3.5** Manual acceptance (M2.10 real)
  - Load plugin in VST3PluginTestHost against real KiwiSDR
    (`g8ure.ddns.net:8078`); change frequency → live reception audible in DAW;
    no zipper noise / dropouts.
  - Test: manual (documented in `doc/workspace-workflow.md` §3.6).

- [ ] **M3.6** Dev infrastructure (T1, T2)
  - clangd MCP (semantic C++ tooling) + Playwright MCP (interactive UI debug).
  - Test: MCP servers available and usable by the agent.

- [ ] **M3.7** Documentation
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
> readouts as `g8ure.ddns.net:8078` in the browser.

> **Grundbedingung (fundamental requirement, applies to all M4 UI work):** the
> VST editor must be freely resizable by dragging the bottom-right corner
> (standard VST3 host resize), with the UI reflowing continuously at any size.

- [ ] **M4.1** Resizable window (Grundbedingung)
  - Editor window freely resizable via bottom-right corner drag (standard VST3
    host behaviour); only clamp is the documented `kMinWidth`/`kMinHeight`
    floor.
  - C++ side: forward host `onSize`/`WM_SIZE` to the webview widget so the
    WebView2 view fills the client area on every resize (extends FIX-22); keep
    `checkSizeConstraint` as the single clamp.
  - UI side: fully responsive Vue layout (fluid grid/flex), no hard-coded
    pixel dimensions; all panels reflow continuously.
  - _Files: `source/editor/plugin_editor.cpp`,
    `source/webview/webview_editor.cpp`, `ui/src/**`_
  - Test: manual — drag corner in VST3PluginTestHost (and a DAW), UI reflows
    at any size without clipping; Vitest — responsive layout at several
    viewport sizes.

- [ ] **M4.2** UI scaffold & component library
  - Vue component primitives matching the KiwiSDR w3 widgets: slider, number
    field, select/dropdown, checkbox/toggle, button, readout, color picker.
  - Dark SDR theme + panel/layout shell (header, control panels, status bar).
  - Central state store bound to the existing bridge
    (`ui/src/services/pluginService.ts`), bidirectionally synced with the C++
    side (setParameter → message; onMessage → state).
  - _Files: `ui/src/components/*`, `ui/src/services/pluginService.ts`,
    `ui/src/views/PluginView.vue`_
  - Ref: `doc/ui-architecture.md` §3 (overview), w3 widget library.
  - Test: Vitest for each primitive; bridge roundtrip integration test.

- [ ] **M4.3** Frequency & Tuning panel
  - Frequency input field (kHz, unit-aware), step-tuning buttons
    (`-10`/`-1`/`-0.1`/`+0.1`/`+1`/`+10` kHz), large frequency readout,
    passband dragger overlay on the waterfall scale.
  - Ref: `doc/ui-architecture.md` §3.1.
  - Test: Vitest — buttons/input emit correct `freqKhz` values.

- [ ] **M4.4** Modulation & Passband panel
  - Mode selector with all 18 modes (`AM`…`QAM`), Low Cut / High Cut /
    Bandwidth fields, filter-reset button.
  - Ref: `doc/ui-architecture.md` §3.2 (full 18-mode list).
  - Test: Vitest — mode enum + passband values map to correct parameters.

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
  - Side-by-side check of the Vue UI against `g8ure.ddns.net:8078` in a
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

- [ ] **M5.1** Station directory fetch
  - Fetch the list of public KiwiSDR receivers (name, location, frequency
    coverage, SNR, user count, status, connect URL) from the public station
    directory. Confirm the exact endpoint/format during implementation
    (KiwiSDR public list, e.g. `kiwisdr.com/public/`).
  - _Files: `source/network/` (fetcher) or UI-side service_
  - Test: integration test against a mocked directory endpoint → stations
    parsed into a typed model.

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
