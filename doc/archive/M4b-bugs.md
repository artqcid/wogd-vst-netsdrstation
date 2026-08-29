---
type: Bug List
title: M4b — Bug List & Analysis (ARCHIV — Bugs in M4c gefixt)
description: Captured bugs after user review of M4 UI state; analysis, root causes, severity
status: deprecated
generated:
  by: human:marku
  at: 2026-08-28
verified:
  by: human:marku
  at: 2026-08-29
tags: [m4, bugs, analysis, ui, playwright, severity]
stale_after: 2026-09-30
sources:
  - title: M4 UI Replication Analysis
    path: doc/M4-ui-replication-analysis.md
---

> **[DEPRECATED]** Diese Datei ist archiviert. Alle Bugs aus M4b wurden in M4c (2026-08-29) gefixt.
> Aktuelles Bug-Manifest: [`M4c.7-bugs.md`](./M4c.7-bugs.md)

# M4b — Bug-Liste & Analyse (Stand: 2026-08-28)

# M4b — Bug-Liste & Analyse (Stand: 2026-08-28)

Erfasste Bugs nach User-Review des M4-Stands. Referenz: `doc/M4-ui-replication-analysis.md`
und die KiwiSDR-Web-Implementierung (kiwisdr.com/public, github.com/jks-prv/KiwiSDR/web/kiwi/).

---

## Warum die Playwright-Tests die Bugs nicht aufgedeckt haben

Die E2E-Tests (`ui/e2e/`) wurden **nie tatsächlich ausgeführt**:

1. **Veraltete Selektoren:** `smoke.spec.ts`, `resize.spec.ts`, `freq-tuning.spec.ts`,
   `band-presets.spec.ts`, `mode-select.spec.ts` greifen auf Selektoren der alten Architektur
   zu (`.kiwi-topbar__title`, `[data-testid="mode-panel"]`, `[data-testid="freq-panel"]`,
   `.kiwi-control-panel`, `[data-testid="status-bar"]` etc.) — diese existieren im neuen
   Layout alle nicht mehr. Die Tests würden komplett rot sein.

2. **Kein CI/CD-Lauf für Playwright:** `npm run test:unit` läuft nur Vitest-Unit-Tests.
   Playwright (`npx playwright test`) wird nirgends automatisch aufgerufen.

3. **`kiwi-layout.spec.ts` prüft nur Screenshot-Diff**, nicht Funktionalität.
   Die Baseline existiert nicht, der erste Lauf würde sie anlegen (kein Fehler).

4. **Vitest-Unit-Tests testen keine visuelle Korrektheit** — sie prüfen nur, ob
   Selektoren im DOM vorhanden sind, nicht ob Positionen/Breiten/Zoom-Verhalten stimmen.

**Konsequenz:** Alle unten aufgeführten Bugs sind echte Implementierungslücken.
Die E2E-Tests müssen vollständig neu geschrieben werden (→ M4b.9).

---

## Bug 1 — Plugin-Zoom falsch (App skaliert statt nur resizen)

**Beschreibung:** Die Anwendung skaliert sich (CSS `transform: scale(...)`) wenn das
Plugin-Fenster vergrößert wird. Das Verhalten ist falsch.

**Soll:** Der Plugin-Editor ist fluid (`100vw × 100vh`, kein festes Surface). Wenn der
User die untere rechte Ecke des Plugin-Fensters zieht, wächst die WebView-Fläche —
alle Layout-Bereiche skalieren mit (flex-grow, `width: 100%`). **Kein** CSS-`transform`
oder `scale()`.

**Ist:** In `App.vue` oder einer übergeordneten Schicht ist ein Scale-Transform
aktiv, das die 1280×720-"Design-Surface" auf die aktuelle Fenstergröße skaliert.
Dieses Verhalten wurde in einem früheren Milestone eingeführt und muss entfernt werden.

**Root cause:** `doc/M4-ui-replication-analysis.md` §1.1 schreibt explizit:
> "Das Original nutzt KEIN `App.vue`-Scale-Transform (kein 1280×720 fixed surface).
> Die Vue-App muss auf fluid `100vw × 100vh` umgestellt werden."

**Fix-Aufwand:** klein — `App.vue` + ggf. `master.css` von Scale-Transform bereinigen.

**Betrifft:** `ui/src/App.vue`, `ui/src/assets/master.css`

---

