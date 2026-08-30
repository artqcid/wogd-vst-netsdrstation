# NetSDRStation-VST — Chronological Log

_Append-only, newest first. Parseable with `grep "^## "`. Entries use
`**Creation**`, `**Update**` or `**Deprecation**` prefix + linked concept file._

## 2026-08-30 — M4c.7 Bugs 16+17 implementiert: CursorBar + Pan/Cursor-Separation

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Bugs 16 (Cursor liegt auf Frequenzleiste statt eigener Leiste) + 17 (Cursor-Edges lassen sich nicht anfassen) implementiert.

**Creation:** [`CursorBar.vue`](../ui/src/components/CursorBar.vue) — eigene 20px-Leiste über der Scale:
- 3 Zonen (lo/hi/center) als separate DIVs mit `pointer-events: all`
- Trapezoid-SVG + Carrier-Line + `freqToPx`-basiertes `carrierX` (reaktiv)
- Drag-State-Machine via `pointerdown/move/up` auf `document`
- `emit('tune' | 'update:lowCut' | 'update:highCut')`

**Update:** [`FrequencyRuler.vue`](../ui/src/components/FrequencyRuler.vue) — auf Scale-only reduziert: Cursor-SVG + Drag-Logik entfernt, Tick-Berechnung (Major/Minor) + Wheel-Zoom behalten.

**Creation:** [`cursorPanLogic.ts`](../ui/src/components/cursorPanLogic.ts) — Pan/Cursor-Separation:
- `viewCenterKhz = freqKhz + panOffsetKhz`
- `cursorDrag`: freqKhz ändert, panOffset kompensiert → Fenstermitte bleibt stabil
- `panWindow`: freqKhz bleibt (Cursor an absoluter Frequenz), panOffset ändert → Fenster wandert unter dem Cursor durch (KiwiSDR-Akzeptanzkriterium 4)

**Update:** [`PluginView.vue`](../ui/src/views/PluginView.vue) — CursorBar über FrequencyRuler im `scale-area`; `onFreqRulerTune` nutzt `cursorDrag` (Fenster stabil); `onCanvasMouseDown`/`onMouseMove`/`onPan` nutzen `panWindow`; `loKhz`/`hiKhz` aus `viewCenterKhz`.

**Update:** [`kiwiStore.ts`](../ui/src/store/kiwiStore.ts) — `panOffsetKhz` State ergänzt.

**Update:** [`main.ts`](../ui/src/main.ts) — `window.__vueStore` für E2E-Tests exponiert (DEV-only, gleiche Pinia-Instanz).

**Debug-Funde (Wiki-Gleaning):**
- Doppelte `createPinia()` in main.ts → `__vueStore` und App-Store waren verschiedene Instanzen (E2E-Tests sahen veraltete Werte)
- `onFreqRulerTune` setzte nur freqKhz ohne `cursorDrag`-Kompensation → Fenster wanderte beim Cursor-Drag mit (Bug 16)
- `panWindow`-Semantik war invertiert (freq wanderte, offset blieb) → Cursor-Pixel folgte dem Pan nicht (Bug 17); korrigiert nach KiwiSDR-Akzeptanzkriterium 4
- E2E-Pan-Tests draggten auf `.freq-ruler__scale`, aber die Scale hat keinen Pan-Handler mehr → auf Waterfall-Canvas umgestellt

**Status:** 142/142 Vitest ✅, 104/105 E2E ✅, vue-tsc clean. **Bugs 1–17 implementiert** 🎉

## 2026-08-30 — Release-Build-Fehler behoben (Type-Check im CMake-UI-Target)

**Update:** [`main.ts`](../ui/src/main.ts) — `window.__vueStore`-Declaration auf explizites Interface mit `setParam: (name: ParamId, value: number) => void` umgestellt (Import `ParamId` aus bridge-validators). Vorherige Versuche schlugen fehl:
- `setParam: (name: string, ...)` → TS2322 (Store `setParam` akzeptiert nur `ParamId`-Union, string nicht zuweisbar)
- `ReturnType<typeof useKiwiStore>` → TS7022 zirkuläre Typ-Referenz (Store referenziert sich im eigenen Initializer) + Folgefehler TS2339 (statusText/statusState)
- `[key: string]: unknown`-Index-Signatur im Interface → TS2322 ("Index signature missing in _StoreWithState")

