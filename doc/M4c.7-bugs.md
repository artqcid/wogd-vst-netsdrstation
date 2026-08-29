---
type: Bug Manifest + Implementation Plan
title: M4c.7 — Bug-Manifest + Implementation Plan
description: 6 Bugs aus manueller Prüfung (2026-08-29) mit vollständiger Analyse und konkretem Fix-Plan pro Bug. Analysiert 2026-08-29 (agent:plan).
status: bugs-fixed-e2e-pending
generated:
  by: agent:plan
  at: 2026-08-29
verified: ~
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

6 Bugs aus manueller Prüfung des Live-Plugins am 2026-08-29 identifiziert.
Vollständig analysiert 2026-08-29 durch Abgleich mit `explore-8074.json`, `panel.json`, `subtabs.json` und Quellcode-Inspektion.

**Vorgänger:** [`M4b-bugs.md`](./archive/M4b-bugs.md) — M4b-Bugs (archiviert, in M4c gefixt).

**Grundregel:** Jeder Fix wird gegen `explore-8074.json` (127 Elemente, 272 IDs) validiert.
Nach jedem Fix: `reference-matrix.md` aktualisieren (❌ → ✅), E2E-Test schreiben.

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

**Status:** Offen

---

### Task 2: 8 E2E-Tests failen

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