## Bug 2 — Spektrogram/FrequencyRuler-Zoom (Ctrl+Mausrad) fehlt komplett

**Beschreibung:** Wenn der Mauszeiger im Spektrogram oder in der Frequenzband-Leiste
(FrequencyRuler) liegt, soll `Ctrl + Mausrad` den dargestellten Frequenzbereich zoomen
(nicht die Anwendung skalieren). Funktioniert nicht.

**Soll-Verhalten (KiwiSDR-Original):**
- `Ctrl + Mausrad rein` → Zoom-Level erhöhen (schmalerer Frequenzbereich sichtbar),
  bis sinnvoller Minimalbereich (z. B. 1–2 kHz, zoom-Level 14)
- `Ctrl + Mausrad raus` → Zoom-Level verringern, bis maximale Bandbreite sichtbar (zoom 0)
- Beim Zoomen bleibt die Frequenz unter dem Mauszeiger fix (Ankerpunkt)
- Zoom-Level 0–14 steuert `store.wfZoom` (VST3-Parameter `wfZoom`)
- **Nur Frequenzbereich-Zoom, nie CSS-Skalierung**

**Ist:**
- `Waterfall.vue` hat `@wheel.prevent="onWheel"` und emittiert `zoom`-Events
- `PluginView.vue` fängt das Event **nicht** ab (kein `@zoom`-Handler auf dem
  `<Waterfall>`-Tag in PluginView.vue Zeile 101)
- `FrequencyRuler` hat kein Wheel-Listener
- Das Zoom-Delta aus `Waterfall.onWheel` wird nirgends mit `store.wfZoom` verbunden

**Fix-Aufwand:** mittel
- `PluginView.vue`: `@zoom="onWfZoom"` auf `<Waterfall>` ergänzen + Handler
- `FrequencyRuler.vue`: `@wheel.prevent` + emit `zoom`
- `PluginView.vue`: gemeinsamer Handler der `store.wfZoom` und `store.freqKhz`
  (Anker-Rechnung) aktualisiert
- `Waterfall.vue`/`FrequencyRuler.vue`: Zoom-Logik muss `:span-khz` prop berechnen
  aus `wfZoom` + aktuellem Band

**Betrifft:** `ui/src/views/PluginView.vue`, `ui/src/components/Waterfall.vue`,
`ui/src/components/FrequencyRuler.vue`, `ui/src/store/kiwiStore.ts`

---

## Bug 3 — BandScaleBar: falsche Breiten, kein Klick-zu-Frequenz

**Beschreibung:** Die Band-Blöcke in der Bandskala haben falsche proportionale Breiten
und sind als absolut positionierte Spans mit hardcodierten `left`-Werten implementiert
(z. B. `left: 2%`, `left: 4%`). Klick auf einen Block setzt die Frequenz nicht.

**Soll-Verhalten:**
- Jeder Band-Block hat eine Breite proportional zu seiner tatsächlichen Bandbreite,
  relativ zur dargestellten Gesamt-Bandbreite (aktueller Zoom)
- Block-Farben aus KiwiSDR-Original: Broadcast=teal, Amateur=rot/pink, Utility=gelb, etc.
- **Klick auf Block** → setzt `store.freqKhz` auf die Mitte des Frequenzbereichs
- Cursor-Linie im FrequencyRuler und gelbe Linie im Spektrogram müssen mitbewegen
- Beim Zoomen müssen die Blöcke ihre Breiten und Positionen dynamisch anpassen

**Ist:**
- `BandScaleBar.vue` existiert mit hardcodierten `left`-Werten, aber keine
  reaktive Berechnung aus aktuellem `wfZoom` / sichtbarem Frequenzbereich
- Kein `@click`-Handler
- In `PluginView.vue` ist der `.kiwi-bandscale`-Bereich mit Inline-Spans
  (nicht die `BandScaleBar`-Komponente!) implementiert (Zeilen 46–61)

**Fix-Aufwand:** groß
- Eigene Composable `useBandLayout(loKhz, hiKhz)` die Band-Daten mit `x`, `width`
  in Prozent berechnet
- `BandScaleBar.vue` erhält Props `lo-khz`, `hi-khz` und emittiert `@band-click(freqKhz)`
- `PluginView.vue`: `.kiwi-bandscale` durch `<BandScaleBar>` ersetzen, Zoom-Props binden
- Band-Datensatz vollständig (aus `doc/M4-ui-replication-analysis.md` §4.2)