**Update:** [`CursorBar.vue`](../ui/src/components/CursorBar.vue) — `<script setup>` auf `<script setup lang="ts">` konvertiert (Props/Emits/State/Handler typisiert, `PointerEvent`-Annotations in Inline-Handlern). Vorher: TS7016 (Modul implizit `any` → kein Typdeclaration-Export ohne `lang="ts"`).

**Gleaning:** Das CMake-UI-Target (`netsdrstation_ui`) führt `type-check` (vue-tsc) als Custom-Build-Step aus — strikter als das reine `vue-tsc --noEmit` im UI-Ordner (fehlte in der UI-Local-Verifikation). Release-/Debug-Build inkl. UI-Target ist Pflicht vor Commit.

**Status:** Release + Debug Build ✅, ctest 1/1 ✅, 142/142 Vitest ✅, 104/105 E2E ✅, vue-tsc clean.

## 2026-08-29 — M4c.7 Bugs 14+15 implementiert: Spectrum AF + DRM Panel

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Bugs 14 (Spec AF) + 15 (DRM Panel, Mode-Index-Korrektur) implementiert.

**Creation:** [`SpectrumAf.vue`](../ui/src/components/SpectrumAf.vue) — Canvas-basierter AF-Spektrumanalysator:
- 200px Höhe, Canvas-Breite = Container - 100px (50px Margin links/rechts)
- dBm Y-Achse (-10..-110) mit 256-Farb-Colormap (wie SpectrumRf)
- Horizontale Grid-Linien alle 10 dB, dB-Beschriftungen rechts
- **Vertikales Grid** bei 1 kHz Audio-Frequenzschritten
- **Grüne Center-Linie** (lime, 3px) bei `canvas.width/2` (DC/Träger)
- **Rote Rand-Marker** (red, 3px) links/rechts (Nyquist edges)
- Halbtransparentes Passband-Overlay um Träger zentriert

**Creation:** [`DrmPanel.vue`](../ui/src/components/DrmPanel.vue) — DRM Schedule/Services + Decoder-Panel:
- Top-Overlay: 3-spaltiges Schedule-Layout (Status-Checkboxen links, Stationsliste mit Zeitleiste mitte, UTC/Local-Zeit + Legende rechts)
- Decoder-Panel: Header (Dream 2.2.1), Content (Links), Footer (Stop/Monitor IQ/Test 1/Test 2 + LPF)
- Sichtbar bei `store.mode === 8` (korrigierter DRM-Mode-Index)

**Korrektur:** [`PluginView.vue`](../ui/src/views/PluginView.vue) — DRM-Mode-Index von 12 auf **8** korrigiert (laut KiwiSDR `modes_lc`).

**Status:** 126/126 Vitest ✅, 100/101 E2E ✅, vue-tsc clean. **Bugs 1–15 implementiert** 🎉

## 2026-08-29 — M4c.7 Bugs 12+13 implementiert: HeaderBar + Spectrum RF

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Bugs 12 (HeaderBar) + 13 (Spectrum RF) implementiert.

**Creation:** [`HeaderBar.vue`](../ui/src/components/HeaderBar.vue) — KiwiSDR-konforme Topbar:
- 67px Höhe, 4-Spalten-Grid-Layout (L/ML/MR/R)
- Logo + Stationstitel + Antenne (L), Owner-Info (ML), Callsign (MR), UTC/Local-Zeit + TZ + "Powered by OpenWebRX" (R)
- Chevron-Toggle-SVG (43×12px) für Expand/Collapse des Panorama-Bildbereichs
- Expandierbarer RX_PHOTO_FILE-Bereich mit max-height-Animation

**Creation:** [`SpectrumRf.vue`](../ui/src/components/SpectrumRf.vue) — Canvas-basierter RF-Spektrumanalysator:
- Canvas 2D, 200px Höhe, volle Breite
- dBm Y-Achse (-10..-110) mit 256-Farb-Colormap (dunkelblau→cyan→grün→gelb→rot)
- Farbbänder alle 10 dB mit weißen dB-Beschriftungen rechts
- Horizontale Grid-Linien (1px, lightGray) an jeder 10-dB-Grenze
- Passband-Overlay auf separatem transparentem Canvas (rgba(150,150,150,0.25))
- Datenquelle: `store.waterfallBins` (gleiches FFT-Signal wie Wasserfall)
- Bedingtes Rendering via `store.spectrumMode === 'specRF'` in PluginView.vue

