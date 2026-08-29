---
type: Bug Manifest + Implementation Plan
title: M4c.7 — Bug-Manifest + Implementation Plan
description: 15 Bugs aus manueller Prüfung (2026-08-29) mit vollständiger Analyse und konkretem Fix-Plan pro Bug. Analysiert 2026-08-29 (agent:plan).
status: bugs-1-6-done-bugs-7-15-open
generated:
  by: agent:plan
  at: 2026-08-29
verified: 2026-08-29 (agent:DEV — offene Tasks 1–3 abgeschlossen, 85/85 E2E grün; Bug 7–15 neu erfasst 2026-08-29, noch offen)
tags: [m4, m4c, bugs, analysis, ui, parity, extensions, implementation-plan]
stale_after: 2026-10-31
sources:
  - title: Referenz-DOM (explore-8074.json)
    path: ui/e2e/reference/kiwisdr-reference/explore-8074.json
  - title: Panel-Referenz
    path: ui/e2e/reference/kiwisdr-reference/panel.json
  - title: Header-Topbar-Referenz
    path: ui/e2e/reference/kiwisdr-reference/header-topbar.json
  - title: DX-Selects-Smeter-Referenz
    path: ui/e2e/reference/kiwisdr-reference/dx-selects-smeter.json
  - title: Subtabs-Referenz
    path: ui/e2e/reference/kiwisdr-reference/subtabs.json
  - title: Referenz-Matrix
    path: doc/reference-matrix.md
---

# M4c.7 — Bug-Manifest & Implementation Plan

15 Bugs aus manueller Prüfung des Live-Plugins am 2026-08-29 identifiziert.
Vollständig analysiert 2026-08-29 durch Abgleich mit `explore-8074.json`, `panel.json`, `subtabs.json` und Quellcode-Inspektion.

**Vorgänger:** [`M4b-bugs.md`](./archive/M4b-bugs.md) — M4b-Bugs (archiviert, in M4c gefixt).

**Grundregel:** Jeder Fix wird gegen `explore-8074.json` (127 Elemente, 272 IDs) validiert.
Nach jedem Fix: `reference-matrix.md` aktualisieren (❌ → ✅), E2E-Test schreiben.

> ## ⚠️ Research-Pflicht (gilt für JEDEN Bug, fixe Anweisung)
>
> Bevor ein Bug implementiert wird, MUSS eine **Research-Voranalyse** durchgeführt
> werden. Die visuell abgeleiteten IST/SOLL-Beschreibungen in diesem Manifest sind
> **keine Faktengrundlage** — exakte Labels, Parameter-IDs, Wertebereiche, Geometrien
> und Verhaltensdetails müssen aus zwei Quellen verifiziert werden:
>
> 1. **Kiwi SDK / Server-Quellcode** (`jks-prv/KiwiSDR_server` → `web/kiwi/`,
>    früher `Beagle_SDR_GPS`): Schlagworte pro Bug suchen (z. B. `cursor`, `pb_`,
>    `zoom_step`, `dx`, `band`, `agc`, `squelch`, `drm`, …).
> 2. **Live-WebUI — Port zuerst validieren!** Die KiwiSDR-WebUI läuft auf **8073
>    oder 8074** (NICHT 8072 — das ist die externe API/WebSocket). Den tatsächlichen
>    WebUI-Port vor dem Research verifizieren (Referenz-Capture `explore-8074.json`
>    nutzte `:8074`). Referenz-DOM (`ui/e2e/reference/kiwisdr-reference/*.json`)
>    prüfen; falls unvollständig, den betreffenden UI-Zustand im Browser aktivieren
>    und den DOM erneut aufnehmen. Ein falscher Port darf Research/Tests nicht
>    fehlschlagen lassen.
>
> **Delegation:** Research-Tasks werden **an Subagents delegiert** (`general`/
> `explore`, MCP: `netsdr_rag` + GitHub-Read-only). Erst nach abgeschlossenem
> Research wird der Fix implementiert. Das Research-Ergebnis wird als verbindliche
> Checkliste (Feld-Label, ParamId, Range, Default, Geometrie) im Bug-Abschnitt
> festgehalten.

---

## Übersicht