**Betrifft:** `ui/src/components/BandScaleBar.vue`, `ui/src/views/PluginView.vue`,
`ui/src/store/kiwiStore.ts` (loKhz/hiKhz computed aus wfZoom+freqKhz)

---

## Bug 4 — Stations-Tags: kein Klick-zu-Frequenz, kein Popup-Menü

**Beschreibung:** Klick auf einen DX-Tag/Station im TagArea setzt die Frequenz nicht
und zeigt kein Popup-Menü (wie im Original, Bild 2 zeigt Popup bei Station-Klick).

**Soll-Verhalten:**
- Klick auf Tag → `store.freqKhz` wird auf `tag.freqKhz` gesetzt
- Cursor im FrequencyRuler + Wasserfall-Linie springen sofort auf diese Frequenz
- Manche Stationen haben ein zusätzliches Popup (kleines Modal unten), das
  Sender-Name, Frequenz, Sprache, Zeit-Info zeigt (aus dem EiBi/SWBC-Datensatz)
- Popup schließt sich bei Klick außerhalb oder Escape

**Ist:**
- `TagArea.vue` rendert Tags, aber kein `@click`-Handler
- In `PluginView.vue` sind `.kiwi-tag`-Spans inline ohne Event-Handler (Zeilen 65–71)
- Kein Popup-System vorhanden

**Fix-Aufwand:** mittel
- `TagArea.vue`: `@click="emit('tag-click', tag)"` auf jedem Tag
- `PluginView.vue`: `@tag-click="onTagClick"` → `store.setParam('freqKhz', tag.freqKhz)`
- Popup-Komponente `TagPopup.vue` (kleines Modal, positioniert relativ zum Tag)
- Popup-Datensatz: erweiterte Tag-Daten (Name, Beschreibung, Zeiten)

**Betrifft:** `ui/src/components/TagArea.vue`, `ui/src/views/PluginView.vue`,
neu: `ui/src/components/TagPopup.vue`

---

## Bug 5 — FrequencyRuler: kein Frequenz-Cursor (Dragger)

**Beschreibung:** Im FrequencyRuler (Frequenzband-Leiste) fehlt der interaktive
Frequenz-Cursor (gelber/grüner Zeiger).

**Soll-Verhalten (aus Bildern 3+4):**
- **Weit herausgezoomt (Zoom 0–~8):** gelber Cursor (Λ-Form, wie im Original-Screenshot).
  Lässt sich mit der Maus horizontal ziehen → setzt `store.freqKhz`
- **Weit hereingezoomt (Zoom ~9–14):** grüner Cursor, zeigt das aktuelle Passband
  (LoKhz–HiKhz) als Klammer. Linker Rand = Lo, rechter Rand = Hi, Mitte = Frequenz.
  - Klick+Ziehen auf Mitte: verschiebt `freqKhz`
  - Klick+Ziehen auf linken Rand: ändert `lowCut`
  - Klick+Ziehen auf rechten Rand: ändert `highCut`
- Die Cursor-Position muss mit `store.freqKhz` und dem sichtbaren Bereich
  (`loKhz`, `hiKhz`) synchron sein

**Ist:**
- `FrequencyRuler.vue` rendert Ticks und Labels, aber kein Cursor-Element
- `PassbandOverlay.vue` existiert (hat ähnliche Logik für das Wasserfall-Canvas),
  aber ist nicht mit FrequencyRuler verbunden
- In `PluginView.vue` wird `PassbandOverlay` nicht verwendet

**Fix-Aufwand:** groß
- `FrequencyRuler.vue`: Cursor-Overlay (gelb/grün je nach Zoom-Level), interaktiv
- Cursor-Drag-Logik (mousedown/move/up) → emittiert `freq-change`, `low-cut-change`,
  `high-cut-change`
- `PluginView.vue`: Events binden auf `store.setParam(...)`
- Zoom-abhängige Farbe: gelb wenn `wfZoom < 9`, grün wenn `wfZoom >= 9`

**Betrifft:** `ui/src/components/FrequencyRuler.vue`, `ui/src/views/PluginView.vue`

---

## Bug 6 — Menü/ControlPanel: falsche Anordnung, viele Buttons ohne Funktion

### 6.1 Layout weicht vom Original ab

**Beschreibung:** Das ControlPanel stimmt optisch nicht mit der Web-Vorlage überein.