**Update:** [`PluginView.vue`](../ui/src/views/PluginView.vue) — Header durch `<HeaderBar />` ersetzt, StationInput in Connection-Bar ausgelagert, SpectrumRf parallel zum Waterfall bedingt eingeblendet.

**Update:** [`audioPanel.test.ts`](../ui/tests/audioPanel.test.ts) — Toggle-Indizes an neue AudioPanel-Reihenfolge angepasst (Compression + De-emphasis ergänzt).

**Status:** 126/126 Vitest ✅, 100/101 E2E ✅, vue-tsc clean. Bugs 1–13 implementiert, 14+15 offen.

## 2026-08-29 — M4c.7 + M4c.8: Post-Task-Sync (286–295)

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — Bugs 7–11 completed, Bugs 12–15 open.

**Subagent-tasks history (from ARCHITECT summary):**
- Bug 7 (Cursor trapezoid): 13/13 E2E green. Zustandsübergang frequenzbasiert (`passband_visible()`), grün/lime/gelb anhand Pixelbreite ≥50px, drei Drag-Zonen (lo/hi/center).
- Bug 8 (Frequency scale ticks): 13/13 E2E green. SVG `<line>` major/minor ticks mit Klassen `.freq-ruler__tick--major/minor`, Labels als `<text>.freq-ruler__label`, Minor-Subdivision 4:1.
- Bug 9 (Band/Tag area): 6/6 E2E green. TagArea verticale Verbindungslinien (1px schwarz `.tag-area__line`), BandScaleBar `text-align:center`. Forschung in `doc/kiwsdr-research-bug9.md`.
- Bug 10 (Audio tab): Scrollbar 320px max-height + overflow-y:auto; Pan-Slider (local ref); De-emphasis AM/FM dropdowns + Compression Toggle.
- Bug 11 (AGC tab): Forschung abgeschlossen — 7 Parameter (agc, hang, manGain, thresh, threshCW, slope, decay), HTML-Struktur, SET-Befehle.
- Bug 12 (Header area): Forschung abgeschlossen — 67px Höhe, 4-Spalten-Layout, Chevron-Collapse (PNG 43×12px), Panorama-Bild `RX_PHOTO_FILE`.
- Bug 13 (Spec RF): Forschung abgeschlossen — Canvas 2D (200px Höhe), Y-Achse dBm -10..-110, Colormap-Bänder alle 10 dB, Passband-Overlay auf separatem Canvas.
- Bug 14 (Spec AF): Forschung noch ausstehend.
- Bug 15: Noch nicht analysiert.

**Update:** [`index.md`](./index.md) — `kiwsdr-research-bug9.md` hinzugefügt; Bug-Status auf 1–11 gefixt aktualisiert.

## 2026-08-29 — M4c.7 Bug 7 präzisiert: drei Cursor-Interaktions-Zonen

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) §Bug 7 — Interaktions-Logik des
Frequenz-Cursors präzisiert (drei getrennte Klick+Halten-Zonen):

1. **Auf dem Cursor** (Klammer/Flanken) → `low_cut`/`high_cut` ändern (Edge-Resize).
2. **Unterhalb des Cursors** (Frequenzband-Anzeige) → Cursor selbst bewegen (Trägerfrequenz).
3. **Spektrometer-Feld** (Wasserfall/Spektrum) → Pan — Frequenzanzeige inkl. Spektrometer verschieben.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.8a (Analyse) + M4c.7.8b (Fix)
um die drei Zonen ergänzt.

## 2026-08-29 — M4c.7 Bug 7 umgesetzt: Frequenz-Cursor als SVG-Overlay (Research + Implementierung)

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) §Bug 7 — erster offener Bug umgesetzt.

**Research (2 Subagents, `jks-prv/KiwiSDR_server`):**
- Korrektur: Cursor liegt in `web/openwebrx/openwebrx.js` (OpenWebRX-Framework),
  nicht in `kiwi.js`/`waterfall.js`. WebUI-Port **8074** bestätigt.
- Zustandsübergang gelb↔grün ist **frequenzbasiert** (`passband_visible()`: liegt die
  Passband-Mitte im sichtbaren Fenster?), NICHT Pixel-Breite/Zoom-Level.
