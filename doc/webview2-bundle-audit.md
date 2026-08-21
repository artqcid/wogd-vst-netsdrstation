# WebView2-Bundle-Audit – Fehleranalyse M1.18–M1.22

_Erstellt: 2026-08-21 | Grund: Release-Build zeigt kein GUI_

---

## Zusammenfassung

Die Checklistenpunkte M1.18–M1.22 wurden als erledigt markiert, aber das
Release-Plugin zeigt kein GUI. Dieser Bericht dokumentiert vier konkrete Bugs
mit exakten Datei-/Zeilenangaben aus dem Quellcode.

---

## Bug 1 (Critical) – webview-Bibliothek ignoriert den gebündelten Fixed Runtime

### Befund

`webview/webview` 0.12.0 erstellt die WebView2-Umgebung intern in:

```
build/win-msvc/_deps/webview-src/core/include/webview/webview.h:4217-4218
  m_com_handler->set_attempt_handler([&] {
    return m_webview2_loader.create_environment_with_options(
        nullptr,          // browser_dir = nullptr !
        userDataFolder, nullptr, m_com_handler);
  });
```

`nullptr` als `browser_dir` löst in der Bibliothek `find_installed_client` aus
(webview.h:3428), die ausschließlich die Windows-Registry durchsucht:

```
webview.h:3460-3481
  client_info_t find_installed_client(…) const {
    auto root_key = system ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    reg_key key(root_key, sub_key, …);   // Registry-Suche
```

Der gebündelte `FixedRuntime/`-Ordner wird niemals benutzt.
`is_webview2_available()` (webview.h:4292) ruft ebenfalls `nullptr` auf – wirft
bei fehlendem System-WebView2 eine Exception – Plugin lädt, GUI bleibt leer.

### Warum der "Fix" in webview_editor.cpp nicht funktioniert

`source/webview/webview_editor.cpp:26-27` enthält nur den Kommentar:

```cpp
// Use the local WebView2 Fixed Version from the VST3 bundle
w_ = std::make_unique<webview::webview>(/*debug=*/true, parentHandle);
```

Es gibt keine Implementierung, die dem Konstruktor sagt, wo das Fixed Runtime
liegt. `webview::webview` 0.12.0 hat keinen Parameter dafür.

### Lösung (FIX-WV2-A)

`WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` als Umgebungsvariable setzen, bevor
`webview::webview` konstruiert wird. `WebView2Loader.dll` wertet diese Variable
nativ aus, bevor es die Registry benutzt.

```
Reihenfolge in Impl::attach():
1. Pfad des eigenen DLL-Moduls ermitteln (GetModuleFileNameW mit HMODULE)
2. FixedRuntime-Pfad = <moduledir>/FixedRuntime
3. _wputenv_s(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", fixedRuntimePath)
4. std::make_unique<webview::webview>(debug, parentHandle)
5. _wputenv_s(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", L"")  // zurücksetzen
```

Hinweis: `HMODULE` des Plugin-DLL muss beim Laden gespeichert werden (z. B. in
`DllMain` oder `VSTPluginMain`/`GetPluginFactory`).

---

## Bug 2 (Critical) – NS_UI_DIST_URL ist ein maschinenspezifischer absoluter Pfad

### Befund

`source/entry/CMakeLists.txt:43-49`:

```cmake
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/ui/dist/index.html" NS_UI_DIST_INDEX)
if(WIN32)
    set(NS_UI_DIST_URL "file:///${NS_UI_DIST_INDEX}")
endif()
target_compile_definitions(${target} PRIVATE NS_UI_DIST_URL="${NS_UI_DIST_URL}")
```

Auf der Entwicklermaschine wird eingebaut:

```
file:///C:/Users/marku/Documents/GitHub/artqcid/vst-nativ-projects/
        wogd-vst-netsdrstation/ui/dist/index.html
```