**Fehlendes/Falsches:**
- Menü-Icon (≡) + User-Anzeige (A, grün, Zahl) fehlen oder haben kein Tooltip/Funktion
- Frequenz-Eingabefeld: kein großes digitales Readout (die Webversion zeigt eine
  große Monospace-Anzeige), nur ein kleines Input
- Zoom/Nav-Buttons (Row 2+4) zeigen Fragezeichen (`?`) statt Icons (keine Icon-Library)
- Play-Button links am Canvas zeigt `?`, kein Symbol
- Tab-Buttons (RF/WF0/Audio/AGC/User/Stat/Off) fehlt "WF9" Label (sollte die
  aktuelle WF-Nummer zeigen, z. B. "WF9" wenn 9 Empfänger aktiv)

### 6.2 Zoom-Buttons (Row 4) falsch signalisiert + ohne korrekte Funktion

**Soll (aus Bild 5, von links nach rechts):**
1. Lupe `+` → einen Schritt reinzoomen (`wfZoom + 1`)
2. Lupe `-` → einen Schritt rauszoomen (`wfZoom - 1`)
3. Zwei Pfeile nach innen (diagonal) → Maximum rauszoomen (`wfZoom = 0`)
4. Zwei Pfeile nach außen (diagonal, grün hervorgehoben) → Maximum reinzoomen (`wfZoom = 14`)
5. Zwei Pfeile nach außen waagrecht → Zoom to Band (zeigt den aktuellen Band-Bereich)
6. Pfeil nach links `<` → Frequenzbereich nach links schieben (Pan left)
7. Pfeil nach rechts `>` → Frequenzbereich nach rechts schieben (Pan right)

**Ist:** Buttons zeigen `?`, Reihenfolge unklar, `onZoom(±1)` und `onZoom(±2)` ohne
Zoom-to-Band / Pan-Funktion.

**Betrifft:** `PluginView.vue` Zeilen 137–152, 165–174

### 6.3 Frequenz-Steuerungsbuttons fehlen

**Beschreibung:** Im ControlPanel fehlen die präzisen Frequenz-Schritt-Buttons.

**Soll (aus Bild 8, 6 Buttons):**
- `−−` groß: −10 kHz (oder −15 Hz je nach Kontext — lt. User −15 Hz bei kleinen,
  aber das Original zeigt: großes `+/−` = ±10 kHz, mittleres = ±1 kHz, kleines = ±0.1 kHz)
- `−` mittel: −1 kHz
- `−` klein: −0.1 kHz
- `+` klein: +0.1 kHz
- `+` mittel: +1 kHz
- `++` groß: +10 kHz

**Ist:** In Row 4 (Nav) sind 6 Buttons, aber mit falschen Funktionen:
`onZoom(1)`, `onZoom(-1)`, "Expand passband", "Reset passband", `stepFreq(-1)`,
`stepFreq(1)` — `stepFreq` macht nur ±1 kHz, kein ±0.1 / ±10.

**Betrifft:** `PluginView.vue` Zeilen 165–174, `stepFreq()`-Funktion

### 6.4 Sub-Tab-Inhalte unvollständig (RF, Audio, AGC, User, Stat)

**Beschreibung:** Nur der WF0-Tab hat Inhalt. Alle anderen Tabs (RF, Audio, AGC, User,
Stat) zeigen nichts, obwohl sie im Original umfangreiche Panels haben.

**Soll (aus Bildern 6+7):**

| Tab | Inhalt |
|-----|--------|
| **RF** | Farbbalken (Kiwi-Farbskala), WF ceil-Slider, WF floor-Slider, WF rate-Slider, Spec Δ-Slider, AutoScale-Button, SpecColor-Button, P1-Button, Dropdowns (Colormap, Aperture-Algo, Timestamps, FFT-Window) |
| **WF0/WF9** | identisch mit RF (WF = Waterfall-Nummer) |
| **Audio** | Volume-Slider, Mute-Button, Kompression/De-emphasis, NR (Noise Reduction) On/Off |
| **AGC** | AGC On/Off Toggle, Threshold-Slider (-140..0 dB), Decay-Slider (20..5000 ms), Hang On/Off, Slope-Slider, Manual Gain-Slider |
| **User** | Squelch On/Off + Threshold, NB (Noise Blanker) On/Off + Threshold |
| **Stat** | GPS-Lock, User-Count, Buffer-Status, SNR |
| **Off** | Audio off (mute output) |