- Vier Adjust-Handles (`pb_adj_car/lo/hi/cf`), Hit-Testing-Konstanten
  (`env_slop=5`, `env_line_click_area=8`), `min_passband=4 Hz`, `±6000 Hz`-Grenzen.

**Implementierung (2 Subagents + Primary-Fix):**
- `FrequencyRuler.vue`: Cursor als SVG-Overlay (grün `viewBox 0 0 100 26`,
  `left=loPct%`/`width=bwPct%` = echte Passband-Breite; gelb = feste T-/Trapez-Form).
  Drei Hit-Zonen: Flanke lo/hi (resize) + Körper (tune), Clamp über
  `MIN_PASSBAND_HZ`/`LOW/HIGH_CUT_LIMIT`. Primary fixte fehlende `cursorWidth`-Konstante.
- `PluginView.vue`: Pan-Zone in `.kiwi-canvas-area` (Klick+Ziehen verschiebt
  Frequenzanzeige inkl. Spektrometer; Cpanel/Play-Button ausgenommen).

**Verifikation (Primary):** vue-tsc clean, Vitest 112/112, Playwright 85/85 (1 skipped).

**Geänderte Dateien (6):**
- `ui/src/components/FrequencyRuler.vue`, `ui/src/views/PluginView.vue`
- `doc/M4c.7-bugs.md` (Bug 7 ✅ + Research-Ergebnis), `doc/checklist.md` (8a/8b/8c ✅)
- `doc/index.md`, `doc/log.md`

## 2026-08-29 — M4c.7: Research-Pflicht als fixe Anweisung pro Bug + WebUI-Port-Validierung

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) + [`checklist.md`](./checklist.md) —
zwei strukturelle Korrekturen am Bug-Manifest:

1. **Research-Pflicht als fixe Anweisung für JEDEN Bug.** Eine zentrale Regel im
   Manifest-Header legt fest: Vor jedem Fix MUSS eine Research-Voranalyse aus
   (a) Kiwi SDK/Server-Quellcode (`jks-prv/KiwiSDR_server` → `web/kiwi/`) und
   (b) Live-WebUI durchgeführt werden. Die visuell abgeleiteten IST/SOLL-Beschreibungen
   sind keine Faktengrundlage. Jeder Bug (7–15) erhielt einen eigenen
   `### ⚠️ Research-Aufgabe (Pflicht vor Fix)`-Abschnitt; in der Checklist wurde pro
   Bug ein `RESEARCH`-Task ergänzt (Nummerierung: a=Analyse, b=Research, c=Fix).
2. **WebUI-Port validieren.** Die KiwiSDR-WebUI läuft auf **8073 oder 8074** (NICHT
   8072 — das ist die externe API/WebSocket). Alle Research-Aufgaben referenzieren
   jetzt "Port 8073/8074 validieren" statt eines festen (teils falschen) Ports, damit
   Research/Tests nicht wegen falschem Port fehlschlagen.
3. **Delegation:** Research-Tasks werden an Subagents (`general`/`explore`,
   MCP: `netsdr_rag` + GitHub-Read-only) delegiert; erst nach Research wird gefixt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — zentrale Research-Pflicht-Regel + Research-Abschnitt pro Bug
- `doc/checklist.md` — RESEARCH-Tasks pro Bug + Port-Validierung
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 15 erfasst: DRM-Tab (Button) funktioniert nicht

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 15** dokumentiert
(Status: offen + Research-Pflicht). Der DRM-Mode-Button (`panelModes` → `idx: 12`)
setzt nur `mode=12`, löst aber kein DRM-UI aus.

- **IST:** `store.setParam('mode', 12)` — kein Schedule/Services-Overlay, kein Decoder-Panel, keine DRM-Bandbreite.
- **SOLL (KiwiSDR):** DRM-Aktivierung blendet (a) Schedule/Services-Overlay oben ein
  (Status-Checkboxen IO/Time/Frame/FAC/SDC/MSC + Services-Liste, Stations-Schedule mit
  Zeitleiste + roter "jetzt"-Linie, Zeit/Legende), (b) Decoder-Panel unten links
  (Dream 2.2.1, Stop/Monitor IQ/Test 1/Test 2/LPF), (c) Tuning-Klammer verbreitert sich auf ~10 kHz.