Dieser Pfad ist nicht transportabel: auf jedem anderen Rechner oder nach einem
Ordner-Umzug existiert die Datei nicht – WebView2 zeigt Fehlerseite.

Zusätzlich: `ui/dist/` wird nicht in das VST3-Bundle kopiert. Das Bundle enthält
nur `NetSDRStation.vst3`, `WebView2Loader.dll` und `FixedRuntime/`.

### Lösung (FIX-WV2-B1 + FIX-WV2-B2)

**B1:** `ui/dist/` per POST_BUILD in `Contents/x86_64-win/ui/` kopieren.

**B2:** `kUiUrl` in `source/editor/plugin_editor.cpp:29` zur Laufzeit aus dem
DLL-Pfad ableiten:

```cpp
#ifndef NDEBUG
  constexpr const char* kUiUrl = "http://localhost:5173";
#else
  // Laufzeit: <moduledir>/ui/index.html
  static const std::string kUiUrl = buildReleaseUiUrl();
#endif
```

---

## Bug 3 (Important) – POST_BUILD-Pfad hardcodet "Release"

### Befund

`source/entry/CMakeLists.txt:86`:

```cmake
set(PLUGIN_DIR
  "${CMAKE_BINARY_DIR}/VST3/Release/NetSDRStation.vst3/Contents/x86_64-win")
```

Bei Multi-Config-Generatoren (MSVC) ist die Konfiguration `$<CONFIG>` erst zur
Build-Zeit bekannt. Das Hardcoding auf `Release` bedeutet: POST_BUILD kopiert
bei Debug-Builds in das falsche Verzeichnis.

### Lösung (FIX-WV2-C)

```cmake
set(PLUGIN_DIR
  "${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/NetSDRStation.vst3/Contents/x86_64-win")
```

---

## Bug 4 (Important) – Bundle-Pfad hardcodiert statt SMTG-Property

### Befund

`smtg_add_vst3plugin` legt den Bundle-Pfad im Target-Property
`SMTG_PLUGIN_PACKAGE_PATH` ab. Dieses Property zu verwenden ist robuster als
`${CMAKE_BINARY_DIR}/VST3/…` zu hardcodieren.

### Lösung (FIX-WV2-D)

```cmake
get_target_property(_BUNDLE_DIR netsdrstation SMTG_PLUGIN_PACKAGE_PATH)
set(PLUGIN_DIR "${_BUNDLE_DIR}/Contents/x86_64-win")
```

---

## Was korrekt implementiert ist

| Punkt | Status | Bemerkung |
|-------|--------|-----------|
| `WebView2Loader.dll` kopiert | OK | POST_BUILD kopiert nach `x86_64-win/` |
| `FixedRuntime/` kopiert | OK | Enthält `EBWebView/x64/EmbeddedBrowserWebView.dll` |
| FIX-22 `resizeToParent()` | OK | `MoveWindow(widget, …)` korrekt, wird in `attachWebView()` + `onSize()` aufgerufen |
| Bridge-Protokoll | OK | FIX-02/FIX-03 korrekt gelöst (`bridge_protocol.cpp`) |
| `webview_editor.cpp` Pimpl | OK | FIX-16 korrekt – `std::unique_ptr<Impl>` |

---

## Offene Fixes

| ID | Datei | Priorität |
|----|-------|-----------|
| FIX-WV2-A | `source/webview/webview_editor.cpp:18-31` | **Critical** |
| FIX-WV2-B1 | `source/entry/CMakeLists.txt` (POST_BUILD ui/dist) | **Critical** |
| FIX-WV2-B2 | `source/editor/plugin_editor.cpp:24-33` | **Critical** |
| FIX-WV2-C | `source/entry/CMakeLists.txt:86` | Important |
| FIX-WV2-D | `source/entry/CMakeLists.txt:86-106` | Important |

---

_Verfasst von: Architect Agent | Verifiziert gegen: Quellcode-Stand 2026-08-21_