**Ist:** `activeTab === 'WF0'` zeigt 4 Slider-Rows + Dropdowns, Rest zeigt nichts.

**Betrifft:** `PluginView.vue` Zeilen 191+, `store/kiwiStore.ts` (fehlende Params)

### 6.5 Band-Select Dropdown ohne Funktion

**Beschreibung:** Das "select band" Dropdown in Row 1 des ControlPanels hat nur
einen Platzhalter-Eintrag und keine Funktion.

**Soll:** Dropdown mit vollständiger Band-Liste (Amateur + Broadcast + Utility).
Auswahl setzt `store.freqKhz` auf die Band-Mitte und passt `wfZoom` so an,
dass das Band vollständig sichtbar ist.

**Band-Datensatz:**
- Amateur: 160m (1800 kHz), 80m (3500), 60m (5300), 40m (7000), 30m (10100),
  20m (14000), 17m (18068), 15m (21000), 12m (24890), 10m (28000)
- Broadcast: LW (153), MW (520), SW-49m (5900), SW-41m (7100), SW-31m (9500),
  SW-25m (11600), SW-22m (13570), SW-19m (15100), SW-16m (17480), SW-13m (21450), SW-11m (25600)
- Utility: DCF77 (77.5), WWV (2500/5000/10000/15000/20000), NDB (200–1750)

**Betrifft:** `PluginView.vue` Zeile 127–129

### 6.6 Extension Dropdown ohne Funktion

**Beschreibung:** Das "extension" Dropdown ist ein Platzhalter ohne Funktion.

**Soll:** Dropdown mit den verfügbaren Extensions:
CW Decoder, WFAX (Weather Fax), RTTY/FSK, SSTV, tDoA, IQ Display, Noise Blanker, Noise Reduction.
Auswahl öffnet ein dynamisches Control-Panel unterhalb der Tabs (scrollbar).

**Ist:** Leeres Dropdown, kein dynamisches Panel.

**Betrifft:** `PluginView.vue` Zeile 130–132, neu: Extension-Panel-Komponenten

---

## Bug 7 — Visuelle Abweichungen vom Original (allgemein)

**Beschreibung:** Mehrere visuelle Elemente weichen vom KiwiSDR-Original ab.

| Element | Ist | Soll |
|---------|-----|------|
| Icons in Buttons | `?` (kein Icon) | Font Awesome 4.x oder SVG-Icons |
| Play-Button | violettes Rechteck mit `?` | violetter Kreis, Play-Triangle (►) |
| BandScaleBar | falsches Layout (absolute Spans) | proportionale farbige Blöcke (siehe Bug 3) |
| TagArea | nur 7 Demo-Tags in PluginView.vue, DEMO_TAGS in TagArea.vue | EiBi/SWBC-Datensatz (aus `doc/M4-ui-replication-analysis.md` §4.3) |
| FrequencyRuler | statische Hardcode-Labels | dynamisch aus loKhz/hiKhz berechnet |
| ControlPanel-Farben | Tab-Farben korrekt, aber Slider-Styling weicht ab | Kiwi-Slider (breite Spur, runder Thumb, Farbverlauf) |
| Header | hat Kiwi-Vogel-Logo, aber kein echter KiwiSDR-Stil | näher an Original: grüner Kreis+Kiwi-Silhouette OK |
| Waterfall-Farbpalette | Standard, `:color-map` prop korrekt gebunden | Kiwi-Colormap (gelb-rot-blau Gradient) als Default |

---

## Bug 8 — Wasserfall/FrequencyRuler: Span/Zoom nicht an Store gebunden

**Beschreibung:** `Waterfall.vue` und `FrequencyRuler.vue` erhalten keine Props für
`loKhz`/`hiKhz` (sichtbarer Frequenzbereich) aus dem Store. Das Zoomen über
`store.wfZoom` hat daher keinen Effekt auf den dargestellten Bereich.

**Soll:**
```
wfZoom → spanKhz = kiwiBandwidth / 2^wfZoom
loKhz  = freqKhz - spanKhz/2
hiKhz  = freqKhz + spanKhz/2
```
Alle Komponenten (Waterfall, FrequencyRuler, BandScaleBar) erhalten `lo-khz` und
`hi-khz` als Props und rendern ihren Inhalt relativ dazu.

**Ist:** `Waterfall.vue` hat `:centre-khz="store.freqKhz"` aber kein `:span-khz`.
`FrequencyRuler.vue` hat hardcodierte Labels (0…30 MHz).