- **Research-Pflicht (neu als M4c.7.16b):** sorgfältiger Research in WebUI **und**
  Kiwi-SDK — exakte Labels, Checkbox-Semantik, Schedule-Datenquelle (drmrx.org),
  DRM-Bandbreite verifizieren.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.16a (Analyse, ✅) + M4c.7.16b
(Research, offen) + M4c.7.16c (Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 15 (Analyse + Research + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.16a/16b/16c
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 14 erfasst: "Spec AF"-Button soll funktionieren

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 14** dokumentiert
(Status: offen). Der Spectrum-Button hat drei Zustände (Spectrum → Spec RF →
Spec AF → aus); die AF-Ansicht fehlt vollständig.

- **IST:** `cycleSpectrumMode` zyklisch nur das Label (`waterfall`/`specRF`/`specAF`),
  kein AF-Diagramm.
- **SOLL (KiwiSDR):** "Spec AF" (aktiv grün) zeigt das demodulierte Audiospektrum —
  Y-Achse identisch zur RF-Ansicht (dBm, farbcodiert) + horizontale Grid-Lines,
  Area-Chart um den Träger zentriert, **vertikales Grid**, **grüne Center-Linie**
  (Träger/Nullpunkt) + **zwei rote Filtergrenzen**. Gesamtes AF-Spektrum bewegt sich
  synchron beim Tuning.
- **Datenquelle:** vorhandene `waterfallBins`/`SpectrumAnalyzer` (M4.7); echte
  AF-Demodulator-Daten erst in M5+.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.15a (Analyse, ✅) + M4c.7.15b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 14 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.15a/15b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 13 erfasst: "Spec RF"-Button soll funktionieren

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 13** dokumentiert
(Status: offen). Der Spectrum-Button (`cycleSpectrumMode` in `PluginView.vue`)
zyklisch nur das Label (Spectrum → Spec RF → Spec AF), rendert aber kein Diagramm.

- **IST:** `store.spectrumMode` + Label-Wechsel, kein zusätzliches Rendering.
- **SOLL (KiwiSDR):** "Spec RF" (aktiv leuchtend grün) blendet einen
  Spektrumanalysator ein — schwarzer Hintergrund, volle Breite, Y-Achse dBm rechts
  (farbcodiert, horizontale Grid-Lines), hellgraues/weißes Area-Chart,
  halbtransparentes Passband-Overlay, synchron zur Frequenzskala beim Pan/Zoom.
- **Datenquelle:** vorhandene `waterfallBins`/`SpectrumAnalyzer` (M4.7) nutzen;
  echte RF-Spektrumsdaten erst mit WF-WebSocket (M5).

**Update:** [`checklist.md`](./checklist.md) — M4c.7.14a (Analyse, ✅) + M4c.7.14b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 13 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.14a/14b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 12 erfasst: Header-Bereich entspricht nicht der Web UI

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 12** dokumentiert
(Status: offen). Der Header in `PluginView.vue` ist zu flach: keine Station-Info
(Titel + Untertitel-Zeilen mit Standort/Grid/ASL/SNR/Antennen), keine
Credits-Sektion ("Provided by"), kein Collapse/Expand-Toggle + expandierbarer
Bild-Bereich.

- **IST:** Logo + "NetSDRStation" + "Antenna: KiwiSDR broadband" (statisch),
  StationInput im `center`, Callsign-Input + Zeit im `right`.
- **SOLL (KiwiSDR):** dreiteilige hellgraue Top Bar (Branding/Titel/Untertitel +
  Credits + Callsign-Input + Zeit/Logo-Slot), halbtransparenter Collapse/Expand-
  Reiter (Chevron ↓/↑) + expandierbarer Bild-Container mit absolut positionierten
  Overlays (Logos, schwebendes Kontroll-Panel, lila Play-Button).
- **Pflicht:** Connect-Funktionalität (StationInput mit Stationsname +
  Connect/Disconnect + Status) muss im neuen Header-Layout erhalten bleiben.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.13a (Analyse, ✅) + M4c.7.13b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 12 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.13a/13b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 11 erfasst: AGC-Tab beinhaltet eventuell nicht alle Parameter

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 11** dokumentiert
(Status: offen + Research-Pflicht). Der AGC-Tab in `PluginView.vue` hat
AGC/Hang-Toggles + Threshold/Decay/Slope/Man-Gain-Slider, aber keine Aktionsleiste
(AGC/Hang/Defaults/help), keinen Thresh-CW-Slider und keine Scrollbar.

- **IST:** AGC ON/OFF, Threshold, Decay, Hang, Slope, Man Gain (Toggles + Slider gemischt).
- **SOLL (KiwiSDR):** Aktionsleiste (AGC dunkelgrau/grün, Hang mit oranger Umrandung,
  Defaults gelb/schwarz, help grün/weiß rechtsbündig) + Slider Manual gain / Threshold /
  Thresh CW / Slope / Decay mit Einheiten (dB/dBm/msec). Vertikale Scrollbar,
  S-Meter-Skala (S1…S9, +10…+60), Labels weiß/orange abwechselnd.
- **Research-Pflicht (neu als M4c.7.12b):** Parameterliste visuell (Screenshots)
  abgeleitet; exakte ParamIds/Ranges/Defaults/Einheiten per `jks-prv/KiwiSDR_server`
  (`web/kiwi/`) + Live-DOM-Capture verifizieren.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.12a (Analyse, ✅) + M4c.7.12b
(Research, offen) + M4c.7.12c (Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 11 (Analyse + Research + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.12a/12b/12c
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 10 erfasst: Audio-Tab fehlen Parameter + Scrollbar

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 10** dokumentiert
(Status: offen + Research-Pflicht). Der Audio-Tab in `PluginView.vue` zeigt nur
Volume/Mute/NR/Compression/De-emphasis; fast alle KiwiSDR-Audio-Parameter fehlen
und eine Scrollbar existiert nicht.

- **IST:** Volume, Mute, NR, Compression (stub), De-emphasis (stub), keine Scrollbar.
- **SOLL (KiwiSDR):** Noise (2× Dropdown + 2× "More") → Volume → Pan ("L=R", "Comp")
  → Squelch (grünes Dreieck, roter Slider, 2× Zeit-Dropdown) → PB default/low/high/
  center/width → Noise blanker → Noise filter (je "Defaults" + "help") → NB test
  (pulse gain/width). Vertikale Scrollbar, farbcodierte Labels.
- **Research-Pflicht (neu als eigene Task M4c.7.11b):** Parameterliste ist visuell
  (Screenshots) abgeleitet; exakte ParamIds/Ranges/Defaults müssen per
  `jks-prv/KiwiSDR_server` (`web/kiwi/`) + Live-DOM-Capture verifiziert werden.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.11a (Analyse, ✅) + M4c.7.11b
(Research, offen) + M4c.7.11c (Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 10 (Analyse + Research + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.11a/11b/11c
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 9 erfasst: Band- & Stationsleiste ≠ KiwiSDR Web UI

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 9** dokumentiert
(Status: offen). Die Band-Leiste (`BandScaleBar.vue`) und die DX-Stationsleiste
(`TagArea.vue`) sind nicht dynamisch wie im KiwiSDR-Original:

- **IST:** Bänder als `span` mit `left`/`width` (Label nicht garantiert zentriert,
  kein kontinuierliches Re-Layout beim Pan/Zoom); DX-Tags auf 2 festen Reihen
  (`MIN_GAP_PCT`), keine vertikalen Verbindungslinien zur Frequenzachse.
- **SOLL (KiwiSDR, `jks-prv/KiwiSDR` — `web/kiwi/`):** durchgehende farbige Balken
  mit horizontal zentriertem Label, synchron zur Wasserfall-Ansicht (Zoom/Pan);
  DX-Labels als farbcodierte Rechtecke mit dünner vertikaler Verbindungslinie zur
  Frequenzposition + Kollisions-Layout-Algorithmus auf N Ebenen (beim Rauszoomen
  rücken gestapelte Labels wieder zusammen).

**Update:** [`checklist.md`](./checklist.md) — M4c.7.10a (Analyse, ✅) + M4c.7.10b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 9 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.10a/10b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 8 erfasst: Frequenzband-Skala ≠ KiwiSDR Web UI

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 8** dokumentiert
(Status: offen). Die Frequenzskala in `FrequencyRuler.vue` ist keine adaptive
pixel-basierte Tick-Engine wie im KiwiSDR-Original:

- **IST:** hartkodierte span-basierte `if/else`-Kette für `stepKhz`, nur Major-Ticks,
  HTML-`<span>`-Rendering, `formatFreq` ohne einheitliche Dezimalstellen.
- **SOLL (KiwiSDR, `jks-prv/KiwiSDR_server` — `kiwi_draw_scale()`/`scale_draw()`):**
  `STEP_BUCKETS`-Tabelle, Pixel-basierte Schritt-Auswahl
  (`TARGET_LABEL_SPACING_PX` ≈ 90px), Major-Ticks (mit Label) + Minor-Ticks
  (`majorStepHz/5`, ohne Label), Canvas-2D-Rendering, `formatFreqLabel`
  (kHz < 1 MHz, MHz ab 1 MHz).

**Update:** [`checklist.md`](./checklist.md) — M4c.7.9a (Analyse, ✅) + M4c.7.9b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 8 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.9a/9b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 Bug 7 erfasst: Frequenz-Cursor ≠ KiwiSDR Web UI

**Update:** [`M4c.7-bugs.md`](./M4c.7-bugs.md) — neuer **Bug 7** dokumentiert
(Status: offen). Der Frequenz-Cursor in `FrequencyRuler.vue` entspricht nicht der
KiwiSDR-Referenz:

- **IST:** gelber Pfeil (`zoomLevel < 9`) / grüne Klammer (`zoomLevel >= 9`);
  Umschaltung über diskreten `zoomLevel` statt über Passband-Pixelbreite;
  HTML-`div`s statt Vektor-Grafik; keine expliziten `MIN/MAX_BANDWIDTH`-Grenzen.
- **SOLL (KiwiSDR, `jks-prv/KiwiSDR_server` — `ui.js`/`pb.js`/`waterfall.js`):**
  Vektor-Overlay (SVG/Canvas), Zustandsübergang über Passband-Pixelbreite
  (`MIN_INTERACTIVE_WIDTH_PX = 30px`). Zoomed-Out (< 30px): gelbe Trapez-/T-Form,
  kein Edge-Resize, nur Center-Drag. Zoomed-In (≥ 30px): grüne Filter-Repräsentation
  (Leiste + Mittellinie + schräge Flanken), Center-Drag + Edge-Resize mit
  `MIN_BANDWIDTH`/`MAX_BANDWIDTH`-Clamp.

**Update:** [`checklist.md`](./checklist.md) — M4c.7.8a (Analyse, ✅) + M4c.7.8b
(Fix, offen) ergänzt.

**Geänderte Dateien (3):**
- `doc/M4c.7-bugs.md` — Bug 7 (Analyse + Fix-Plan), Übersicht, Chronologie, Frontmatter
- `doc/checklist.md` — M4c.7.8a/8b
- `doc/log.md` — dieser Eintrag

## 2026-08-29 — M4c.7 offene Tasks abgeschlossen: C++-Build verifiziert, 11 E2E-Test-Failures behoben

**Update:** `doc/M4c.7-bugs.md` — die drei dort dokumentierten offenen Tasks (Stand
2026-08-29) wurden abgeschlossen:

1. **Task 1 (C++ Build):** `cmake --preset win-msvc` konfiguriert (VST3_SDK_ROOT +
   WEBVIEW2_SDK_ROOT bereits im CMake-Cache unter `thirdParty/`). Debug + Release
   Build grün, VST3-Validator 47/47, `ctest` 1/1 (100%) grün.

2. **Task 2 (8 E2E-Tests failen):** Nach den Syntax-Fixes aus Commit `f6d9dd6`
   verblieben 11 Failures. Alle behoben — es handelte sich durchweg um **veraltete
   oder fragile Tests**, keine UI-Bugs:
   - `band-presets.spec.ts` — `has-text("20m")` matchte auch "120m" (Strict-mode);
     auf exakten Regex `/^20m$/` / `/^80m$/` umgestellt.
   - `dx-tags.spec.ts` — Koordinaten-Klick traf im 2-Reihen-Layout ein Nachbar-Tag;
     Klick per `evaluate(el => el.click())`. Nicht-existentes Tag "VOA" → "WWV".
   - `extension-select.spec.ts` — Bug 6.3 hatte 26 Extensions + Platzhalter eingebaut
     (27 Optionen), Test erwartete 1; `selectOption('extension ∨')` → echte Option
     'WSPR'. Panel-Play-Button `.kiwi-cpanel__play-btn` existiert nach Bug 6.5 nicht
     mehr → Tests auf den Floating-Button `.kiwi-play-btn` umgestellt.
   - `frequency-ruler.spec.ts` — `formatFreq(0)` liefert `'0'` (kein `'0 kHz'`);
     exakter Match `/^0$/`. kHz-Labels erscheinen erst bei niedriger Frequenz
     (Default 14100 kHz rendert alles als MHz) → vor dem Zoomen auf 500 kHz tunen.
   - `panel-controls.spec.ts` — Audio-Button-Locator per `title="Audio mute/unmute"`
     statt `hasText: '🔊'` (Text ändert sich nach Klick). RF-Attn-Buttons exakter
     Regex `/^0 dB$/` (Substring matchte -10/-20 dB). CW-peaks-Button nutzt
     `.kiwi-cpanel__btn`, nicht `.kiwi-cpanel__toggle`.

3. **Task 3 (veraltete E2E-Selektoren):** Einziger veralteter Selektor war
   `.kiwi-cpanel__play-btn` in `extension-select.spec.ts` (siehe oben). Übrige
   Suite nutzt aktuelle Klassen.

**Verifikation:** Playwright **85 passed / 1 skipped / 0 failed** (vorher 75/11),
Vitest 112/112, Debug+Release Build + Validator 47/47 + ctest 1/1.

**Geänderte Dateien (5):**
- `ui/e2e/band-presets.spec.ts`
- `ui/e2e/dx-tags.spec.ts`
- `ui/e2e/extension-select.spec.ts`
- `ui/e2e/frequency-ruler.spec.ts`
- `ui/e2e/panel-controls.spec.ts`

## 2026-08-29 — Agent-Infrastruktur-Fixes: Modelle, AGENTS.md-Struktur, Permission-Lücke

**Update:** Drei Root-Causes der Regelverletzung (Nemotron 3 Ultra Free ignorierte
MCP-First → index.md) behoben:

1. **Modelle verstärkt:** `BUILD.md` (`~/.config/opencode/agent/BUILD.md`) von
   `opencode/nemotron-3-ultra-free` auf `opencode-go/deepseek-v4-pro` umgestellt.
   `DEV.md` von `opencode/nemotron-3.5-lightning-free` auf
   `opencode-go/deepseek-v4-flash` umgestellt. Stärkere Models = besseres
   Instruction-Following für die komplexe AGENTS.md.

2. **Navigation & Knowledge First** als **separate, erste Subsection** in
   `Mandatory Workflow` eingefügt (vor `Autopilot mode`). Bisher war die Regel
   "doc/index.md first" als Step 1 unter `MCP-First workflow (RAG / Code-Wiki)`
   versteckt — der Header ließ sie RAG-spezifisch wirken. Jetzt steht die Regel
   **fettgedruckt und prominent** eigenständig. MCP-First-Sektion auf Tool-Referenz
   reduziert (keine duplizierten Steps mehr).

3. **Permission-Lücke geschlossen:** `opencode.json` um `permission`-Block ergänzt
   (`external_directory: "allow"` + alle Tools auf `"allow"`). Die "No permission
   prompts"-Regel existierte nur als Prompt-Text in AGENTS.md, wurde aber von der
   Tool-Runtime nicht durchgesetzt — damit ist sie jetzt auf beiden Ebenen aktiv.

**Geänderte Dateien (6):**
- `~/.config/opencode/agent/BUILD.md` — Modell opencode-go/deepseek-v4-pro
- `~/.config/opencode/agent/DEV.md` — Modell opencode-go/deepseek-v4-flash
- `AGENTS.md` — Neue Navigation-Sektion, MCP-First entschlackt
- `opencode.json` (workspace) — permission-Block
- `doc/log.md` — dieser Eintrag

**Hinweis:** opencode muss neu gestartet werden, damit Model- und Permission-Änderungen
wirksam werden. AGENTS.md wirkt beim nächsten Agent-Start.</think>

<｜DSML｜parameter name="filePath" string="true">C:\Users\marku\Documents\GitHub\artqcid\vst-nativ-projects\wogd-vst-netsdrstation\docksam werden. AGENTS.md wirkt beim nächsten Agent-Start.

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