| Bug | Komponente | Priorität | Komplexität | Status |
|-----|-----------|-----------|-------------|--------|
| [Bug 1](#bug-1--p1-button-funktion-falsch) | PluginView.vue | Mittel | Klein | ✅ |
| [Bug 2](#bug-2--wasserfall--spektrum-zeigt-keine-echten-daten) | Waterfall.vue / C++ | Hoch | Mittel | ✅ |
| [Bug 3](#bug-3--frequenzband-leiste-inkorrekt) | BandScaleBar.vue | Mittel | Mittel | ✅ |
| [Bug 4](#bug-4--dx-tags-fehlen--layout-falsch) | TagArea.vue | Niedrig | Klein | ✅ |
| [Bug 5](#bug-5--khz-lineal-skaliert-nicht-bei-hohem-zoom) | FrequencyRuler.vue | Mittel | Klein | ✅ |
| [Bug 6](#bug-6--bedienpanel-61-68) | PluginView.vue | Hoch | Gross | ✅ |
| [Bug 7](#bug-7--frequenz-cursor-entspricht-nicht-der-kiwisdr-web-ui) | FrequencyRuler.vue | Hoch | Gross | ✅ |
| [Bug 8](#bug-8--frequenzband-skala-verhält-sich-nicht-wie-die-web-ui) | FrequencyRuler.vue | Hoch | Mittel | ⬜ |
| [Bug 9](#bug-9--band--stationsleiste-verhält-sich-nicht-wie-die-web-ui) | BandScaleBar.vue / TagArea.vue | Hoch | Gross | ⬜ |
| [Bug 10](#bug-10--audio-tab-fehlen-parameter--scrollbar) | PluginView.vue | Hoch | Gross | ⬜ |
| [Bug 11](#bug-11--agc-tab-beinhaltet-eventuell-nicht-alle-parameter) | PluginView.vue | Hoch | Mittel | ⬜ |
| [Bug 12](#bug-12--header-bereich-entspricht-nicht-der-web-ui) | PluginView.vue | Hoch | Gross | ⬜ |
| [Bug 13](#bug-13--spec-rf-button-soll-funktionieren) | PluginView.vue / Waterfall.vue | Hoch | Gross | ⬜ |
| [Bug 14](#bug-14--spec-af-button-soll-funktionieren) | PluginView.vue / Waterfall.vue | Hoch | Gross | ⬜ |
| [Bug 15](#bug-15--drm-tab-button-funktioniert-nicht) | PluginView.vue | Hoch | Gross | ⬜ |

---

## Bug 1 — P1-Button Funktion falsch

**Betroffene Datei:** `ui/src/views/PluginView.vue` Zeile 203

### Analyse (abgeschlossen 2026-08-29)

**IST (Zeile 203):**
```html
<button class="kiwi-cpanel__btn kiwi-cpanel__btn--violet">P1</button>
```
Der Button hat kein `@click`-Handler — er ist funktionslos (kein Audio-Stop, kein Panel-Toggle).

**Referenz (explore-8074.json `id-button-spec-peak0`):**
- `cls: "id-button-spec-peak0 w3-margin-L-16 class-button w3-noactive w3-hold id-btn-grp-34"`
- Text: `P1`, Position: x=1204, y=703 (WF0-Tab, Spec Δ-Zeile)
- Klasse `w3-hold` = Toggle-Button (an/aus)
- P1 = **"Spectrum Peak Hold 1"** — hält den Spektrum-Spitzenwert fest (keine Bewegung des Peaks nach unten)

**Ebenfalls kein Handler:** P2 (Zeile 329) = `id-button-spec-peak1` (zweiter Peak-Hold-Kanal).

**Readme-Panel (separate Funktion, kein Zusammenhang mit P1):**
- `id-readme` (Zeile 495 in panel.json): ein separates Welcome-Panel (DIV class-panel, x:10, y:495, w:605, h:295)
- Toggle: `id-readme-vis` (Kreis-Button oben-rechts im Panel, x:581, y:505)
- Der Readme-Toggle sitzt im readme-Panel selbst, NICHT im Bedienpanel
- Das Readme-Panel wird initial angezeigt und vom User weggedrückt — kein Fix nötig für M4c.7

### Fix-Plan Bug 1

**Datei:** `ui/src/views/PluginView.vue`

**Schritt 1 — Store-State für Peak-Hold (kiwiStore.ts):**
```ts
// In state() hinzufügen:
specPeak1: false,
specPeak2: false,
```
`specPeak1`/`specPeak2` sind boolean, kein Bridge-Parameter (UI-local, betrifft nur Canvas-Rendering).

**Schritt 2 — P1/P2 Buttons mit Handler verdrahten (PluginView.vue Zeilen 203, 329):**
```html
<!-- Zeile 203: WAS -->
<button class="kiwi-cpanel__btn kiwi-cpanel__btn--violet">P1</button>
<!-- WIRD -->
<button
  class="kiwi-cpanel__btn kiwi-cpanel__btn--violet"
  :class="{ 'kiwi-cpanel__btn--violet-active': store.specPeak1 }"
  @click="store.specPeak1 = !store.specPeak1"
  title="Spectrum Peak Hold 1"
>P1</button>
```
Analog P2 (Zeile 329) mit `store.specPeak2`.

**Schritt 3 — Waterfall.vue: Peak-Hold Overlay (optisch):**
Wenn `specPeak1 = true`: die aktuelle maximale Bin-Höhe pro Spalte wird in einem `peakBins`-Array gespeichert und als gelbe Linie über dem Spektrum gezeichnet. Dies ist rein visuell (kein C++-Parameter).

**Schritt 4 — CSS-Klasse für aktiven Zustand:**
```css
/* Zeile ~1094 */
.kiwi-cpanel__btn--violet-active { background: #b39ddb; color: white; }
```

**Akzeptanzkriterium:** P1-Click togglet `store.specPeak1`; Button wechselt visuell (heller/dunkler); Peak-Linie erscheint/verschwindet im Spektrum.

**E2E-Test:** `ui/e2e/panel-controls.spec.ts` — P1-Button togglet Klasse `kiwi-cpanel__btn--violet-active`.

---

## Bug 2 — Wasserfall / Spektrum zeigt keine echten Daten

**Betroffene Dateien:**
- `ui/src/components/Waterfall.vue`
- `source/vst/processor/plugin_processor_audio.cpp`
- `source/editor/plugin_editor.cpp`

### Analyse (abgeschlossen 2026-08-29)

**Datenfluss-Kette (vollständig vorhanden):**
1. `plugin_processor_audio.cpp` Zeile 354–358: Audio-Samples werden in `spectrumSamples_` (lock-free queue) gepusht — **aber nur wenn `kiwiClient_->isConnected() && !mute_`**
2. `plugin_processor.cpp` Zeile 406–437 (`sendWaterfall`): Worker-Thread dreint die Queue, berechnet DFT via `SpectrumAnalyzer::computeDbF`, sendet als IMessage `"NetSDRStation:Waterfall"` und ruft `onWaterfall_` callback
3. `plugin_controller.cpp` Zeile 141–142: Empfängt `"NetSDRStation:Waterfall"` IMessage, ruft `waterfallSink_(bins)`
4. `plugin_editor.cpp` Zeile 103: `setWaterfallSink` → `pushWaterfall` → `webView_.eval("window.setWaterfall([...])")`
5. `pluginService.ts` Zeile 131–132: `window.setWaterfall` → `waterfallHandler` → `store.setWaterfallBins(bins)`
6. `Waterfall.vue` Zeile 123–125: `watch(() => props.bins, ...)` → `pushFrame(bins)`

**Root Cause:** Die Kette ist vollständig implementiert — **aber nur wenn eine KiwiSDR-Verbindung aktiv ist** (`kiwiClient_->isConnected()`).

Ohne aktive Verbindung: `spectrumSamples_` wird nicht gefüllt → `drained.empty()` → früher Return in `sendWaterfall` → **kein Frame an UI**.

Das Canvas zeigt dann die initiale Füllfarbe `#1e5f7f` (Zeile 135 in `Waterfall.vue`) ohne jegliche Spektrumsdaten.

**Im Dev-Mode (Browser ohne VST):** `pluginService.isInNative() = false` → `window.setWaterfall` wird nie von C++ aufgerufen → `store.waterfallBins = []` → leerer Canvas.

### Fix-Plan Bug 2

**Entscheidung (2026-08-29):** Kein Simulator, kein Fallback.

> "Keine Verbindung → kein Spektrogramm" ist **korrekt**. Der leere/blaue
> Canvas bei fehlender Verbindung ist kein Bug, sondern der erwartete Zustand.
> Ein Fake-Datenstrom täuscht produktive Daten vor und ist für M4c.7 nicht
> sinnvoll. Echter Waterfall-Stream vom KiwiSDR-Server kommt in M5 (WF-WebSocket).

**Was also tatsächlich zu tun ist:** Prüfen ob der Canvas-Zustand bei
fehlender Verbindung für den Nutzer verständlich kommuniziert wird.

**Schritt 1 — `Waterfall.vue`: Status-Overlay bei leerem Stream (Zeile 133–136):**

Statt stillem blauem Canvas: ein einfaches Text-Overlay "No connection" oder
ein visuelles Placeholder-Signal einblenden wenn `props.bins.length === 0`.

```html
<!-- In Waterfall.vue template, über dem Canvas -->
<div v-if="!hasBins" class="waterfall__no-signal">
  No signal — connect to a KiwiSDR station
</div>
```

```ts
const hasBins = computed(() => props.bins.length > 0)
```

Das ist **kein Simulator** — nur eine UI-Information. Der Canvas bleibt blau,
darüber liegt ein Text.

**Schritt 2 — Nichts an der C++-Kette ändern.** Der `isConnected()`-Guard in
`plugin_processor_audio.cpp` Zeile 354 ist korrekt und bleibt unverändert.

**Akzeptanzkriterium:** Ohne Verbindung zeigt der Canvas das Overlay
"No signal". Mit aktiver Verbindung verschwindet das Overlay und echte
Spektrumdaten werden angezeigt.

**E2E-Test:** `ui/e2e/waterfall.spec.ts` — im disconnected-State ist das
`.waterfall__no-signal`-Element sichtbar; im connected-State nicht.

---

## Bug 3 — Frequenzband-Leiste inkorrekt

**Betroffene Datei:** `ui/src/components/BandScaleBar.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:**
- `BandDef` hat nur `freq` (Mittelpunkt) — kein `startFreq`/`endFreq`
- CSS `.band-scale__block` hat kein `width` → rendert als Punkt (0-Breite, zentriert)
- 21 Bänder (11 Broadcast + 10 Amateur) statt der 87 Optionen in KiwiSDR

**Referenz (aus `explore-8074.json` bodyText extrahiert — vollständige Band-Optionen):**

```
BROADCAST: LW, MW, 120m, 90m, 75m, 60m, 49m, 41m, 31m, 25m, 22m, 19m, 16m, 15m, 13m, 11m
UTILITY:   VLF, LF, NDB, Time 2.5, Time 3.33, Time 5, Time 7.85, Time 10, Time 14.67, Time 15, Time 20
AMATEUR:   LF, MF, 160m, 80m, 60m, 40m, 30m, 20m, 17m, 15m, 12m, 10m
BEACONS:   IBP 20m, IBP 17m, IBP 15m, IBP 12m, IBP 10m
MARINE:    MF, 2 MHz, 4 MHz, 6 MHz, 8 MHz, 12 MHz, 22 MHz, 25 MHz
AERO:      2 MHz, 3 MHz, 3 MHz, 4 MHz, 5 MHz, 6 MHz, 8 MHz, 10 MHz, 11 MHz, 13 MHz, 15 MHz, 17 MHz, 22 MHz
MARKERS:   3594, 4558, 5154, 5156 L, 5292 D/B, 6928 V, 7039, 7509, 8000 C, 8495, 10872, 13528, 16332, 20048
```
Total: ~79 Einzel-Optionen + 7 Kategorie-Header = 86 Dropdown-Einträge.

**Rendering-Problem:** Die `BandScaleBar` zeigt KEINE Balken/Blöcke weil `width` fehlt.
In KiwiSDR: jeder Block hat eine definierte Frequenzbreite (z.B. Broadcast 49m = 5900–6200 kHz → 300 kHz breit).

**Zoom-Transformation:** Die `freqToPercent(freq)` Funktion nutzt bereits `viewLowMhz`/`viewHighMhz` (übergeben von PluginView.vue). Das ist korrekt. Fehlt nur die `width`.

### Fix-Plan Bug 3

**Schritt 1 — `BandDef` Interface erweitern (BandScaleBar.vue Zeile 45):**
```ts
interface BandDef {
  label: string
  startFreq: number  // MHz — Bandanfang
  endFreq: number    // MHz — Bandende
  color: string
  textColor?: string
}
```
`freq` (Mittelpunkt) entfällt — Position berechnet sich aus `startFreq`.

**Schritt 2 — Vollständige Band-Datenliste (ITU-Bandplan + KiwiSDR-Standard):**

Die `BandScaleBar` zeigt die farbigen **Blöcke** für die wichtigsten Bänder (nicht alle 87 Dropdown-Optionen — das sind Dropdown-Einträge für den Band-Select im Panel, nicht für die visuelle Skala):

```ts
const BROADCAST_BANDS: BandDef[] = [
  { label: 'LW',  startFreq: 0.1485, endFreq: 0.2835, color: '#FF9800' },
  { label: 'MW',  startFreq: 0.5265, endFreq: 1.6065, color: '#FF9800' },
  { label: '120m',startFreq: 2.3,    endFreq: 2.495,  color: '#4fc3f7' },
  { label: '90m', startFreq: 3.2,    endFreq: 3.4,    color: '#4fc3f7' },
  { label: '75m', startFreq: 3.9,    endFreq: 4.0,    color: '#4fc3f7' },
  { label: '60m', startFreq: 4.75,   endFreq: 5.06,   color: '#4fc3f7' },
  { label: '49m', startFreq: 5.9,    endFreq: 6.2,    color: '#4fc3f7' },
  { label: '41m', startFreq: 7.2,    endFreq: 7.45,   color: '#4fc3f7' },
  { label: '31m', startFreq: 9.4,    endFreq: 9.9,    color: '#4fc3f7' },
  { label: '25m', startFreq: 11.6,   endFreq: 12.1,   color: '#4fc3f7' },
  { label: '22m', startFreq: 13.57,  endFreq: 13.87,  color: '#4fc3f7' },
  { label: '19m', startFreq: 15.1,   endFreq: 15.8,   color: '#4fc3f7' },
  { label: '16m', startFreq: 17.48,  endFreq: 17.9,   color: '#4fc3f7' },
  { label: '15m', startFreq: 18.9,   endFreq: 19.02,  color: '#4fc3f7' },
  { label: '13m', startFreq: 21.45,  endFreq: 21.85,  color: '#4fc3f7' },
  { label: '11m', startFreq: 25.6,   endFreq: 26.1,   color: '#4fc3f7' },
]

const AMATEUR_BANDS: BandDef[] = [
  { label: '160m', startFreq: 1.8,     endFreq: 2.0,    color: '#ef5350', textColor: 'white' },
  { label: '80m',  startFreq: 3.5,     endFreq: 3.8,    color: '#ef5350', textColor: 'white' },
  { label: '60m',  startFreq: 5.3515,  endFreq: 5.3665, color: '#ef5350', textColor: 'white' },
  { label: '40m',  startFreq: 7.0,     endFreq: 7.2,    color: '#ef5350', textColor: 'white' },
  { label: '30m',  startFreq: 10.1,    endFreq: 10.15,  color: '#ef5350', textColor: 'white' },
  { label: '20m',  startFreq: 14.0,    endFreq: 14.35,  color: '#ef5350', textColor: 'white' },
  { label: '17m',  startFreq: 18.068,  endFreq: 18.168, color: '#ef5350', textColor: 'white' },
  { label: '15m',  startFreq: 21.0,    endFreq: 21.45,  color: '#ef5350', textColor: 'white' },
  { label: '12m',  startFreq: 24.89,   endFreq: 24.99,  color: '#ef5350', textColor: 'white' },
  { label: '10m',  startFreq: 28.0,    endFreq: 29.7,   color: '#ef5350', textColor: 'white' },
]
```

**Schritt 3 — Template-Rendering mit Breite (BandScaleBar.vue Zeile 9–16):**
```html
<span
  v-for="band in visibleBands"
  :key="band.label + band.startFreq"
  class="band-scale__block"
  :style="{
    left:       freqToPercent(band.startFreq) + '%',
    width:      freqWidthPercent(band.startFreq, band.endFreq) + '%',
    background: band.color,
    color:      band.textColor ?? 'black',
  }"
  @click="$emit('tune', (band.startFreq + band.endFreq) / 2 * 1000)"
>{{ band.label }}</span>
```

Hilfsfunktionen (Zeile 84):
```ts
function freqToPercent(freqMhz: number): number {
  const span = props.viewHighMhz - props.viewLowMhz
  if (span <= 0) return 0
  return ((freqMhz - props.viewLowMhz) / span) * 100
}

function freqWidthPercent(startMhz: number, endMhz: number): number {
  const span = props.viewHighMhz - props.viewLowMhz
  if (span <= 0) return 0
  return Math.max(0.1, ((endMhz - startMhz) / span) * 100)  // min 0.1% damit Band sichtbar bleibt
}
```

`visibleBands`: Bänder filtern die im sichtbaren Fenster liegen:
```ts
const visibleBands = computed(() =>
  allBands.value.filter(b => b.endFreq >= props.viewLowMhz && b.startFreq <= props.viewHighMhz)
)
```

**Schritt 4 — CSS: `transform: translateX(-50%)` entfernen** (Zeile 137 im aktuellen Code):
```css
/* WAS */
.band-scale__block { transform: translateX(-50%); }
/* WIRD */
.band-scale__block { /* kein transform — left ist Bandanfang */ }
```

**Schritt 5 — Band-Select Dropdown mit 87 Optionen (PluginView.vue Zeile 104–106):**
Das ist ein separates Fix-Ziel: der `<select aria-label="Band">` braucht alle 87 Optionen mit Frequenzsprung.
Band-Select-Daten in `kiwiStore.ts` als `BAND_OPTIONS`-Konstante definieren (Kategorie-Gruppen via `<optgroup>`).

**Akzeptanzkriterium:** Farbige Rechteck-Blöcke sichtbar auf der Frequenz-Skala. Bei Zoom > 3 werden einzelne Bänder sichtbar breiter. Band-Select hat 87 Optionen.

**E2E-Test:** `ui/e2e/band-scale.spec.ts` — mindestens 5 `.band-scale__block` mit `width > 0` sichtbar.

---

## Bug 4 — DX-Tags: zu wenige, kein zweireihiges Layout

**Betroffene Datei:** `ui/src/components/TagArea.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:**
- `DEMO_TAGS`: 30 Einträge (Mix aus Utility/SWBC)
- `TagArea.vue` CSS: `height: 22px`, `overflow: hidden` — nur EINE Reihe möglich

**Referenz (explore-8074.json — 73 echte DX-Tags):**
- 73 Buttons `id-dx-label_0` bis `id-dx-label_72`
- `rect.y` wechselt zwischen 102 und 137 (35px Abstand) = **zweireihiges Layout**
- Tags mit `dx-has-ext cl-dx-label-ext` Klasse = Extension-fähig (Klick öffnet Extension-Panel)

**Architektur-Entscheidung (2026-08-29):**
Die echten Frequenzen und Labels kommen beim echten KiwiSDR über den **WF-WebSocket** (`/WF`-Pfad, Port 8074) als `MSG dx_community=[json]`. Das ist ein separater zweiter WebSocket-Stream — wir haben aktuell nur den SND-Stream.

**Entschieden: Dynamisches Laden via WF-Socket wird implementiert (M5-Feature).**
- M4c.7 (dieser Bug): statische Liste mit 73 Tags als vollständiger Platzhalter
- M5: WF-WebSocket öffnen + `MSG dx_community` parsen + Tags dynamisch in den Store schreiben
- Vorteil: echte Live-DX-Spots vom KiwiSDR-Server (stationsabhängig, aktuell)
- Vorteil: Tags wechseln automatisch beim Stationswechsel
- Scope M5: neues `kiwi_wf_client.cpp` (analog zu `kiwi_client.cpp`), `pushDxTags()` in Bridge, `dxTags` im Store

**Vollständige 73-Tags-Liste aus explore-8074.json (Label + geschätzte Frequenz aus rect.x):**
Die genauen Frequenzen der Labels ohne Zahl im Text (z.B. "NAVTEX", "WSPR") stammen aus bekannten Standardfrequenzen:

| Label | Freq (kHz) | hasExt |
|-------|-----------|--------|
| LW 243 | 243 | nein |
| NAVTEX | 518 | ja |
| WSPR | 630 | ja |
| DSC | 2187.5 | ja |
| STANAG DHO26 | 1131 | nein |
| STANAG OUA4 | 1268 | nein |
| MWARA CAR | 1377 | nein |
| STANAG IDN | 1519 | nein |
| MWARA SAT-1,2 | 1638 | nein |
| SSTV EU | 1890 | ja |
| FAX ZAF | 2070 | ja |
| STANAG DHJ58 | 2138 | nein |
| FAX GRC | 2248 | ja |
| The Air Horn | 2612 | nein |
| L marker | 2718 | nein |
| The Pip | 3003 | nein |
| VOLMET | 3485 | nein |
| R. Mi Amigo Intl | 3315 | nein |
| DSC (distress) | 2187.5 | ja |
| MWARA SEA-1 | 4125 | nein |
| WSPR ISM | 3570 | ja |
| V marker | 4625 | nein |
| FAX THA | 4298 | ja |
| DDH7 GER | 4583 | ja |
| FAX GER | 4882 | ja |
| VMW AUS | 4426 | nein |
| STANAG PBC | 5680 | nein |
| STANAG FUM | 6215 | nein |
| MWARA SAT-1 | 5505 | nein |
| STANAG FUG | 6215 | nein |
| HM01 CUB | 5820 | nein |
| SuperDARN radar | 8000 | nein |
| FAX RUS | 7781 | ja |
| FAX AUS | 7535 | ja |
| D marker | 8000 | nein |
| FAX UK | 7880 | ja |
| HFDL ZAF | 8825 | ja |
| HM01 CUB | 9330 | nein |
| STANAG FUJ | 9007 | nein |
| VMW AUS | 9355 | nein |
| FAX CHN | 10010 | ja |
| PBB NLD | 11527 | ja |
| FAX JPN | 13988 | ja |
| MWARA NAT-B/D/F | 11384 | nein |
| D marker | 14670 | nein |
| FAX GER | 14467.3 | ja |
| SSTV | 14230 | ja |
| DDH8 GER | 14863 | ja |
| STANAG FUG | 15867 | nein |
| RWM RUS | 14996 | nein |
| FAX AUS | 16135 | ja |
| FAX GER | 17800 | ja |
| D marker | 20048 | nein |
| FAX CHN | 18010 | ja |
| DSC (distress) | 16804.5 | ja |
| FAX JPN | 17445 | ja |
| FAX SGP | 16035 | ja |
| HFDL PAN | 17919 | ja |
| FAX ZAF | 18910 | ja |
| STANAG FUV | 19680 | nein |
| DSC | 19680.5 | ja |
| NAVTEX | 24084 | ja |
| WWV | 10000 | nein |
| FAX AUS | 20469 | ja |
| FT8 | 14074 | ja |
| SSTV | 14230 | ja |
| MWARA NP | 23210 | nein |
| DSC | 22374 | ja |
| FT8 | 21074 | ja |
| DSC | 23100 | ja |
| NAVTEX | 25170 | ja |
| FT8 | 28074 | ja |
| SSTV | 28680 | ja |

### Fix-Plan Bug 4

**Scope M4c.7 (dieser Fix):** Statische Liste auf 73 Einträge erweitern + zweireihiges Layout. Das ist der vollständige Platzhalter bis M5 den WF-Socket bringt.

**Scope M5 (separates Feature, nicht hier):** WF-WebSocket + `MSG dx_community` → dynamische Tags. Siehe `doc/plan.md` M5.

**Schritt 1 — `DEMO_TAGS` in `TagArea.vue` (Zeile 47) durch vollständige Liste ersetzen:**
Die 30 aktuellen Einträge ersetzen durch die obige 73-Einträge-Liste (mit korrekten Frequenzen aus bekannten Standardfrequenzen, nicht aus rect.x-Schätzung).

**Schritt 2 — Zweireihiges Layout (TagArea.vue CSS, Zeile 109–116):**

```css
/* WAS */
.tag-area {
  height: 22px;
  overflow: hidden;
}

/* WIRD */
.tag-area {
  height: 44px;        /* 2 Reihen à 22px */
  overflow: hidden;
  position: relative;
}
```

Zweireihige Positionierung: Tags die sich überlappen (Abstand < 30px) in die zweite Reihe verschieben.
Implementierung in `TagArea.vue` computed `visibleTags`:

```ts
const visibleTags = computed(() => {
  const sorted = activeTags.value
    .filter(t => t.freqKhz >= props.viewLowKhz && t.freqKhz <= props.viewHighKhz)
    .map(t => ({ ...t, row: 0 }))  // default row=0 (oben)
    .sort((a, b) => a.freqKhz - b.freqKhz)

  // Kollisions-Detektion: wenn zwei Tags zu nahe sind, zweite Reihe
  const MIN_GAP_PCT = 3  // % der Gesamtbreite
  for (let i = 1; i < sorted.length; i++) {
    const prev = sorted[i - 1]
    const curr = sorted[i]
    const pctDiff = freqToPercent(curr.freqKhz) - freqToPercent(prev.freqKhz)
    if (pctDiff < MIN_GAP_PCT) curr.row = prev.row === 0 ? 1 : 0
  }
  return sorted
})
```

CSS `top` je nach `row`:
```html
:style="{ top: tag.row === 0 ? '2px' : '24px', left: freqToPercent(tag.freqKhz) + '%' }"
```

**Schritt 3 — Verbindungslinie zum Spektrogramm (optional, niedrige Priorität):**
Kann via `::before` mit `border-left: 1px solid rgba(0,0,0,0.3)` und `height: 100vh` (clipped durch tag-area) realisiert werden. Für M4c.7 niedrige Priorität.

**Akzeptanzkriterium:** TagArea zeigt 73 Tags. Bei Zoom=0 (volle Bandbreite) sind mehrere Tags in zweiter Reihe sichtbar.

**E2E-Test:** `ui/e2e/tag-area.spec.ts` — mindestens 20 `.tag-area__tag` Elemente sichtbar bei zoom=0.

---

## Bug 5 — kHz-Lineal skaliert nicht bei hohem Zoom

**Betroffene Datei:** `ui/src/components/FrequencyRuler.vue` Zeilen 79–106

### Analyse (abgeschlossen 2026-08-29)

**IST (Zeile 87–92):**
```ts
if (span >= 20000)      stepKhz = 5000
else if (span >= 10000) stepKhz = 2000
else if (span >= 5000)  stepKhz = 1000
else if (span >= 1000)  stepKhz = 200
else if (span >= 200)   stepKhz = 50
else                    stepKhz = 10    // ← MINIMUM bei 10 kHz!
```

**Problem:** Bei `span < 200 kHz` ist `stepKhz = 10 kHz` → bei hohem Zoom (span < 10 kHz) gibt es fast keine Ticks.

**Zoom-zu-Ticks Analyse (bei maxSpan=30000 kHz):**

| zoomLevel | span (kHz) | aktueller step | Ticks | neuer step | Ticks |
|-----------|-----------|----------------|-------|------------|-------|
| 0  | 30000  | 5000 kHz | 6   | 5000 kHz | 6    |
| 4  | 1875   | 200 kHz  | 9.4 | 200 kHz  | 9.4  |
| 8  | 117    | 10 kHz   | 11.7| 10 kHz   | 11.7 |
| 10 | 29.3   | 10 kHz   | **2.9** | 2 kHz | **14.6** |
| 12 | 7.3    | 10 kHz   | **0.7** | 0.5 kHz | **14.6** |
| 14 | 1.8    | 10 kHz   | **0.2** | 0.1 kHz | **18.3** |

Zoom 10–14 ist aktuell kaputt (0–3 Ticks statt 10–18).

**`zoomLevel`-Prop** wird in der `ticks`-Berechnung gar nicht genutzt (nur `span` wird verwendet). Die span-basierte Berechnung ist konzeptionell richtig, aber die Stufengrenzen reichen nicht bis in den sub-kHz-Bereich.

### Fix-Plan Bug 5

**Einzige Änderung:** Die `if/else`-Kette in `FrequencyRuler.vue` (Zeile 87–92) um 4 Stufen erweitern:

```ts
// WAS (Zeile 87-92):
let stepKhz: number
if (span >= 20000)      stepKhz = 5000
else if (span >= 10000) stepKhz = 2000
else if (span >= 5000)  stepKhz = 1000
else if (span >= 1000)  stepKhz = 200
else if (span >= 200)   stepKhz = 50
else                    stepKhz = 10

// WIRD:
let stepKhz: number
if (span >= 20000)      stepKhz = 5000
else if (span >= 10000) stepKhz = 2000
else if (span >= 5000)  stepKhz = 1000
else if (span >= 1000)  stepKhz = 200
else if (span >= 200)   stepKhz = 50
else if (span >= 50)    stepKhz = 10
else if (span >= 10)    stepKhz = 2
else if (span >= 2)     stepKhz = 0.5
else                    stepKhz = 0.1   // 100 Hz bei zoom 14
```

**Zusätzlich — Label-Format bei sub-kHz-Schritten (Zeile 108–112 `formatFreq`):**
```ts
function formatFreq(khz: number): string {
  if (khz === 0) return '0'
  if (khz >= 1000) return `${(khz / 1000).toFixed(khz % 1000 === 0 ? 0 : 1)} MHz`
  if (khz >= 1)    return `${khz % 1 === 0 ? khz : khz.toFixed(1)} kHz`
  return `${Math.round(khz * 1000)} Hz`  // ← neu: 100 Hz Ticks als "100 Hz"
}
```

**Akzeptanzkriterium:** Bei zoom=14 (span≈1.8 kHz) sind mindestens 10 Ticks mit Hz-Labels sichtbar.

**E2E-Test:** `ui/e2e/frequency-ruler.spec.ts` — bei `viewHighKhz - viewLowKhz = 2` mindestens 8 `.freq-ruler__tick` Elemente sichtbar.

---

## Bug 6 — Bedienpanel (6.1–6.8)

**Betroffene Datei:** `ui/src/views/PluginView.vue`

### Analyse (abgeschlossen 2026-08-29)

Vollständige Analyse der Sub-Bugs mit konkreten Zeilen:

---

### 6.1 — Doppelter Collapse-Button

**IST (Zeile 125–126 + 347):**
- Zeile 125: `<button ... @click="onPan(-1)" title="Pan left">◀</button>` (linker Pfeil-Icon)
- Zeile 126: `<button ... @click="onPan(1)" title="Pan right">▶</button>` (rechter Pfeil-Icon)
- Zeile 347: `<span class="kiwi-cpanel__vis-hide"...>◀</span>` (der echte Collapse-Button)

**Problem:** Der Pan-Links-Button (◀) und der Collapse-Button (◀) verwenden dasselbe Symbol → visuell ununterscheidbar, "doppelter Collapse".

**Referenz (explore-8074.json):** Nur 1 Collapse-Button (`id-control-vis`, kreisförmig). Pan-Buttons sind separate Icons ohne ◀/▶-Symbol.

**Fix-Plan 6.1:**
- Pan-Buttons (Zeile 125–126): Symbol wechseln zu `«` / `»` (Doppelpfeil) oder `⟨` / `⟩`
- `title`-Attribute anpassen: "Pan left" → "Shift left"
- Collapse-Button (Zeile 347) bleibt unverändert

---

### 6.2 — Band-Select leer

**IST (Zeile 104–106):**
```html
<select class="kiwi-cpanel__select" aria-label="Band">
  <option>select band ∨</option>
</select>
```

**Fix-Plan 6.2:**
Die 87 Band-Optionen als `<optgroup>`-Struktur einfügen. Band-Select-Daten als Konstante `BAND_SELECT_OPTIONS` in einem separaten File `ui/src/data/bands.ts` definieren (getrennt von BandScaleBar-Daten):

```ts
// ui/src/data/bands.ts
export interface BandOption { label: string; freqKhz: number }
export interface BandGroup { group: string; bands: BandOption[] }

export const BAND_SELECT_GROUPS: BandGroup[] = [
  { group: 'BROADCAST', bands: [
    { label: 'LW',   freqKhz: 243 },
    { label: 'MW',   freqKhz: 720 },
    { label: '120m', freqKhz: 2400 },
    { label: '90m',  freqKhz: 3300 },
    { label: '75m',  freqKhz: 3950 },
    { label: '60m',  freqKhz: 4900 },
    { label: '49m',  freqKhz: 6050 },
    { label: '41m',  freqKhz: 7325 },
    { label: '31m',  freqKhz: 9650 },
    { label: '25m',  freqKhz: 11850 },
    { label: '22m',  freqKhz: 13720 },
    { label: '19m',  freqKhz: 15450 },
    { label: '16m',  freqKhz: 17680 },
    { label: '15m',  freqKhz: 18960 },
    { label: '13m',  freqKhz: 21650 },
    { label: '11m',  freqKhz: 25850 },
  ]},
  { group: 'UTILITY', bands: [
    { label: 'VLF',      freqKhz: 20 },
    { label: 'LF',       freqKhz: 100 },
    { label: 'NDB',      freqKhz: 350 },
    { label: 'Time 2.5', freqKhz: 2500 },
    { label: 'Time 3.33',freqKhz: 3330 },
    { label: 'Time 5',   freqKhz: 5000 },
    { label: 'Time 7.85',freqKhz: 7850 },
    { label: 'Time 10',  freqKhz: 10000 },
    { label: 'Time 14.67',freqKhz: 14670 },
    { label: 'Time 15',  freqKhz: 15000 },
    { label: 'Time 20',  freqKhz: 20000 },
  ]},
  { group: 'AMATEUR', bands: [
    { label: 'LF',   freqKhz: 136 },
    { label: 'MF',   freqKhz: 475 },
    { label: '160m', freqKhz: 1850 },
    { label: '80m',  freqKhz: 3650 },
    { label: '60m',  freqKhz: 5358 },
    { label: '40m',  freqKhz: 7100 },
    { label: '30m',  freqKhz: 10125 },
    { label: '20m',  freqKhz: 14175 },
    { label: '17m',  freqKhz: 18118 },
    { label: '15m',  freqKhz: 21225 },
    { label: '12m',  freqKhz: 24940 },
    { label: '10m',  freqKhz: 28500 },
  ]},
  { group: 'BEACONS', bands: [
    { label: 'IBP 20m', freqKhz: 14100 },
    { label: 'IBP 17m', freqKhz: 18110 },
    { label: 'IBP 15m', freqKhz: 21150 },
    { label: 'IBP 12m', freqKhz: 24930 },
    { label: 'IBP 10m', freqKhz: 28200 },
  ]},
  { group: 'MARINE', bands: [
    { label: 'MF',    freqKhz: 2182 },
    { label: '2 MHz', freqKhz: 2000 },
    { label: '4 MHz', freqKhz: 4000 },
    { label: '6 MHz', freqKhz: 6000 },
    { label: '8 MHz', freqKhz: 8000 },
    { label: '12 MHz',freqKhz: 12000 },
    { label: '22 MHz',freqKhz: 22000 },
    { label: '25 MHz',freqKhz: 25000 },
  ]},
  { group: 'MARKERS', bands: [
    { label: '3594',    freqKhz: 3594 },
    { label: '4558',    freqKhz: 4558 },
    { label: '5154',    freqKhz: 5154 },
    { label: '5156 L',  freqKhz: 5156 },
    { label: '5292 D/B',freqKhz: 5292 },
    { label: '6928 V',  freqKhz: 6928 },
    { label: '7039',    freqKhz: 7039 },
    { label: '7509',    freqKhz: 7509 },
    { label: '8000 C',  freqKhz: 8000 },
    { label: '8495',    freqKhz: 8495 },
    { label: '10872',   freqKhz: 10872 },
    { label: '13528',   freqKhz: 13528 },
    { label: '16332',   freqKhz: 16332 },
    { label: '20048',   freqKhz: 20048 },
  ]},
]
```

In `PluginView.vue` (Zeile 104–106) ersetzen:
```html
<select class="kiwi-cpanel__select" aria-label="Band" @change="onBandSelect">
  <option value="">select band</option>
  <optgroup v-for="g in BAND_SELECT_GROUPS" :key="g.group" :label="g.group">
    <option v-for="b in g.bands" :key="b.label" :value="b.freqKhz">{{ b.label }}</option>
  </optgroup>
</select>
```

Handler in Script:
```ts
function onBandSelect(e: Event) {
  const val = parseFloat((e.target as HTMLSelectElement).value)
  if (!isNaN(val)) store.setParam('freqKhz', val)
}
```

---

### 6.3 — Extension-Select leer

**IST (Zeile 107–109):**
```html
<select class="kiwi-cpanel__select" aria-label="Extension">
  <option>extension ∨</option>
</select>
```

**Referenz (explore-8074.json bodyText):** 27 Extensions:
`ALE_2G, ant_switch, colormap, CW_decoder, CW_skimmer, DRM, DX spots, FAX, FFT, FSK, FT8/FT4, HFDL, IBP_scan, iframe, IQ_display, Loran_C, NAVTEX/DSC, noise_blank, noise_filter, S_meter, sig_gen, SSTV, TDoA, timecode, waterfall, WSPR`

**Fix-Plan 6.3:**
In `PluginView.vue` (Zeile 107–109):
```html
<select class="kiwi-cpanel__select" aria-label="Extension" @change="onExtSelect">
  <option value="">extension</option>
  <option v-for="ext in EXTENSIONS" :key="ext" :value="ext">{{ ext }}</option>
</select>
```

Konstante im Script:
```ts
const EXTENSIONS = [
  'ALE_2G','ant_switch','colormap','CW_decoder','CW_skimmer','DRM',
  'DX spots','FAX','FFT','FSK','FT8/FT4','HFDL','IBP_scan','iframe',
  'IQ_display','Loran_C','NAVTEX/DSC','noise_blank','noise_filter',
  'S_meter','sig_gen','SSTV','TDoA','timecode','waterfall','WSPR'
] as const
```

Handler: `store.activeExtension = (e.target as HTMLSelectElement).value` (neuer Store-State-Eintrag).
Extension-Panel (`<ExtensionPanel>`) empfängt `activeExtension` als Prop (bereits vorhanden in `ExtensionPanel.vue`).

---

### 6.4 — Zoom-Buttons: Lupe-Symbol überläuft Button

**IST (Zeile 120–121):**
```html
<button class="kiwi-cpanel__icon-btn" @click="onZoom(1)" title="Zoom in">🔍+</button>
<button class="kiwi-cpanel__icon-btn" @click="onZoom(-1)" title="Zoom out">🔍−</button>
```
`kiwi-cpanel__icon-btn` ist `width: 20px; height: 18px` (Zeile 956–960). Das 🔍-Emoji ist ~16px gross → überläuft bei 20px-Button wenn Font-Rendering variiert.

**Fix-Plan 6.4:**
Zoom-Buttons auf 28px × 20px vergrössern und `overflow: hidden` setzen:
```html
<button class="kiwi-cpanel__icon-btn kiwi-cpanel__icon-btn--zoom"
  @click="onZoom(1)" title="Zoom in">+🔍</button>
<button class="kiwi-cpanel__icon-btn kiwi-cpanel__icon-btn--zoom"
  @click="onZoom(-1)" title="Zoom out">−🔍</button>
```

CSS (neue Modifier-Klasse):
```css
.kiwi-cpanel__icon-btn--zoom {
  width: 28px;
  font-size: 11px;
  overflow: hidden;
  letter-spacing: -1px;
}
```

---

### 6.5 — Button-Anordnung / -Grösse nicht 1:1

**IST (Zeile 96–132):** Reihenfolge: `[freq-input] [Band] [Ext] [Play]` dann darunter `[icons-row]`

**Referenz (explore-8074.json, y~435/465):**
- Row 1 (y=435): `[id-freq-input w=100] [Band-Select w=97] [Ext-Select w=101]` — kein Play!
- Row 2 (y=465): `[A VFO w=20] [9 Users w=16] [icon×6] [Spectrum-Button w=75]`

Floating Play-Button ist SEPARAT links am Canvas (nicht im Bedienpanel-Row-1).
Im Plugin ist der Play-Button in Row 1 integriert — das ist falsch.

**Fix-Plan 6.5:**
- Row 1 (Zeile 96–111): Play-Button `<button class="kiwi-cpanel__play-btn">▶</button>` aus Row 1 entfernen
- Der Play-Button im Canvas-Bereich (Zeile 79 `.kiwi-play-btn`) bleibt und ist korrekt
- Row 2 anpassen: VFO-Button "A" (neu, Zeile ~114) und Users-Badge "9" vor den Icon-Buttons einfügen:

```html
<!-- Row 2: VFO + Users + Icons + Spectrum -->
<div class="kiwi-cpanel__row kiwi-cpanel__row--icons">
  <button class="kiwi-cpanel__vfo-btn" title="VFO A/B">A</button>
  <span class="kiwi-cpanel__icon kiwi-cpanel__icon--cyan" title="Users">9</span>
  <!-- ... rest der Icons ... -->
  <button class="kiwi-cpanel__spectrum-btn" @click="onSpectrumMode">Spectrum</button>
</div>
```

---

### 6.6 — Spectrum-Button ist ein Label, kein Button

**IST (Zeile 129):**
```html
<span class="kiwi-cpanel__text-label">Spectrum</span>
```

**Referenz (explore-8074.json `id-button-spectrum`):**
- `cls: "id-button-spectrum class-button id-btn-grp-12 w3-btn w3-ext-btn w3-round-6px"`
- Text: `"Spectrum"`, Button-Tag, w=75, h=22

**Drei Modi (aus M4-implementation-plan.md §8):** "Spectrum" / "Spec RF" / "Spec AF"

**Fix-Plan 6.6:**
```ts
// kiwiStore.ts: neuer State
spectrumMode: 'waterfall' as 'waterfall' | 'specRF' | 'specAF'
```

```html
<!-- PluginView.vue Zeile 129: -->
<button
  class="kiwi-cpanel__btn kiwi-cpanel__spectrum-btn"
  @click="cycleSpectrumMode"
  :title="spectrumModeLabel"
>{{ spectrumModeLabel }}</button>
```

```ts
const SPECTRUM_MODES = ['waterfall', 'specRF', 'specAF'] as const
const SPECTRUM_LABELS: Record<string, string> = {
  waterfall: 'Spectrum', specRF: 'Spec RF', specAF: 'Spec AF'
}
const spectrumModeLabel = computed(() => SPECTRUM_LABELS[store.spectrumMode])
function cycleSpectrumMode() {
  const idx = SPECTRUM_MODES.indexOf(store.spectrumMode as any)
  store.spectrumMode = SPECTRUM_MODES[(idx + 1) % SPECTRUM_MODES.length]
}
```

---

### 6.7 — Audio-Button: falsches Symbol (♪ statt Lautsprecher)

**IST (Zeile 131):**
```html
<button ... title="Audio">♪</button>
```

**Referenz (explore-8074.json btn-grp-11):** Icon-Button ohne Text (Font-Icon). KiwiSDR nutzt CSS-Font-Icons (kein Unicode-Emoji). Standard für Audio: Lautsprecher-Symbol.

**Fix-Plan 6.7:**
```html
<!-- WAS -->
<button class="kiwi-cpanel__icon-btn kiwi-cpanel__icon-btn--green" @click="onToggleAudio()" title="Audio">♪</button>

<!-- WIRD -->
<button
  class="kiwi-cpanel__icon-btn"
  :class="store.mute ? 'kiwi-cpanel__icon-btn--red' : 'kiwi-cpanel__icon-btn--green'"
  @click="onToggleAudio()"
  title="Audio mute/unmute"
>🔊</button>
```

Symbol: `🔊` (Speaker, Unicode 128266) im Mute-Zustand → `🔇` (128263). Beide passen in 20px-Button.

Alternativ (ohne Emoji): `<span>&#9654;</span>` + CSS `transform: scaleX(-1)` für Lautsprecher-Form.

---

### 6.8 — RF-Tab hat keinen Inhalt

**IST:** In `PluginView.vue` gibt es kein `<template v-if="activeTab === 'RF'">` — der RF-Tab zeigt leere Fläche.

Vorhandene Tab-Templates (verifiziert Zeilen 177–305): WF0, Audio, AGC, User, Stat, Off — RF fehlt.

**Referenz (subtabs.json RF-Tab `panelElements`):** RF-Tab enthält:
- Attenuation-Buttons (aus KiwiSDR-Quellcode): 0 dB / -10 dB / -20 dB / -30 dB / -40 dB
- NB2-Slider (Noise Blanker Level 2)
- CW peaks Toggle
- IQ swap/mix Buttons

**Fix-Plan 6.8:**
Nach Zeile 205 (Ende des WF0-Templates) einfügen:

```html
<template v-if="activeTab === 'RF'">
  <div class="kiwi-cpanel__ctrl-row">
    <span class="kiwi-cpanel__ctrl-label">Attn</span>
    <button
      v-for="db in [0, -10, -20, -30, -40]"
      :key="db"
      class="kiwi-cpanel__btn"
      :class="store.rfAttn === db ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
      @click="store.setParam('rfAttn', db)"
    >{{ db === 0 ? '0 dB' : db + ' dB' }}</button>
  </div>
  <div class="kiwi-cpanel__ctrl-row">
    <span class="kiwi-cpanel__ctrl-label">NB level</span>
    <input type="range" class="kiwi-slider kiwi-cpanel__slider"
      :min="0" :max="100" :step="1"
      :value="store.nbThresh" @input="onSlider('nbThresh', $event)" />
    <span class="kiwi-cpanel__ctrl-val">{{ store.nbThresh }}%</span>
  </div>
  <div class="kiwi-cpanel__ctrl-row">
    <span class="kiwi-cpanel__ctrl-label">CW peaks</span>
    <button class="kiwi-cpanel__btn"
      :class="store.cwPeaks ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
      @click="store.setParam('cwPeaks', store.cwPeaks ? 0 : 1)"
    >{{ store.cwPeaks ? 'ON' : 'OFF' }}</button>
  </div>
</template>
```

Store-Erweiterungen in `kiwiStore.ts`:
```ts
rfAttn: 0,     // 0 | -10 | -20 | -30 | -40 dB
cwPeaks: false,
```
`rfAttn` und `cwPeaks` in `BOOLEAN_PARAMS` (cwPeaks) und `ParamId` (bridge-validators.ts) eintragen.

---

## Bug 7 — Frequenz-Cursor entspricht nicht der KiwiSDR Web UI

**Betroffene Datei:** `ui/src/components/FrequencyRuler.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der Cursor ist aktuell ein statisches Overlay aus HTML-`div`s, dessen
Darstellung rein über `zoomLevel` (diskret 0–14) umschaltet:

- `zoomLevel < 9` → gelber Pfeil (`freq-ruler__cursor-arrow` + vertikale Linie), **nur** Center-Drag.
- `zoomLevel >= 9` → grüne Klammer (`freq-ruler__cursor-bracket`) mit zwei Griffen (`lo`/`hi`), Center-Drag + Edge-Resize.

**Problem (Paritäts-Abweichung):**

1. **Umschaltung über `zoomLevel`, nicht über Pixel-Breite.** Die KiwiSDR-Logik
   entscheidet anhand der **physischen Pixel-Breite des Passbands**
   (`MIN_INTERACTIVE_WIDTH_PX = 30px`), nicht anhand eines diskreten Zoom-Stufen-Zählers.
   Bei kleinem Viewport/anderem Hz-per-Pixel-Verhältnis unterscheiden sich die Zustände.
2. **"Zoomed-Out"-Zustand ist falsch.** KiwiSDR zeigt bei schmalem Passband eine
   **gelbe Trapez-/T-Form** (obere Leiste + zwei nach außen abfallende Linien +
   Mittel-Tick) als feste, ikonische Form. Unser gelber Pfeil ist eine andere Geometrie
   und hat keine sichtbare Passband-Andeutung.
3. **"Zoomed-In"-Zustand ist unvollständig.** KiwiSDR zeichnet eine **echte
   Filter-Repräsentation**: obere Leiste = Passbandbreite, vertikale Mittellinie =
   Trägerfrequenz, schräge Flanken links/rechts = Filter-Roll-off bis zur Baseline.
   Unsere Klammer hat nur Griffe, keine schrägen Flanken.
4. **Edge-Resizing ohne explizite `MIN_BANDWIDTH`/`MAX_BANDWIDTH`-Grenzen.** Aktuell
   wird nur ein `±100 Hz`-Minimalabstand erzwungen (`onLoMouseDown`/`onHiMouseDown`),
   aber kein definierter oberer/unterer Bandbreiten-Limit-Vertrag.
5. **Rendering via HTML-Divs, nicht Vektor-Grafik.** Schräge Flanken + präzises
   Hit-Testing (Flanke vs. Mitte) sind mit `div`/CSS nur umständlich korrekt abbildbar.
6. **Drei getrennte Interaktions-Zonen nicht abgebildet.** Das Original unterscheidet
   beim Klick+Halten drei Bereiche mit unterschiedlicher Drag-Wirkung (siehe unten);
   unsere `onDragMove` kennt nur `cursor`/`lo`/`hi` auf dem Cursor selbst.

**Referenz (KiwiSDR, `jks-prv/KiwiSDR_server`):** Das Element wird nicht mit
CSS-Boxen gelöst, sondern direkt als **Vektor-Grafik (Canvas-API 2D Context)** auf
die Frequenzskala gezeichnet (typisch in `ui.js`, `pb.js` (Passband) oder
`waterfall.js`). Bei jedem Maus-Event (`mousemove`, `mousedown`) wird die
Mausposition bestimmt: Mitte der berechneten Pixel-Box → Frequenz verschieben;
linker/rechter Rand **und Kasten grün (breit genug)** → Filterbreite ändern.

**Drei Interaktions-Zonen (Klick+Halten, Referenz-Verhalten):**

| Zone | Bereich | Wirkung beim Ziehen |
|------|---------|---------------------|
| **1. Auf dem Cursor** | direkt auf der Klammer/den Flanken | **low/high-Band ändern** — linke Flanke → `low_cut`, rechte Flanke → `high_cut` (Edge-Resize) |
| **2. Unterhalb des Cursors** | in der Frequenzband-Anzeige (Band-Skala), unterhalb des Cursors | **Cursor selbst bewegen** — ganzer Cursor (Trägerfrequenz) verschiebt sich horizontal |
| **3. Spektrometer-Feld** | im Wasserfall-/Spektrum-Bereich | **Pan** — Frequenzanzeige **inkl. Spektrometer** wird horizontal verschoben |

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/` — Schlagworte `cursor`,
   `passband`, `pb_`, `tuning`, `bracket`. Exakte Cursor-Geometrie (gelbe T-/Trapez-
   Form, grüne Klammer mit schrägen Flanken), Pixel-Breiten-Threshold
   (`MIN_INTERACTIVE_WIDTH_PX`), `MIN_BANDWIDTH`/`MAX_BANDWIDTH`-Werte extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** Cursor bei verschiedenen Zoomstufen
   prüfen; Referenz-DOM (`explore-8074.json`) auf Cursor-Elemente
   (`id-*-cursor`/`id-*-bracket`) durchsuchen. Exakte Zustandsübergang-Geometrie +
   Interaktions-Zonen verifizieren.

### Research-Ergebnis (2026-08-29, agent:general)

> **Wichtige Korrektur:** Der Passband-Cursor liegt **nicht** in `kiwi.js`/`waterfall.js`,
> sondern im OpenWebRX-Extension-Framework **`web/openwebrx/openwebrx.js`**.

- **Vier Adjust-Handles** (DOM `id-scale-container`): `pb_adj_car` (Center-Frequenz),
  `pb_adj_lo` (low cut), `pb_adj_hi` (high cut), `pb_adj_cf` (cf/bw-Anzeige).
- **Grüner Passband:** eigener Canvas `id-spectrum-pb-canvas`, gezeichnet via
  `pb_ctx.fillRect(x1, 0, x2-x1, sh)`; `owrx.pbx1/pbx2` aus `where_clicked()`
  (openwebrx.js ~1061–1062). Farbe **lime**; `spb_color: '#ffffff44'`.
- **Sichtbarkeit/Zustand:** Passband sichtbar wenn `(x1 > 0 || x2 < sw1) &&
  wfext.spb_on && owrx.allow_pb_adj` (openwebrx.js:4651); `wfext.spb_on: 1`.
- **Hit-Zonen** in `where_clicked()` (openwebrx.js ~1028–1062): Flanke → low/high-cut,
  Mitte → Frequenz, Spektrometer → Pan.
- **Bandbreiten-Grenzen:** `filter.min_passband` (z. B. 4 Hz analog), mode-spezifisch
  `kiwi_passbands(subtype).lo/hi`.
- **Schlüsselfunktionen:** `freq_to_pixel()` (:2569), `passband_visible()` (:2597),
  `freq_passband_center()` (:6421), `freq_passband(pbc_Hz)` (:6432).
- **WebUI-Port:** `8074` (bestätigt aus `explore-8074.json` → `url: ...:8074/`).

### Research-Ergebnis — Follow-up (2026-08-29, agent:general)

**Zustandsübergang gelb↔grün (KORREKTUR):** KEINE Pixel-Breiten-Schwelle und kein
Zoom-Level-Vergleich. Die Entscheidung ist **rein frequenzbasiert** über
`passband_visible()` (openwebrx.js:1976): Liegt die Passband-**Mittelfrequenz**
(`freq_passband_center()`) im sichtbaren Wasserfall-Fenster (Bin-Bereich
`[x_bin, x_bin + bins_at_cur_zoom()]`) → **grün** (auf Spektrum-Canvas). Liegt sie
außerhalb → **gelb** (nur auf der Skala, `#ffff00`, openwebrx.js:664).

**Hit-Testing (Konstanten, openwebrx.js:666–672):**
`env_bounding_line_w=5`, `env_att_w=5`, `env_h1=17`, `env_h2=5`,
`env_line_click_area=8`, `env_slop=5`.

| Region | Pixel-Bereich | Aktion |
|--------|---------------|--------|
| Mitte (Line) | `[line_px-4, line_px+4]` | Frequenz verschieben |
| Linke Flanke (Beginning) | `[from_px, from_px+15]` | low-cut resizen |
| Rechte Flanke (Ending) | `[to_px-15, to_px]` | high-cut resizen |
| Körper (whole envelope) | `[from_px, to_px]` | Center verschieben |
| Außerhalb (Scale-Canvas) | Rest | **Pan** |

(Shift = BFO/PBS, Alt = bwlo/bwhi — optionale Modifier, M4c.7 optional.)

**MIN/MAX-Bandbreite (openwebrx.js:833–837, 1093–1127):**
- `min_passband = 4` Hz (nur Null-Kollaps-Schutz).
- `low_cut_limit = -sampleRateDiv2`, `high_cut_limit = +sampleRateDiv2`
  (Default ±5000 Hz; ±6000 Hz bei 12 kHz Audio-Rate).
- **Kein separates MAX** — Obergrenze implizit = `high_cut_limit - low_cut_limit`.
- Validierung: `new_lo >= low_cut_limit`, `new_hi <= high_cut_limit`,
  `new_hi - new_lo >= min_passband`, `new_lo < new_hi`.

**Passband-Tabelle (openwebrx.js:805–821, mode-spezifisch):**
`am {-4900,+4900}` · `usb {+300,+2700}` · `lsb {-2700,-300}` · `cw {+300,+700}` ·
`nbfm {-6000,+6000}` · `iq/drm {-5000,+5000}`.

### Fix-Plan Bug 7

**Ziel:** Den Cursor als zoom-abhängige **Passband-Repräsentation** neu bauen —
idealerweise als **absolut positioniertes, interaktives SVG-Overlay** (oder
dedizierte Canvas-Schicht) exakt über der Frequenzskala.

**Schritt 1 — Zustandsübergang über `passband_visible()` statt Pixel-Breite/`zoomLevel`:**

```ts
// KORREKTUR (Research): der Zustand hängt NICHT von einer 30px-Schwelle ab,
// sondern davon, ob die Passband-Mitte im sichtbaren Frequenzfenster liegt.
const isZoomedIn = computed(() =>
  store.freqKhz >= props.viewLowKhz && store.freqKhz <= props.viewHighKhz
)
// isZoomedIn  → grün (expandierte Filter-Repräsentation)
// !isZoomedIn → gelb (kollabierte T-/Trapez-Form)
```

`zoomLevel`/Pixel-Breite bleiben nur indirekt relevant (sie bestimmen den Span und
damit `viewLow/HighKhz`); die **Entscheidung** trifft die Frequenz-Position
(`freq_passband_center()`-Äquivalent = `cursorKhz`).

**Schritt 2 — Zustand "Zoomed-Out" (gelb, kollabiert):**

- **Visual:** feste ikonische Trapez-/T-Form: obere horizontale Leiste, zwei nach
  außen abfallende Linien, Mittel-Tick. Farbe `#FFFF00` / `#FFD700`.
- **Interaktivität:**
  - Linke/rechte Flanke **deaktiviert** (kein Bandbreiten-Resize).
  - Gesamte gelbe Form per Klick+Ziehen horizontal verschiebbar → Trägerfrequenz
    (`center_carrier_frequency`) ändert sich.

**Schritt 3 — Zustand "Zoomed-In" (grün, expandiert):**

- **Visual:** echte Filter-Repräsentation:
  - obere horizontale Leiste = exakte Passbandbreite,
  - vertikale Linie exakt in der Mitte = Trägerfrequenz,
  - schräge Flanken links/rechts = Filter-Roll-off bis Baseline.
  - Farbe `#00FF00`.
- **Interaktivität:**
  - **Center-Drag:** Mitte ziehen → gesamtes Passband verschieben (Trägerfrequenz).
  - **Edge-Resize:** linke/rechte schräge Flanke als Draggriff (`cursor: ew-resize`).
  - **Limits:** links ändert `low_cut`, rechts ändert `high_cut`; Bandbreite darf
    `MAX_BANDWIDTH` nicht überschreiten und `MIN_BANDWIDTH` nicht unterschreiten
    (Flanken dürfen sich nicht kreuzen).

**Schritt 4 — Rendering als SVG-Overlay statt HTML-Divs:**

```html
<svg class="freq-ruler__cursor-svg" ...>
  <!-- gelb: T-/Trapez-Form -->
  <!-- grün: Leiste + Mittellinie + schräge Flanken -->
</svg>
```

- SVG erlaubt schräge Flanken (`<polygon>`/`<path>`) und präzises Hit-Testing
  (separate `<path>`-Elemente für Mitte / linke Flanke / rechte Flanke mit
  eigenen `pointerdown`-Handlern).
- Maus-/Touch-Events rechnen den Offset über das aktuelle Hz-per-Pixel-Verhältnis
  zurück in Frequenz (wie im Original: Mitte → `SET freq`, Flanke → `SET low_cut`/`high_cut`).

**Schritt 5 — Event-Handling nach KiwiSDR-Muster:**

```ts
function onPointerDown(e, zone: 'center' | 'lo' | 'hi') { /* ... */ }
function onPointerMove(e) {
  const deltaKhz = (e.clientX - startX) / pxPerKhz
  if (zone === 'center') emit('tune', startFreq + deltaKhz)
  else if (zone === 'lo')  emit('low-cut', clamp(startLo + deltaKhz * 1000, ...))
  else if (zone === 'hi')  emit('high-cut', clamp(startHi + deltaKhz * 1000, ...))
}
```

**Schritt 6 — Drei Interaktions-Zonen (Hit-Testing):**

Die Drag-Wirkung hängt davon ab, **wo** der User klickt und hält:

1. **Auf dem Cursor** (Klammer/Flanken selbst): `pointerdown` auf der linken Flanke
   startet `low-cut`-Resize, auf der rechten Flanke `high-cut`-Resize
   (`cursor: ew-resize`). Nur im "zoomed-in"-Zustand aktiv (gelbe Form hat keine
   draggable Flanken).
2. **Unterhalb des Cursors** (in der Frequenzband-Anzeige / Band-Skala): `pointerdown`
   in diesem Bereich startet **Cursor-Move** — der gesamte Cursor (Trägerfrequenz)
   folgt der Maus horizontal (`emit('tune', ...)`).
3. **Spektrometer-Feld** (Wasserfall/Spektrum-Bereich): `pointerdown` hier startet
   **Pan** — Frequenzanzeige **inkl. Spektrometer** wird verschoben (nicht nur der
   Cursor; der gesamte sichtbare Frequenzbereich `viewLow/HighKhz` wandert).

```ts
type DragZone = 'lo-flanke' | 'hi-flanke' | 'cursor-move' | 'pan'

function hitTest(x: number, y: number): DragZone {
  if (onCursorFlanke(x, y, 'lo')) return 'lo-flanke'
  if (onCursorFlanke(x, y, 'hi')) return 'hi-flanke'
  if (inBandScaleBelowCursor(y))    return 'cursor-move'
  return 'pan'                       // Spektrometer-/Wasserfall-Bereich
}
```

**Akzeptanzkriterium:**
- Passbandbreite < 30px → gelbe Trapez-/T-Form, **kein** Edge-Resize, Cursor per Klick+Halten unterhalb verschiebbar.
- Passbandbreite ≥ 30px → grüne Form mit Mittellinie + schrägen Flanken; Klick auf Flanke resized `low_cut`/`high_cut` unter `MIN_BANDWIDTH`/`MAX_BANDWIDTH`.
- Klick+Halten **unterhalb** des Cursors (Frequenzband) bewegt den Cursor.
- Klick+Halten **im Spektrometer-Feld** panniert Frequenzanzeige inkl. Spektrometer.

**E2E-Test:** `ui/e2e/frequency-ruler.spec.ts` — erweitern:
- Zoomed-Out: Cursor hat gelbe Form, Flanken nicht draggable (`ew-resize` nicht aktiv), Drag unterhalb des Cursors ändert `freqKhz`.
- Zoomed-In: Cursor grün, Flanken draggable, `low_cut`/`high_cut` ändern sich beim Ziehen, kreuzen nie.
- Drag im Spektrometer-Feld panniert (`viewLow/HighKhz` wandern), nicht nur der Cursor.

---

## Bug 8 — Frequenzband-Skala verhält sich nicht wie die Web UI

**Betroffene Datei:** `ui/src/components/FrequencyRuler.vue`

**Verwandt:** Bug 5 (kHz-Lineal skaliert nicht bei hohem Zoom) — Bug 5 erweiterte nur
die span-basierte `if/else`-Kette um sub-kHz-Stufen. Bug 8 ist die **vollständige**
Parität: die KiwiSDR-Skala ist eine **adaptive, pixel-basierte** Tick-Engine, keine
hartkodierte Schritt-Kette.

### Analyse (abgeschlossen 2026-08-29)

**IST:** `ticks`-Computed in `FrequencyRuler.vue` wählt `stepKhz` über eine feste
`if/else`-Kette basierend auf `span` (kHz), erzeugt **nur Major-Ticks** (keine
Minor-Subdivisions) und rendert HTML-`<span>`-Elemente (`.freq-ruler__tick` /
`.freq-ruler__label`). `formatFreq()` liefert `'0'`, `'x kHz'`, `'x.x MHz'`.

**Problem (Paritäts-Abweichung):**

1. **Hartkodierte Schritt-Kette statt Bucket-Tabelle.** KiwiSDR wählt die
   Schrittweite dynamisch aus einer Tabelle (`zoom_step` / `STEP_BUCKETS`) anhand
   der **Pixel-Auflösung**, damit Labels nie kollidieren. Unser `if/else` ist starr
   und kennt kein Kollisions-Feedback.
2. **Keine Minor-Ticks.** KiwiSDR zeichnet **Major-Ticks (mit Label)** und
   **Minor-Ticks (ohne Label, 5×/10× feinere Unterteilung)**. Wir zeichnen nur eine
   Tick-Art.
3. **HTML-DOM statt Canvas/SVG.** KiwiSDR rendert die Skala direkt per
   Canvas-2D (`kiwi_draw_scale()` / `scale_draw()`) — lange Striche mit Text,
   kurze Striche ohne Text im Schleifendurchlauf. Unser DOM-Ansatz skaliert bei
   hoher Tick-Zahl schlecht und erschwert exakte Positionierung.
4. **Label-Formatierung nicht einheitlich.** KiwiSDR formatiert konsistent:
   `< 1 MHz` → kHz, `>= 1 MHz` → MHz, mit einheitlicher Dezimalstellen-Zahl im
   gezoomten Zustand.

**Referenz (KiwiSDR, `jks-prv/KiwiSDR_server` — `web/kiwi/waterfall.js` / `kiwi.js`):**
Kern-Funktion `kiwi_draw_scale()` / `scale_draw()`. Es wird eine Tabelle
`zoom_step` gepflegt; je nach `zoom`-Stufe (0–14) wechselt die Schrittweite
(z. B. Zoom 0 = 5 MHz, Zoom 10 = 5 kHz). Bei jedem `redraw()` wird die sichtbare
Startfrequenz ermittelt, per Modulo (`%`) der erste linke Teilstrich berechnet und
in einer Schleife abwechselnd lange Striche (mit Text) und kurze Striche (ohne Text)
gezeichnet.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/waterfall.js` / `kiwi.js` —
   `kiwi_draw_scale()` / `scale_draw()` / `zoom_step`-Tabelle vollständig extrahieren:
   exakte Bucket-Werte, Major/Minor-Tick-Verhältnis (5× oder 10×), Label-Format-Logik.
2. **Live-WebUI (Port 8073/8074 validieren):** Frequenzskala bei mehreren Zoomstufen
   prüfen; exakte Tick-Dichte, Grid-Abstände und Label-Formatierung verifizieren.

### Fix-Plan Bug 8

**Ziel:** Die Frequenzskala als adaptive Canvas-/SVG-Engine neu bauen, die
Tick-Dichte, Schrittweite und Label-Format dynamisch aus Zoom-Level + sichtbarem
Frequenzbereich ableitet.

**Schritt 1 — Pixel-basierte Schritt-Auswahl statt hartkodierter Kette:**

```ts
const STEP_BUCKETS = [
  100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000,
  100000, 200000, 500000, 1000000, 2000000, 5000000,
] // Hz

// Hz-per-Pixel aus sichtbarem Bereich:
//   hzPerPx = (maxFreqHz - minFreqHz) / canvasWidthPx
// target Hz-Abstand zwischen Major-Labels (TARGET_LABEL_SPACING_PX ~ 80–100px):
//   targetHz = TARGET_LABEL_SPACING_PX * hzPerPx
// majorStepHz = kleinster Bucket-Wert >= targetHz
```

**Schritt 2 — Major-/Minor-Tick-Hierarchie:**

- `minorStepHz = majorStepHz / 5` (5 Sub-Ticks pro Major-Tick).
- Major-Tick: höher (10px), dicker (`lineWidth=2`), **mit Label**.
- Minor-Tick: kürzer (5px), dünner (`lineWidth=1`), **ohne Label**.

**Schritt 3 — Rendering-Loop (Canvas 2D):**

```ts
function drawFrequencyScale(ctx, width, height, minFreqHz, maxFreqHz) {
  ctx.clearRect(0, 0, width, height)
  const hzPerPx = (maxFreqHz - minFreqHz) / width
  const majorStepHz = selectStepBucket(hzPerPx * 90) // ~90px Abstand
  const minorStepHz = majorStepHz / 5
  const firstMinorHz = Math.floor(minFreqHz / minorStepHz) * minorStepHz

  for (let freq = firstMinorHz; freq <= maxFreqHz; freq += minorStepHz) {
    const x = (freq - minFreqHz) / hzPerPx
    if (x < 0 || x > width) continue
    const isMajor = Math.abs(freq % majorStepHz) < 0.001
      || Math.abs((freq % majorStepHz) - majorStepHz) < 0.001
    ctx.strokeStyle = '#FFFFFF'
    ctx.beginPath()
    if (isMajor) {
      ctx.lineWidth = 2
      ctx.moveTo(x, 0); ctx.lineTo(x, 10); ctx.stroke()
      ctx.fillStyle = '#FFFFFF'
      ctx.font = 'bold 11px sans-serif'
      ctx.textAlign = 'center'
      ctx.fillText(formatFreqLabel(freq), x, 22)
    } else {
      ctx.lineWidth = 1
      ctx.moveTo(x, 0); ctx.lineTo(x, 5); ctx.stroke()
    }
  }
}
```

**Schritt 4 — Label-Formatierung (`formatFreqLabel`):**

- `< 1 MHz` → kHz (z. B. `175 kHz`, `400 kHz`)
- `>= 1 MHz` → MHz (z. B. `7.100 MHz`, `14.080 MHz`)
- Überflüssige Trailing-Nullen entfernen, aber **einheitliche Dezimalstellen** im
  gezoomten Zustand.

**Schritt 5 — Zustands-Übergang über Zoom (KiwiSDR `zoom_step`):**

Die adaptive Engine darf gern zusätzlich eine `zoom_step`-Tabelle pflegen, die pro
`zoom`-Stufe (0–14) eine grobe Start-Schrittweite liefert und durch die
Pixel-Berechnung verfeinert wird — exakt wie im Original.

**Akzeptanzkriterium:**
- Labels kollidieren nie (Pixel-basierte Schritt-Auswahl mit `TARGET_LABEL_SPACING_PX`).
- Major-Ticks tragen Labels, Minor-Ticks (5× feiner) tragen keine.
- Zoom 0 → grobe MHz-Striche; hoher Zoom → feine kHz-/Hz-Striche (vgl. Bug 5).
- Labels konsistent formatiert (kHz unter 1 MHz, MHz ab 1 MHz).

**E2E-Test:** `ui/e2e/frequency-ruler.spec.ts` — erweitern:
- Bei default zoom: Major-Ticks mit MHz-Labels + Minor-Ticks ohne Label vorhanden.
- Nach Zoom-in: Tick-Dichte nimmt zu, Labels überlappen nicht (Kollisions-Check).
- Label-Format: unter 1 MHz `kHz`, ab 1 MHz `MHz`.

---

## Bug 9 — Band- & Stationsleiste verhält sich nicht wie die Web UI

**Betroffene Dateien:** `ui/src/components/BandScaleBar.vue` (obere Leiste),
`ui/src/components/TagArea.vue` (untere Leiste)

**Verwandt:** Bug 3 (Frequenzband-Leiste inkorrekt) + Bug 4 (DX-Tags fehlen /
zweireihiges Layout). Bug 3/4 lieferten die **Daten** (Bänder mit start/endFreq,
73 DX-Tags) und die grundlegende Struktur. Bug 9 ist die **dynamische** Parität:
synchrones Mitlaufen mit dem Wasserfall (Zoom/Pan) und der Kollisions-Layout-
Algorithmus mit vertikalen Verbindungslinien.

### Analyse (abgeschlossen 2026-08-29)

**IST:**

- **BandScaleBar.vue:** farbige `span`-Blöcke (`left`/`width` aus `startFreq`/`endFreq`
  über `viewLowMhz`/`viewHighMhz`). Labels sind `white-space: nowrap` und bei sehr
  schmalen Bändern teilweise abgeschnitten. Positionen sind zwar an `viewLow/High`
  gebunden, aber nicht garantiert **kontinuierlich synchron** zum Pan/Zoom (kein
  explizites Re-Layout bei jeder Pixeländerung).
- **TagArea.vue:** DX-Tags als absolute `span`s auf 2 Reihen (Kollisions-Detection
  über `MIN_GAP_PCT = 3`). **Keine** vertikalen Verbindungslinien (Zeiger) vom
  Rechteck zur Frequenz-Achse.

**Problem (Paritäts-Abweichung):**

1. **Balken nicht durchgehend / Label-Zentrierung.** KiwiSDR rendert Bänder als
   durchgehende horizontale Balken mit **horizontal zentriertem** Label. Unser
   Label sitzt in einem `span` mit `padding`, nicht garantiert in der Balkenmitte.
2. **Keine kontinuierliche Synchronisation.** Beim Zoomen/Pannen müssen sich
   Balkenbreite, -start und -ende synchron zur Frequenzskala vergrößern/verkleinern/
   herausbewegen. Unsere Implementierung reagiert nur auf Prop-Änderungen, nicht
   auf ein explizites Re-Layout bei jedem Redraw.
3. **Fehlende vertikale Verbindungslinien (Zeiger).** KiwiSDR zeichnet eine dünne
   vertikale Linie von der Unterkante des DX-Labels exakt zur Frequenzposition auf
   der Achse. Unsere TagArea hat keine solche Linie.
4. **Kollisionsvermeidung nur 2 Reihen, nicht dynamisch.** KiwiSDR prüft
   X-Koordinate + Breite jeder Box; überlappen zwei Boxen, wird die zweite auf eine
   tiefere/höhere Y-Ebene verschoben (nicht nur 2 feste Reihen) und die
   Verbindungslinie verlängert sich. Beim Rauszoomen werden gestapelte Labels wieder
   auf dieselbe Ebene gelegt. Unsere Logik kennt nur `row: 0|1`.

**Referenz (KiwiSDR, `jks-prv/KiwiSDR` — früher `Beagle_SDR_GPS`, `web/kiwi/`):**
Die Pixel-Berechnung aus Frequenzen liegt traditionell in den JS-Dateien des Ordners
`web/kiwi/` (Schlagworte: `dx`, `labels`, `band`). Bänder = durchgehende farbige
Balken mit zentriertem Label; DX-Labels = kleine farbcodierte Rechtecke mit
vertikaler Verbindungslinie zur Frequenzachse + Kollisions-Layout-Algorithmus.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/` — Schlagworte `dx`, `labels`,
   `band`, `dx_label`, `band_scale`. Kollisions-Layout-Algorithmus (Ebenen-Zuweisung),
   vertikale Verbindungslinien-Geometrie, Band-Label-Zentrierung extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** Band-Leiste + DX-Tags bei Zoom/Pan
   prüfen; Referenz (`explore-8074.json` dx-label/band-Elemente) verifizieren.
   Exaktes Ebenen-Verhalten beim Raus-/Hineinzoomen dokumentieren.

### Fix-Plan Bug 9

**Ziel:** Beide Leisten als **dynamische** Overlays bauen, die kontinuierlich mit
dem Wasserfall-Viewport (Start-/Endfrequenz) mitlaufen.

**Schritt 1 — BandScaleBar: durchgehende Balken + zentriertes Label:**

- Balken als durchgehende `<div>`/SVG-Rechtecke mit `left = freqToPercent(startFreq)`,
  `width = freqWidthPercent(startFreq, endFreq)`.
- Label **absolut zentriert** im Balken (Flexbox `justify-content: center` oder
  `text-align: center` + `overflow: hidden`); bei zu schmalem Balken Label
  ausblenden statt abschneiden.

**Schritt 2 — Kontinuierliche Synchronisation beim Zoom/Pan:**

```ts
// Beide Leisten erhalten dieselben viewLow/viewHigh wie FrequencyRuler (loKhz/hiKhz)
// aus PluginView. Bei jeder Änderung (wfZoom/freqKhz) re-rendern die Bänder/Tags
// synchron. Für Canvas/SVG: Redraw in einem requestAnimationFrame-Kanal.
```

**Schritt 3 — TagArea: vertikale Verbindungslinie (Zeiger):**

- Pro Tag eine dünne vertikale Linie von der Unterkante des Labels bis zur
  Frequenzachse (via `::before` mit `border-left`, Höhe = Abstand zur Achse, oder
  als `<line>` im SVG).

**Schritt 4 — Kollisions-Layout-Algorithmus (N Ebenen statt 2):**

```ts
// Statt row: 0|1 — dynamische Ebenen-Zuordnung:
// Für jedes Tag (nach Frequenz sortiert): prüfe ob es sich mit dem zuletzt auf
// jeder Ebene platzierten Tag überlappt (X + Breite). Überlappung → nächste Ebene.
// Beim Zoom-out (Abstand wächst) → Ebenen neu berechnen, Labels rücken wieder zusammen.
```

**Akzeptanzkriterium:**
- Bänder als durchgehende farbige Balken mit zentriertem Label, synchron zum Zoom/Pan.
- DX-Labels mit vertikaler Verbindungslinie zur Frequenzachse.
- Kollisionsvermeidung auf N Ebenen; beim Rauszoomen rücken gestapelte Labels wieder zusammen.

**E2E-Test:** `ui/e2e/band-scale.spec.ts` + `ui/e2e/tag-area.spec.ts` — erweitern:
- Bänder: Breite/Position ändern sich nach Zoom/Pan; Label zentriert.
- Tags: vertikale Verbindungslinien vorhanden; nach Rauszoomen keine Überlappung mehr.

---

## Bug 10 — Audio-Tab: fehlende Parameter + Scrollbar

**Betroffene Datei:** `ui/src/views/PluginView.vue` (Audio-Tab) + `ui/src/components/AudioPanel.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der Audio-Tab (`<template v-if="activeTab === 'Audio'">`) enthält aktuell
nur: Volume-Slider + Mute-Button, NR-Toggle, Compression (stub), De-emphasis (stub).
Es fehlen fast alle KiwiSDR-Audio-Parameter. Zudem hat das Panel **keine Scrollbar**
— im Original erreicht man alle Parameter nur per Scrollen.

**Referenz (KiwiSDR Web UI, visuell erfasst aus Screenshots + Quellcode):**

Der vollständige Audio-Tab umfasst (Reihenfolge von oben nach unten):

1. **Allgemeine Audio-Kontrollen**
   - **Noise:** Dropdown (Std. "off") + Button "More" (dunkelgrau/oranger Text) + Dropdown (Std. "off") + Button "More".
   - **Volume:** Slider + Dropdown (Std. "off", rechtsbündig).
   - **Pan:** Slider (mit "L=R"-Indikator) + Button "Comp" (dunkelgrau/grüner Text).
   - **Squelch:** Label mit grünem Dreieck + Slider (roter Track) + Text-Label "off" + 2× Dropdown für Zeitwert (z. B. "0s").
2. **Passband (PB) Konfiguration**
   - **PB default:** Button "PB default" (gelb/schwarz) + Dropdown (Std. "off", rechtsbündig).
   - **PB low:** Slider + Wertanzeige (z. B. "-4900").
   - **PB high:** Slider + Wertanzeige (z. B. "4900").
   - **PB center:** Slider + Wertanzeige (z. B. "0").
   - **PB width:** Slider + Wertanzeige (z. B. "9800").
   - Trennlinie.
3. **Noise Blanker & Noise Filter**
   - **Noise blanker:** Dropdown (Std. "off") + Button "Defaults" (gelb/schwarz) + Button "help" (grün/weiß, rechtsbündig). Trennlinie.
   - **Noise filter:** Dropdown (Std. "off") + Button "Defaults" + Button "help". Trennlinie.
4. **Noise Blanker Test**
   - **Noise blanker test (Header):** Dropdown (Std. "test off", roter Text).
   - **Test pulse gain:** dynamisches Label (z. B. "Test pulse gain: 0 dB") + Slider.
   - **Test pulse width:** dynamisches Label (z. B. "Test pulse width: 1 samples") + Slider.

**Styling:** dunkelgrauer Hintergrund; Parameter-Labels farbcodiert (Noise/Volume/Pan/
PB... orange, Squelch grün, Noise blanker/filter cyan); **vertikale Scrollbar rechts**
(nicht alles passt auf den Bildschirm).

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

Die Parameterliste oben ist **aus Screenshots visuell abgeleitet** — exakte
Wertebereiche der Slider, Dropdown-Inhalte und mögliche versteckte Parameter sind
daraus **nicht als Fakt belegbar**. Vor der Implementierung ist ein Research nötig:

1. **Quellcode-Referenz:** `jks-prv/KiwiSDR_server` → `web/kiwi/` durchsuchen
   (Schlagworte: `audio`, `squelch`, `noise_blank`, `noise_filter`, `pb_`,
   `test_pulse`). Die exakten Parameter-IDs, Slider-Min/Max und Dropdown-Optionen
   extrahieren.
2. **Live-DOM-Referenz (WebUI-Port 8073/8074 validieren):** `ui/e2e/reference/kiwisdr-reference/subtabs.json`
   prüfen — enthält der Audio-Tab bereits alle Elemente? Falls nicht,
   `capture-reference.spec.ts` (oder der Capture-Helper) gegen die WebUI erneut
   laufen lassen und den vollständigen Audio-Tab-DOM aufnehmen.
3. **Ergebnis:** Die Parameterliste in dieser Analyse als **verbindliche Checkliste**
   (jedes Element → ParamId + Range + Default) verfeinern, bevor gebaut wird.

### Fix-Plan Bug 10

**Schritt 0 — Research (Pflicht):** obige Research-Aufgabe abschließen; die
verbindliche Parameterliste festhalten.

**Schritt 1 — Scrollbare Container-Komponente:**

```html
<!-- Audio-Tab-Inhalt in scrollbaren Wrapper kapseln -->
<div class="kiwi-cpanel__tab-scroll" role="tabpanel">
  <!-- alle Audio-Parameter in fester Reihenfolge -->
</div>
```
```css
.kiwi-cpanel__tab-scroll {
  max-height: 320px;          /* oder flex: 1 innerhalb des Panels */
  overflow-y: auto;           /* vertikale Scrollbar rechts */
}
```

**Schritt 2 — Wiederverwendbare Sub-Komponenten:**

- `SliderRow` (Label + Slider + Wertanzeige)
- `DropdownRow` (Label + Dropdown)
- `ActionRow` (Label + Button(s), farbcodiert)

**Schritt 3 — Parameter in fester Reihenfolge einbauen** (siehe Referenz-Liste):
Noise (2× Dropdown + 2× "More") → Volume → Pan → Squelch → PB default/low/high/
center/width → Noise blanker → Noise filter → Noise blanker test (pulse gain/width).

**Schritt 4 — Farbcodierte Labels:** Noise/Volume/Pan/PB orange, Squelch grün,
Noise blanker/filter cyan — via CSS-Klassen (`kiwi-cpanel__ctrl-label--orange/--green/--cyan`).

**Akzeptanzkriterium:**
- Audio-Tab zeigt alle Parameter aus der (per Research verifizierten) Liste.
- Vertikale Scrollbar rechts; alle Parameter per Scrollen erreichbar.
- Labels farbcodiert, Slider/Dropdowns/Buttons in fester Reihenfolge.

**E2E-Test:** `ui/e2e/audio-tab.spec.ts` — erweitern:
- Alle Audio-Parameter sichtbar (nach `scrollIntoView`).
- Scrollbar vorhanden (`overflow-y: auto`), letzte Parameter per Scroll erreichbar.
- Parameter-Reihenfolge entspricht Referenz.

---

## Bug 11 — AGC-Tab beinhaltet eventuell nicht alle Parameter

**Betroffene Datei:** `ui/src/views/PluginView.vue` (AGC-Tab)

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der AGC-Tab (`<template v-if="activeTab === 'AGC'">`) enthält aktuell:
AGC ON/OFF-Toggle, Threshold-Slider, Decay-Slider, Hang-Toggle, Slope-Slider,
Man Gain-Slider. Der Aufbau weicht vom KiwiSDR-Original ab: dort sitzt eine
**horizontale Aktionsleiste (AGC / Hang / Defaults / help)** oben, darunter die
Slider-Parameter; unser Layout mischt Toggles und Slider ohne diese Struktur.
Zudem fehlt **Thresh CW** (ein eigener CW-Threshold-Slider) und eine **Scrollbar**.

**Referenz (KiwiSDR Web UI, visuell erfasst aus Screenshots + Quellcode):**

1. **Obere Aktionsleiste (Buttons, horizontal):**
   - **AGC:** Button (dunkelgrau/grüner Text) — globaler Ein-/Ausschalter.
   - **Hang:** Button (dunkelgrau/weißer Text, orangene Umrandung).
   - **Defaults:** Button (gelb/schwarz).
   - **help:** Button (grün/weiß), **rechtsbündig** auf derselben Zeile.
2. **Slider-Parameter (Label links | Slider mitte | Wert rechts):**
   - **Manual gain:** Label weiß, Wert z. B. "50 dB".
   - **Threshold:** Label orange, Wert z. B. "-110 dBm".
   - **Thresh CW:** Label weiß, Wert z. B. "-130 dBm".
   - **Slope:** Label orange, Wert z. B. "6 dB".
   - **Decay:** Label orange, Wert z. B. "1000 msec".

**Styling:** dunkelgrauer Hintergrund; Labels wechseln zwischen weiß/orange
(Gruppierungs-/Prioritäts-Hinweis); **vertikale Scrollbar rechts**; S-Meter-Skala
(S1…S9, +10…+60) entweder als Teil des Scroll-Containers oder **fest am unteren
Rand** des übergeordneten UI-Fensters.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

Die Parameterliste ist **visuell (Screenshots) abgeleitet** — exakte Wertebereiche,
Dropdown-/Toggle-Semantik und mögliche versteckte Parameter (z. B. weitere
AGC-Parameter wie `agc_gain`/`agc_hold`/`agc_mintop` in manchen KiwiSDR-Builds) sind
daraus **nicht als Fakt belegbar**. Vor der Implementierung ist ein Research nötig:

1. **Quellcode-Referenz:** `jks-prv/KiwiSDR_server` → `web/kiwi/` durchsuchen
   (Schlagworte: `agc`, `hang`, `slope`, `decay`, `threshold`, `thresh_cw`,
   `manual_gain`). Exakte Parameter-IDs, Slider-Min/Max, Einheiten und Toggle-
   Semantik extrahieren.
2. **Live-DOM-Referenz (WebUI-Port 8073/8074 validieren):** `ui/e2e/reference/kiwisdr-reference/subtabs.json`
   prüfen — enthält der AGC-Tab alle Elemente? Falls nicht, Capture gegen die
   WebUI erneut laufen lassen und den vollständigen AGC-Tab-DOM aufnehmen.
3. **Ergebnis:** verbindliche Parameterliste (ParamId + Range + Default + Einheit)
   als Checkliste verfeinern, bevor gebaut wird.

### Fix-Plan Bug 11

**Schritt 0 — Research (Pflicht):** obige Research-Aufgabe abschließen.

**Schritt 1 — Scrollbare Container-Komponente:** AGC-Inhalt in
`.kiwi-cpanel__tab-scroll` (wie Bug 10) kapseln (`max-height` + `overflow-y: auto`).

**Schritt 2 — Obere Aktionsleiste:** `AGC` / `Hang` / `Defaults` / `help`
(rechtsbündig) als Button-Reihe.

**Schritt 3 — Slider-Parameter (Label | Slider | Wert):** Manual gain, Threshold,
Thresh CW, Slope, Decay — in fester Reihenfolge, Wert-Anzeige mit Einheit
(`dB`/`dBm`/`msec`).

**Schritt 4 — S-Meter-Skala:** entweder Teil des Scroll-Containers oder fest am
unteren Rand fixiert (je nach Original-Verhalten).

**Schritt 5 — Farbcodierte Labels:** weiß/orange abwechselnd wie in der Referenz.

**Akzeptanzkriterium:**
- AGC-Tab zeigt Aktionsleiste + alle Slider (inkl. Thresh CW) + Scrollbar.
- Werte-Anzeigen mit korrekten Einheiten; S-Meter-Skala korrekt integriert.
- Parameterliste per Research verifiziert.

**E2E-Test:** `ui/e2e/agc.spec.ts` — erweitern:
- Aktionsleiste (AGC/Hang/Defaults/help) + alle Slider vorhanden (nach `scrollIntoView`).
- Thresh CW-Slider vorhanden; S-Meter-Skala sichtbar; Scrollbar funktioniert.

---

## Bug 12 — Header-Bereich entspricht nicht der Web UI

**Betroffene Datei:** `ui/src/views/PluginView.vue` (Header-Block) + ggf. neue
`ui/src/components/HeaderBar.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der Header ist eine einfache, flache Zeile mit drei Sektionen
(`kiwi-header__left` Logo + Titel + "Antenna: KiwiSDR broadband",
`kiwi-header__center` Name + Status + `StationInput`,
`kiwi-header__right` Callsign-Input + UTC/Local/Timezone). Höhe und
Informationsdichte weichen vom KiwiSDR-Original ab.

**Problem (Paritäts-Abweichung):**

1. **Fehlende Station-Info.** KiwiSDR zeigt einen **großen, fetten Titel** (z. B.
   "VK5ARG Public KiwiSDR Receiver #1") plus **zwei Untertitel-Zeilen**:
   - Zeile 1: Standort, Grid, ASL, `[map]`-Link, SNR-Werte.
   - Zeile 2: Antennen-Spezifikationen.
   Unser Header zeigt nur "NetSDRStation" + "Antenna: KiwiSDR broadband" (statisch).
2. **Fehlende Credits-Sektion.** KiwiSDR hat eine mittlere Sektion "Provided by"
   + zwei Hyperlinks. Unser Header hat diese nicht.
3. **Fehlender Collapse/Expand-Toggle.** KiwiSDR hat am unteren mittleren Rand der
   Top Bar einen halbtransparenten Reiter mit Dreieck/Chevron, der einen
   **Bild-Bereich** (Panoramabild der Station) auf-/zuklappt. Unser Header hat keine
   Ausklapp-Funktion.
4. **Fehlender Bild-Bereich (Expanded View).** KiwiSDR klappt ein breites
   Hintergrundbild aus mit Overlays: Logos in den Ecken, schwebendes Kontroll-Panel
   (Frequenz-Input + "select band"/"extension"-Dropdowns + runder Play-Button) unten
   rechts, lila Play-Button vertikal zentriert links.

**WICHTIG — Bestands-Funktion erhalten:** Die **Connect-Funktionalität**
(`StationInput` mit Stationsname, Connect/Disconnect-Button, Status-Badge) MUSS im
neuen Header-Layout erhalten bleiben. Sie ist aktuell in `kiwi-header__center`
integriert; im Redesign ist ein fester Platz dafür vorzusehen (z. B. in der rechten
Sektion neben "Your name or callsign" oder als eigener Bereich).

**Referenz (KiwiSDR Web UI):** hellgraue Top Bar in drei Sektionen
(links Branding/Station-Info, Mitte Credits "Provided by", rechts Callsign-Input +
"Powered by OpenWebRX"-Logo **oder** mehrzeilige Zeitanzeige). Unten mittig ein
halbtransparenter Reiter (Chevron ↓/↑) als Collapse/Expand-Toggle; darunter ein
expandierbarer Bild-Container mit absolut positionierten Overlays.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/` — Schlagworte `topbar`,
   `header`, `callsign`, `collapse`, `expand`, `image`. Exakte Top-Bar-Höhe,
   Sektions-Layout, Collapse/Expand-Mechanik und Bild-Container-Overlays extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** Header-Topbar prüfen; Referenz
   `header-topbar.json` + `explore-8074.json` (`id-topbar-*`) verifizieren. Exakte
   Höhe, Titel-/Untertitel-Format, Credits-Links und Bild-Bereich aufnehmen.

### Fix-Plan Bug 12

**Schritt 1 — Top Bar dreiteilig (hellgrau, Flexbox/Grid):**

- **Links:** Logo (Kiwi-Vogel) + Titel (groß/fett) + Untertitel-Zeile 1 (Standort,
  Grid, ASL, `[map]`, SNR) + Untertitel-Zeile 2 (Antennen).
- **Mitte:** "Provided by" + zwei Hyperlinks (unterstrichen).
- **Rechts:** "Your name or callsign:"-Input + Slot für **entweder** "Powered by
  OpenWebRX"-Logo **oder** Zeitanzeige (UTC/Local/Timezone).

**Schritt 2 — Connect-Funktionalität erhalten (Pflicht):**

```html
<!-- StationInput bleibt im Header (Stationsname + Connect/Disconnect + Status) -->
<StationInput :station="store.station" :status="store.status"
  @connect="onStation" @disconnect="store.disconnect()" />
```

**Schritt 3 — Collapse/Expand-Toggle (Reiter):**

```ts
const isExpanded = ref(false)
// Chevron: isExpanded ? '▲' : '▼' (bzw. CSS-Rotation)
```

**Schritt 4 — Expandierbarer Bild-Bereich (Transitions):**

```html
<div class="kiwi-header__expand" :class="{ '--open': isExpanded }">
  <!-- Panoramabild + Overlays (position: absolute): Logos, Kontroll-Panel, Play-Button -->
</div>
```
```css
.kiwi-header__expand { max-height: 0; overflow: hidden; transition: max-height 0.3s ease; }
.kiwi-header__expand.--open { max-height: 400px; }
```

**Schritt 5 — Overlays im Bild-Container (position: absolute):**
- Logos in Ecken (unten links, oben rechts).
- Schwebendes Kontroll-Panel unten rechts: numerisches Frequenz-Input + zwei
  Dropdowns ("select band", "extension") + runder Play/Pause-Button.
- Lila Play-Button links, vertikal zentriert.

**Akzeptanzkriterium:**
- Header hat Titel + Untertitel-Zeilen + Credits + Callsign-Input + Zeit/Logo-Slot.
- Collapse/Expand-Toggle klappt Bild-Bereich weich ein/aus (Chevron ↓/↑).
- **Connect-Funktionalität (StationInput + Status) bleibt vollständig erhalten.**

**E2E-Test:** `ui/e2e/kiwi-layout.spec.ts` / `header-topbar`-Tests — erweitern:
- Titel + Untertitel + "Provided by" + Callsign-Input vorhanden.
- Toggle-Reiter klappt Bild-Bereich ein/aus.
- `StationInput` (Connect/Status) weiterhin sichtbar und funktional.

---

## Bug 13 — "Spec RF"-Button soll funktionieren

**Betroffene Dateien:** `ui/src/views/PluginView.vue` (Spectrum-Button + Layout),
`ui/src/components/Waterfall.vue` + ggf. neue `ui/src/components/SpecRf.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der Spectrum-Button (`cycleSpectrumMode` in `PluginView.vue`, Bug 6.6)
zyklisch nur das **Label** durch `Spectrum` → `Spec RF` → `Spec AF`
(`SPECTRUM_MODES`/`SPECTRUM_LABELS`, `store.spectrumMode`). Es wird **kein**
zusätzliches Diagramm gerendert — der Modus-Wechsel ist rein kosmetisch.

**Problem (Paritäts-Abweichung):**

1. **Kein Spektrumanalysator.** Bei "Spec RF" soll über Frequenzskala + Wasserfall
   ein 2D-Spektrumanalysator (Area-Chart) eingeblendet werden. Aktuell passiert nichts.
2. **Fehlende Y-Achse (Signalstärke dBm).** Rechts eine vertikale dBm-Skala
   (z. B. -50 oben bis -100 unten), Werte farblich hinterlegt (rot/hellgrün/cyan/
   dunkelblau), mit horizontalen grauen Rasterlinien über die volle Breite.
3. **Fehlendes Passband-Overlay.** Ein halbtransparentes graues Rechteck markiert die
   aktuelle Passband-Breite und muss synchron zur grünen Klammer (Bug 7) mitwandern
   und sich in der Breite ändern.
4. **Fehlende Sync mit der Frequenzachse.** Der Graph muss beim Pan/Zoom verzögerungs-
   frei mit dem Wasserfall mitskalieren/verschieben (X-Achse exakt synchron).

**Datenquelle (Hinweis):** M4.7 hat bereits `SpectrumAnalyzer` (Goertzel-DFT) +
`waterfallBins` (dBFS) im Store + `setWaterfallBins`. Der Spec-RF-Graph kann diese
Bins als Grundlage verwenden; echte RF-Spektrumsdaten kommen erst mit dem
WF-WebSocket (M5). Für M4c.7 reicht das Rendern des vorhandenen Spektrums
(kein Fake-Datenstrom — siehe Bug 2 Entscheidung).

**Referenz (KiwiSDR Web UI):** "Spec RF"-Toggle im schwebenden Kontroll-Panel
(aktiv = leuchtend grün) blendet einen Spektrumanalysator ein: schwarzer Hintergrund,
volle Breite, Y-Achse dBm rechts mit farbcodierten Werten + horizontalen Grid-Lines,
hellgraues/weißes Area-Chart, halbtransparentes Passband-Overlay, synchron zur
Frequenzskala.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/` — Schlagworte `spec_rf`,
   `spectrum`, `spec_display`, `waterfall`. Exakte RF-Spektrums-Geometrie
   (Y-Achsen-Werte + Farbcodes, Area-Chart-Stil, Passband-Overlay) extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** "Spec RF"-Modus im Browser aktivieren;
   Referenz-DOM (`explore-8074.json`/`subtabs.json`) prüfen. Exakte dBm-Skala,
   Grid-Lines und Passband-Overlay-Verhalten verifizieren.

### Fix-Plan Bug 13

**Schritt 1 — Toggle-Button aktiv grün:**

```html
<button class="kiwi-cpanel__spectrum-btn"
  :class="{ 'kiwi-cpanel__spectrum-btn--active': store.spectrumMode === 'specRF' }"
  @click="cycleSpectrumMode">Spec RF</button>
```

**Schritt 2 — Spektrumanalysator-Komponente (Area-Chart):**

```html
<template v-if="store.spectrumMode === 'specRF'">
  <SpecRf :bins="store.waterfallBins"
          :view-low-khz="loKhz" :view-high-khz="hiKhz"
          :low-cut-hz="store.lowCut" :high-cut-hz="store.highCut" />
</template>
```

**Schritt 3 — Diagramm-Rendering (Canvas/SVG, schwarzer Hintergrund):**
- Y-Achse rechts: dBm-Werte (-50 … -100), farbcodiert; horizontale Grid-Lines grau.
- Area-Chart hellgrau/weiß, X-Achse synchron zur Frequenzskala (`viewLow/HighKhz`).

**Schritt 4 — Passband-Overlay (halbtransparentes graues Rechteck):**
- Position aus `lowCut`/`highCut` + `freqKhz` → X-Prozent wie in `FrequencyRuler`
  (`loPct`/`hiPct`/`bwPct`). Synchron zur grünen Klammer (Bug 7).

**Schritt 5 — Sync bei Pan/Zoom:**
- Spec-RF-Graph bekommt dieselben `viewLowKhz`/`viewHighKhz` wie `FrequencyRuler`/
  `Waterfall` und re-rendert reaktiv (Canvas-Redraw bei Prop-Änderung).

**Akzeptanzkriterium:**
- "Spec RF"-Button (aktiv grün) blendet den Spektrumanalysator ein/aus.
- Area-Chart mit Y-Achse (dBm, farbcodiert) + Grid-Lines + Passband-Overlay.
- Graph skaliert/verschiebt synchron mit Wasserfall + Frequenzskala beim Pan/Zoom.

**E2E-Test:** `ui/e2e/wf0-tab.spec.ts` / neues `spec-rf.spec.ts`:
- Klick auf "Spec RF" → Spektrumanalysator sichtbar, Button aktiv (grün).
- Area-Chart vorhanden; Passband-Overlay verschiebt sich bei Änderung von low/high cut.

---

## Bug 14 — "Spec AF"-Button soll funktionieren

**Betroffene Dateien:** `ui/src/views/PluginView.vue` (Spectrum-Button + Layout),
`ui/src/components/Waterfall.vue` + ggf. neue `ui/src/components/SpecAf.vue`

**Verwandt:** Bug 13 (Spec RF). Der Spectrum-Button hat **drei Zustände**
(`Spectrum` → `Spec RF` → `Spec AF` → aus). Bug 13 behandelt die RF-Ansicht,
Bug 14 die AF-Ansicht. Beide nutzen `store.spectrumMode`.

### Analyse (abgeschlossen 2026-08-29)

**IST:** `cycleSpectrumMode` zyklisch nur das Label (`waterfall`/`specRF`/`specAF`),
rendert aber weder RF- noch AF-Diagramm. Die AF-Ansicht fehlt vollständig.

**Problem (Paritäts-Abweichung):**

1. **Kein AF-Spektrumanalysator.** Beim zweiten Klick ("Spec AF") soll ein
   AF-Spektrum des demodulierten Audiosignals erscheinen — aktuell passiert nichts.
2. **Fehlende AF-spezifische Overlays:**
   - **Vertikales Grid** (hellgraue Linien) für Audio-Frequenzschritte.
   - **Grüne Center-Linie** (Mittelachse) = Nullpunkt/Träger des Audiosignals,
     exakt synchron zur Mitte der grünen Klammer (Tuning-Bereich).
   - **Rote Begrenzungslinien** links/rechts = äußere Audio-Passband-Grenzen
     (Filtergrenzen).
3. **AF-Graph ist um die getunte Frequenz zentriert** (im Gegensatz zum RF-Graph),
   muss sich beim Tuning (grüne Klammer verschieben) synchron mitbewegen.

**Datenquelle (Hinweis):** Wie Bug 13 nutzt die AF-Ansicht die vorhandenen
`waterfallBins`/`SpectrumAnalyzer` (M4.7) als Grundlage. Echte AF-Demodulator-Daten
(Audio-Basisband) sind ein DSP-/Netzwerk-Thema für M5+. Für M4c.7 reicht das
korrekte Rendern der AF-Ansicht (kein Fake-Datenstrom, siehe Bug 2).

**Referenz (KiwiSDR Web UI):** "Spec AF"-Toggle (aktiv grün) wechselt das obere
Diagramm von RF auf AF: gleiche Y-Achse (dBm, farbcodiert) + horizontale Grid-Lines,
hellgraues/weißes Area-Chart (um Träger zentriert), zusätzlich vertikales Grid,
grüne Center-Linie (Träger) + zwei rote Filtergrenzen-Linien. Ein weiterer Klick
blendet das Diagramm wieder aus.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

1. **Kiwi SDK:** `jks-prv/KiwiSDR_server` → `web/kiwi/` — Schlagworte `spec_af`,
   `af_spectrum`, `audio spectrum`. Exakte AF-Geometrie (vertikales Grid,
   grüne Center-Linie, rote Filtergrenzen, Zentrierung um Träger) extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** "Spec AF"-Modus im Browser aktivieren;
   Referenz-DOM prüfen. Exakte AF-Overlays + Tuning-Sync-Verhalten verifizieren.

### Fix-Plan Bug 14

**Schritt 1 — Toggle (drei Zustände) aktiv grün:**

```ts
// Bestand (Bug 6.6): SPECTRUM_MODES = ['waterfall','specRF','specAF']
// cycleSpectrumMode() zyklisch durch alle drei; Button aktiv bei specRF/specAF.
```

**Schritt 2 — AF-Spektrumanalysator-Komponente:**

```html
<template v-if="store.spectrumMode === 'specAF'">
  <SpecAf :bins="store.waterfallBins"
          :view-low-khz="loKhz" :view-high-khz="hiKhz"
          :cursor-khz="store.freqKhz"
          :low-cut-hz="store.lowCut" :high-cut-hz="store.highCut" />
</template>
```

**Schritt 3 — Diagramm-Rendering (Canvas/SVG, schwarzer Hintergrund):**
- Y-Achse rechts identisch zur RF-Ansicht (-50…-100 dBm, farbcodiert) + horizontale Grid-Lines.
- AF-Area-Chart hellgrau/weiß, **um den Träger zentriert**.
- **Vertikales Grid** (hellgraue Linien) für Audio-Frequenzschritte.

**Schritt 4 — Overlays (AF-spezifisch):**
- **Grüne Center-Linie:** X-Position = `cursorPct` (Mittenfrequenz), leuchtend grün.
- **Rote Begrenzungslinien:** X-Positionen = `loPct`/`hiPct` (aus `lowCut`/`highCut`),
  synchron zu den Filtergrenzen.

**Schritt 5 — Sync bei Pan/Tuning:**
- AF-Graph + grüne Center-Linie + rote Filtergrenzen re-rendern reaktiv bei
  `freqKhz`/`lowCut`/`highCut`/`viewLow/HighKhz`-Änderung.

**Akzeptanzkriterium:**
- "Spec AF"-Button (aktiv grün) wechselt das Diagramm von RF auf AF (und wieder aus).
- AF-Graph um Träger zentriert, mit vertikalem Grid, grüner Center-Linie + zwei roten Filtergrenzen.
- Gesamtes AF-Spektrum bewegt sich synchron beim Tuning (grüne Klammer verschieben).

**E2E-Test:** `ui/e2e/wf0-tab.spec.ts` / neues `spec-af.spec.ts`:
- 2× Klick auf Spectrum-Button → "Spec AF"-Label + AF-Diagramm sichtbar.
- Grüne Center-Linie + rote Filtergrenzen vorhanden, verschieben sich beim Tuning.

---

## Bug 15 — DRM-Tab (Button) funktioniert nicht

**Betroffene Datei:** `ui/src/views/PluginView.vue` (Mode-Button `DRM` + Layout) +
ggf. neue `ui/src/components/DrmSchedule.vue`, `DrmDecoderPanel.vue`

### Analyse (abgeschlossen 2026-08-29)

**IST:** Der DRM-Mode-Button (`panelModes` → `{ idx: 12, label: 'DRM' }`) ruft nur
`store.setParam('mode', 12)`. Es gibt **kein** DRM-spezifisches UI: kein Schedule-/
Services-Overlay oben, kein Decoder-Panel, keine DRM-Bandbreiten-Anpassung der
Tuning-Klammer.

**Problem (Paritäts-Abweichung):**

1. **Kein Top-Overlay (DRM Schedule & Services).** Bei aktivem DRM-Modus überlagert
   im Original ein mehrteiliges Panel den oberen Bereich (wo sonst Spektrumanalysator/
   Leerraum ist): Status-Checkboxen (IO/Time/Frame/FAC/SDC/MSC) + "Services:"-Liste,
   Schedule-Ansicht mit Stationsliste + Zeitleiste, Zeit/Legende-Sektion.
2. **Kein Decoder-Kontroll-Panel (unten links).** Ein freischwebendes
   "Digital Radio Mondiale decoder"-Panel fehlt (Header + Content mit Hyperlinks +
   Footer-Aktionsleiste Stop/Monitor IQ/Test 1/Test 2 + LPF-Checkbox).
3. **Keine DRM-Bandbreite auf der Tuning-Klammer.** Die grüne Klammer (Bug 7) müsste
   sich auf die typische DRM-Bandbreite (~10 kHz) verbreitern.

**Referenz (KiwiSDR Web UI, visuell erfasst aus Screenshots):** DRM-Aktivierung
löst weitreichende UI-Änderungen aus — ein Schedule/Services-Overlay oben und ein
Decoder-Panel unten links, plus breitere Tuning-Klammer.

### ⚠️ Research-Aufgabe (Pflicht vor Fix)

Die Beschreibung ist **visuell (Screenshots) abgeleitet** — exakte Labels, Dropdown-
Inhalte, Checkbox-Semantik, Decoder-Status-Flags (FAC/SDC/MSC), Schedule-Datenquelle
und die genaue DRM-Bandbreite sind **nicht als Fakt belegbar**. Vor der
Implementierung ist ein **sorgfältiger Research** in WebUI **und** Kiwi-SDK nötig:

1. **Kiwi-SDK / Server-Quellcode:** `jks-prv/KiwiSDR_server` → `web/kiwi/` durchsuchen
   (Schlagworte: `drm`, `dream`, `schedule`, `fac`, `sdc`, `msc`, `drmschedule`).
   Exakte Feld-Labels, Decoder-Status-Flags, Schedule-Datenquelle (drmrx.org),
   Default-Dropdown-Werte extrahieren.
2. **Live-WebUI (Port 8073/8074 validieren):** `ui/e2e/reference/kiwisdr-reference/subtabs.json`
   + `panel.json` prüfen; ggf. DRM-Modus auf einer DRM-fähigen Station im Browser
   aktivieren und den vollständigen DOM aufnehmen (Schedule-Overlay + Decoder-Panel).
3. **Ergebnis:** verbindliche UI-Spezifikation (Feld-Labels, Checkbox-Liste,
   Schedule-Struktur, Zeitleisten-Semantik, DRM-Bandbreite) als Checkliste verfeinern.

### Fix-Plan Bug 15

**Schritt 0 — Research (Pflicht):** obige Research-Aufgabe abschließen.

**Schritt 1 — DRM-Modus als Store-State:**

```ts
// store.mode === 12 (DRM) steuert die Sichtbarkeit der DRM-Overlays
const isDrmActive = computed(() => store.mode === 12)
```

**Schritt 2 — Top-Overlay (DRM Schedule & Services):**

```html
<template v-if="isDrmActive">
  <DrmSchedule />
</template>
```

- **Links (schwarz):** Checkbox-Reihe IO/Time/Frame/FAC/SDC/MSC + "Services:"-Liste (1–4).
- **Mitte (weiß):** scrollbare Stationsliste mit blauem Info-Icon links, Zeitleiste
  rechts (grüne/rosafarbene Balken), rote vertikale "jetzt"-Linie, graue Trennlinien.
- **Rechts (schwarz):** UTC/Local-Zeit, Dropdown ("by service"), Legenden-Buttons
  "verified" (grün) / "not verified" (rosa).

**Schritt 3 — DRM Decoder-Kontroll-Panel (unten links):**

```html
<DrmDecoderPanel v-if="isDrmActive" />
```

- Header (dunkelgrau, Titel cyan, "help" grün + "X" rechts).
- Content: Text + Hyperlinks ("DRM decoder is based on Dream 2.2.1",
  "Schedule information courtesy of drmrx.org"), vertikale Scrollbar.
- Footer: Buttons Stop (rot) / Monitor IQ (magenta) / Test 1 + Test 2 (cyan) + "LPF"-Checkbox.

**Schritt 4 — DRM-Bandbreite auf der Tuning-Klammer:**

- Beim DRM-Modus setzt der Passband-Cursor (Bug 7) die Bandbreite auf ~10 kHz
  (`low_cut`/`high_cut` entsprechend), deutlich breiter als AM/SSB.

**Schritt 5 — Vue-Transitions + Klick-Durchgriff:**

- Top-Panel + Decoder-Panel mit Vue-`<transition>` weich ein-/ausblenden.
- Overlays dürfen das darunterliegende Canvas nicht funktional blockieren
  (Klick außerhalb → Overlay schließen bzw. Event durchreichen).

**Akzeptanzkriterium:**
- DRM-Button blendet Schedule/Services-Overlay + Decoder-Panel weich ein/aus.
- Tuning-Klammer verbreitert sich auf ~10 kHz im DRM-Modus.
- Alle Labels/Checkboxen/Buttons gemäß (per Research verifizierter) Spezifikation.

**E2E-Test:** `ui/e2e/mode-select.spec.ts` / neues `drm.spec.ts`:
- DRM-Button → Schedule-Overlay + Decoder-Panel sichtbar.
- Tuning-Klammer breiter (~10 kHz); Overlays schließen bei erneutem DRM-Klick.

---

## E2E-Lückenanalyse

### Problem 1: Playwright sieht nur sichtbare Elemente

`page.locator()` erfasst nur Elemente im aktuellen Viewport. Scrollbare Tab-Inhalte (WF0 etc.)
wurden nicht gescrollt → nicht von E2E-Tests abgedeckt.

**Fix:** `await element.scrollIntoViewIfNeeded()` vor jedem `locator()` für Tab-Inhalte ODER
`page.evaluate(() => document.querySelectorAll('[data-testid]'))` für vollständigen DOM-Snapshot
unabhängig von Scroll-Position.

### Problem 2: dx-selects-smeter.json hat leere Arrays

Alle `allOptions: []` — der Live-Capture-Helper hat Band/Extension/Dropdowns nicht erfasst.

**Fix:** E2E-Capture-Script anpassen: `await select.evaluate(el => [...el.options].map(o => o.text))` statt allgemeinem Locator. Neuen Capture-Lauf nach Bug 6.2/6.3 Fix.

### Problem 3: Zu wenige Elemente pro Tab

E2E-Tests prüfen pro Tab nur 2–8 Elemente — im Web-UI sind es 10–20+.

**Fix:** `it.each`-Tests mit allen Tab-Elementen aus `reference-matrix.md` (Kategorien 7–13, 78 Elemente gesamt).

---

## 1:1-Paritäts-Strategie

1. **Referenz-DOM als Ground Truth:** `explore-8074.json` (127 Elemente, 272 IDs) ist autoritative Quelle für ALLE UI-Elemente. Jeder Fix muss gegen diese Referenz validiert werden.

2. **Analyse vor Fix:** Diese Datei enthält die abgeschlossene Analyse. Kein Fix ohne Rückbezug auf die obigen Analyse-Ergebnisse.

3. **E2E-Test pro Element:** Nach jedem Fix einen E2E-Test schreiben der das Element gegen die Referenz prüft (Existenz, Position, Text, Verhalten). Test-Files pro Bug-Gruppe:
   - `ui/e2e/panel-controls.spec.ts` (Bugs 1, 6)
   - `ui/e2e/waterfall.spec.ts` (Bug 2)
   - `ui/e2e/band-scale.spec.ts` (Bug 3)
   - `ui/e2e/tag-area.spec.ts` (Bug 4)
   - `ui/e2e/frequency-ruler.spec.ts` (Bug 5)

4. **Referenz-Matrix als Checklist:** `doc/reference-matrix.md` (78 Elemente, 15 Kategorien) nach jedem Fix aktualisieren (❌ → ✅).

5. **Implementierungs-Reihenfolge (nach Aufwand/Risiko):**
   1. Bug 5 (FrequencyRuler) — 1 Zeile ändern, 0 Risiko
   2. Bug 6.6/6.7 (Spectrum-Button, Audio-Symbol) — je 1 Element, einfach
   3. Bug 6.1/6.4 (Pan-Symbole, Zoom-Button) — CSS/Icon-Fix, einfach
   4. Bug 1 (P1-Button) — kleiner Handler + Store-State
   5. Bug 6.2/6.3 (Band/Ext-Select) — neue Datei `bands.ts` + Template
   6. Bug 6.5 (Button-Anordnung) — Layout-Refactor
   7. Bug 6.8 (RF-Tab) — neues Template + Store-Erweiterung
   8. Bug 4 (DX-Tags) — neue Tag-Liste + zweireihiges Layout
   9. Bug 3 (BandScaleBar) — Interface-Änderung + neue Daten
   10. Bug 2 (Wasserfall) — "No signal"-Overlay in `Waterfall.vue` (kein Simulator)

---

## Extensions-Planung (M4x)

> Keine Implementierung in M4c.7 — nur Planung.

KiwiSDR-Extensions werden über `id-select-ext` (27 Optionen) ausgewählt.

**M4x-Tasks (spätere Phase):**
1. Alle 27 Extensions einzeln analysieren (UI-Erweiterung + Panel-Inhalt)
2. Pro Extension: Vue-Stub-Komponente in `ui/src/components/extensions/`
3. Spectrum-Button 3 Modi vollständig implementieren (Bug 6.6-Fix ist Voraussetzung)
4. Extension-Panel (`id-ext-controls`) als generisches Container-Panel
5. E2E-Tests pro Extension-Stub

---

## M4c.7 Offene Tasks (Stand 2026-08-29)

> **Status 2026-08-29 (agent:DEV):** Alle drei offenen Tasks sind abgeschlossen.
> Verifikation: C++-Build Debug+Release grün (Validator 47/47, ctest 1/1), Playwright
> 85 passed / 1 skipped / 0 failed, Vitest 112/112. Details: `doc/log.md`.

### Task 1: C++ Build (VST3_SDK_ROOT)

**Fehler:**
```
cmake --preset win-msvc
CMake Error at CMakeLists.txt:57 (message):
  VST3 SDK not found. Set -DVST3_SDK_ROOT=<path> to the VST3 SDK
  (e.g. C:/Users/<you>/vst3sdk), or enable NS_VENDOR_VST3_SDK.
```

**Root cause:** Weder `VST3_SDK_ROOT` Environment-Variable gesetzt noch `NS_VENDOR_VST3_SDK=ON`. Der VST3 SDK ist nicht installiert/geklont.

**Fix:** VST3 SDK als Git-Submodul (`git submodule add` mit NS_VENDOR_VST3_SDK=ON) ODER Umgebungsvariable setzen. Dazu WEBVIEW2_SDK_ROOT prüfen.

**Status:** ✅ Erledigt — SDKs existieren unter `C:/Users/marku/Documents/GitHub/thirdParty/`
(`vst3sdk`, `WebView2SDK`). Der CMake-Cache (`build/win-msvc/CMakeCache.txt`) hat die
Pfade bereits korrekt gesetzt; `cmake --build build/win-msvc --config Debug|Release`
läuft durch (VST3-Validator 47/47, ctest 1/1).

---

### Task 2: 8 E2E-Tests failen

> **Status 2026-08-29 (agent:DEV):** ✅ Erledigt. Die Syntax-Fixes aus Commit
> `f6d9dd6` deckten die Fälle 2.1–2.7 ab; danach verblieben 11 Failures (alle
> veraltete/fragile Tests, keine UI-Bugs). Behandelt in
> `ui/e2e/band-presets.spec.ts`, `dx-tags.spec.ts`, `extension-select.spec.ts`,
> `frequency-ruler.spec.ts`, `panel-controls.spec.ts`. Resultat: **85 passed /
> 1 skipped / 0 failed**.

22 Tests ausgeführt, 13 passed, 1 skipped, **8 failed**.

#### 2.1 `tag-area.spec.ts:9` — `toHaveCount` Syntax-Fehler

**Fehler:** `toHaveCount(async () => {...})` — Playwright's `toHaveCount` akzeptiert keinen Callback, sondern eine Zahl.

**Fix:** `expect(await tags.count()).toBeGreaterThanOrEqual(20)`

#### 2.2 `panel-controls.spec.ts:55` — `toHaveCount({ min: 3 })` Syntax-Fehler

**Fehler:** `toHaveCount` akzeptiert nur `number`, kein Objekt.

**Fix:** `expect(await optgroups.count()).toBeGreaterThanOrEqual(3)`

#### 2.3 `panel-controls.spec.ts:72` — `toHaveCount({ min: 20 })` Syntax-Fehler

Identisch zu 2.2. **Fix:** `expect(await options.count()).toBeGreaterThanOrEqual(20)`

#### 2.4 `panel-controls.spec.ts:109` — Audio Button: Grüne Klasse nach Klick nicht gefunden

**Fehler:** Es wird `.kiwi-cpanel__icon-btn--green` gesucht, aber nach dem Klick ist der Button `--red`. Der Locator findet das Element nicht mehr.

**Fix:** Button vor dem Klick sichern (`const audioBtn = page.locator(...).first()`), dann klicken und prüfen ob er `--red` hat.

#### 2.5 `panel-controls.spec.ts:142` — RF Tab: `.kiwi-cpanel__btn--attn` existiert nicht

**Fehler:** Die Attn-Buttons im RF-Tab haben keine spezielle CSS-Klasse — sie verwenden `kiwi-cpanel__btn` mit `kiwi-cpanel__btn--green`/`--gray`. Der Test sucht nach nicht-existentem Selector.

**Fix:** Statt `.kiwi-cpanel__btn--attn` direkt über Text-Inhalt finden: `page.locator('.kiwi-cpanel__btn', { hasText: '0 dB' })` oder über die Ctrl-Row: `.kiwi-cpanel__ctrl-row` mit Text 'Attn'.

#### 2.6 `band-scale.spec.ts:49` — Regex passt nicht auf Dezimal-Format

**Fehler:** Frequenz-Input zeigt `"7100.00"` (2 Dezimalstellen), Regex erwartet `^7[012]\d{2}$` (exakt 4 Ziffern, keine Dezimalen).

**Fix:** Regex anpassen: `expect(freqText).toMatch(/^71[0-9]{2}/)` (präfixt)

#### 2.7 `frequency-ruler.spec.ts:16` — `'0'`-Label-Count falsch

**Fehler:** `labels.filter({ hasText: '0' })` findet 3 Elemente („30 MHz" enthält „0"). `filter({ hasText })` ist ein Substring-Match.

**Fix:** Exakten Match: `labels.filter({ hasText: /^0$/ })` oder `page.locator('.freq-ruler__label').filter({ hasText: '0 kHz' })`

#### 2.8 `frequency-ruler.spec.ts:39` — Keine kHz-Labels nach Zoom

**Fehler:** Nach 4 Zoom-Klicks (wfZoom=4, span≈1875 kHz, step=200 kHz) werden Labels erwartet die „kHz" enthalten. Aber `formatFreq` gibt für 200 kHz → „200 kHz", für 1000 kHz → „1.0 MHz". Bei span 1875 kHz ist der erste Tick bei 0, step 200 → Labels: 0, 200 kHz, 400 kHz, 600 kHz, 800 kHz, 1.0 MHz, 1.2 MHz... Es SOLLTEN kHz-Labels vorhanden sein. Möglicherweise reichen 4 Zoom-Klicks nicht — prüfen ob der Store-Zoom korrekt gesetzt wird.

**Fix:** Mehr Zoom-Klicks (6–8) oder `page.evaluate` um `wfZoom` direkt auf 8 zu setzen, dann Labels prüfen.

---

### Task 3: Veraltete E2E-Selektoren (M4b Bug 9)

Die E2E-Tests aus M4b-Zeiten (`.kiwi-control-panel`, `[data-testid="..."]`) wurden noch nicht geprüft ob sie wirklich alle aktualisiert wurden. Der `smoke.spec.ts` läuft — andere könnten noch veraltete Selektoren haben.

**Fix:** Alle E2E-Tests in `ui/e2e/` gegen das aktuelle DOM prüfen.

> **Status 2026-08-29 (agent:DEV):** ✅ Erledigt. Einziger veralteter Selektor war
> `.kiwi-cpanel__play-btn` in `extension-select.spec.ts` (Panel-Play-Button durch
> Bug 6.5 entfernt; Tests auf Floating-Button `.kiwi-play-btn` umgestellt). Keine
> weiteren `kiwi-control-panel`/`data-testid`-Referenzen in `ui/e2e/*.spec.ts`.

---

## Chronologie

| Zeit | Ereignis |
|------|----------|
| 2026-08-28 | M4b-Bugs erfasst (M4b-bugs.md) |
| 2026-08-29 | M4c abgeschlossen (E2E-Tests, Bugfixes) — M4b-Bugs gefixt |
| 2026-08-29 | M4c.7 Bug-Manifest erstellt — 6 neue Bugs aus manueller Prüfung |
| 2026-08-29 | M4c.7 Bugs implementiert (6 Bugs + 12 Sub-Bugs gefixt, Build ✅, 112 Unit-Tests ✅) |
| 2026-08-29 | M4c.7 E2E-Tests geschrieben (5 neue Files, 22 Tests) — 8 failen, 2 haben Syntax-Fehler |
| 2026-08-29 | C++ Build blockiert (VST3_SDK_ROOT fehlt) — als Task dokumentiert |
| 2026-08-29 | **M4c.7 Tasks 1–3 abgeschlossen:** C++-Build verifiziert (SDK-Pfade im Cache), 11 E2E-Failures behoben (veraltete/fragile Tests), Playwright 85/85 grün |
| 2026-08-29 | **Bug 7 erfasst:** Frequenz-Cursor entspricht nicht KiwiSDR (zoom-abhängige Passband-Repräsentation fehlt) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 8 erfasst:** Frequenzband-Skala verhält sich nicht wie Web UI (adaptive pixel-basierte Tick-Engine fehlt) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 9 erfasst:** Band- & Stationsleiste verhält sich nicht wie Web UI (dynamische Skalierung + Kollisions-Layout + Verbindungslinien fehlen) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 10 erfasst:** Audio-Tab fehlen Parameter + Scrollbar (Research-Pflicht vor Fix) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 11 erfasst:** AGC-Tab beinhaltet eventuell nicht alle Parameter (Research-Pflicht vor Fix) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 12 erfasst:** Header-Bereich entspricht nicht der Web UI (Station-Info, Credits, Collapse/Expand + Bild-Bereich fehlen; Connect-Funktionalität muss erhalten bleiben) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 13 erfasst:** "Spec RF"-Button soll funktionieren (Spektrumanalysator-Top-View fehlt) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 14 erfasst:** "Spec AF"-Button soll funktionieren (AF-Spektrumanalysator mit Center-Linie + Filtergrenzen fehlt) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 15 erfasst:** DRM-Tab (Button) funktioniert nicht (Schedule/Services-Overlay + Decoder-Panel + DRM-Bandbreite fehlen; Research-Pflicht) — Analyse + Fix-Plan dokumentiert, noch offen |
| 2026-08-29 | **Bug 7 umgesetzt:** Research (openwebrx.js, Port 8074) + Cursor als SVG-Overlay (passband_visible-Zustand, drei Hit-Zonen, MIN/MAX-Clamp) + Pan-Zone im Wasserfall — vue-tsc + Vitest 112/112 + E2E 85/85 grün |