**Fix-Aufwand:** groß (Architektur-Änderung: `spanKhz` als zentrale berechnete Größe)

**Betrifft:** `ui/src/store/kiwiStore.ts`, `ui/src/views/PluginView.vue`,
`ui/src/components/Waterfall.vue`, `ui/src/components/FrequencyRuler.vue`,
`ui/src/components/BandScaleBar.vue`

---

## Bug 9 — E2E-Tests komplett veraltet

**Beschreibung:** Alle Playwright-E2E-Tests prüfen Selektoren der alten Architektur.
Sie würden bei einem echten Playwright-Run alle fehlschlagen.

**Veraltete Selektoren (muss ersetzt werden):**
- `.kiwi-topbar__title` → `.kiwi-header__title`
- `[data-testid="mode-panel"]` → kein testid mehr (Modes sind in `.kiwi-cpanel__row--modes`)
- `[data-testid="band-panel"]` → nicht vorhanden
- `[data-testid="freq-panel"]` → nicht vorhanden
- `[data-testid="audio-panel"]` → nicht vorhanden
- `[data-testid="waterfall-panel"]` → nicht vorhanden
- `[data-testid="extension-panel"]` → nicht vorhanden
- `[data-testid="status-bar"]` → nicht vorhanden (S-Meter ist `.kiwi-cpanel__smeter`)
- `.kiwi-control-panel` → `.kiwi-cpanel`
- `.k-readout`, `.k-button` → nicht vorhanden

**Betrifft:** `ui/e2e/smoke.spec.ts`, `ui/e2e/resize.spec.ts`,
`ui/e2e/freq-tuning.spec.ts`, `ui/e2e/band-presets.spec.ts`,
`ui/e2e/mode-select.spec.ts`

---

## Prioritäts-Übersicht

| Bug | Titel | Priorität | Aufwand |
|-----|-------|-----------|---------|
| 1 | Plugin-Zoom (Scale-Transform entfernen) | Kritisch | Klein |
| 2 | Ctrl+Mausrad Spektrogram-Zoom | Hoch | Mittel |
| 8 | Zoom/Span an Store binden (Architektur) | Hoch | Groß |
| 3 | BandScaleBar Breiten + Klick-zu-Frequenz | Hoch | Groß |
| 5 | FrequencyRuler Cursor-Dragger | Hoch | Groß |
| 4 | Stations-Tags Klick-zu-Frequenz + Popup | Mittel | Mittel |
| 6.1 | Icons (Font Awesome) | Mittel | Klein |
| 6.2 | Zoom-Buttons korrekt signalisiert + Pan | Mittel | Klein |
| 6.3 | Frequenz-Schritt-Buttons (±0.1/±1/±10 kHz) | Mittel | Klein |
| 6.4 | Sub-Tab-Inhalte (RF/Audio/AGC/User/Stat) | Mittel | Groß |
| 6.5 | Band-Select Dropdown | Mittel | Mittel |
| 6.6 | Extension Dropdown + Panel | Niedrig | Groß |
| 7 | Visuelle Korrekturen (Colormap, Slider, etc.) | Niedrig | Mittel |
| 9 | E2E-Tests vollständig neu schreiben | Hoch | Groß |

---

## Empfohlene Implementierungsreihenfolge (M4b)

1. **M4b.1** Bug 1: Scale-Transform entfernen (App.vue/master.css)
2. **M4b.2** Bug 8: Zoom-Architektur — `spanKhz` als Store-Composable, Props an alle Komponenten
3. **M4b.3** Bug 2: Ctrl+Mausrad → Store-Zoom
4. **M4b.4** Bug 5: FrequencyRuler Cursor-Dragger (gelb/grün)
5. **M4b.5** Bug 3: BandScaleBar proportional + Klick-zu-Frequenz
6. **M4b.6** Bug 6.1+6.2+6.3: Icons + Zoom-Buttons + Frequenz-Schritte
7. **M4b.7** Bug 4: TagArea Klick-zu-Frequenz + Popup
8. **M4b.8** Bug 6.4+6.5+6.6: Sub-Tab-Inhalte + Band/Extension Dropdowns
9. **M4b.9** Bug 9: E2E-Tests neu schreiben (nach allen obigen Fixes)
10. **M4b.10** Bug 7: Visuelle Korrekturen (Colormap, Slider-Style, Icons)
