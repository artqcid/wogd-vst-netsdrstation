---
type: Bug Manifest
title: M4c.7 — Bug-Manifest + 1:1-Paritäts-Gap
description: 6 Bugs aus manueller Prüfung (2026-08-29), E2E-Lückenanalyse, Extensions-Planung. Vorgänger: M4b-bugs.md (Archiv).
status: in-progress
generated:
  by: agent:plan
  at: 2026-08-29
verified: ~
tags: [m4, m4c, bugs, analysis, ui, parity, extensions]
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
  - title: Referenz-Matrix
    path: doc/reference-matrix.md
---

# M4c.7 — Bug-Manifest & 1:1-Paritäts-Strategie

6 Bugs aus manueller Prüfung des Live-Plugins am 2026-08-29 identifiziert.
Jeder Bug = **ANALYSE-Task** (Referenz-DOM-Abgleich) → **FIX-Task**.
E2E-Tests (65 Playwright, 112 Vitest) laufen grün, deckten aber diese Bugs nicht ab → E2E-Lückenanalyse.

**Vorgänger:** [`M4b-bugs.md`](./archive/M4b-bugs.md) — M4b-Bugs (archiviert, in M4c gefixt).

---

## Bug 1 — Violetter Button öffnet nicht Tip-Panel

**Betroffene Komponente:** `PluginView.vue` (`.kiwi-cpanel__btn--violet`)

**IST:** Violetter Button (vermutlich "P1"-Button in WF0-Tab oder Bedienpanel) verhält sich
falsch — stoppt Audio statt ein Tip/Welcome-Panel zu togglen.

**Referenz (panel.json):**
- `id-readme` — Welcome-Panel (x:10, y:495, w:605, h:295), Text beginnt mit "Welcome!"
- `id-readme-inner` — Panel-Inhalt (x:20, y:505, w:585, h:275)
- `id-readme-vis` + `id-readme-hide` / `id-readme-show` — Vis-Toggle oben-rechts im Panel
- Inhalt: "Project website: kiwisdr.com ... Here are some tips: Show and hide the panels ..."

**SOLL:** Button togglet `isReadmeOpen` → `<ReadmePanel>` erscheint/verschwindet.
Tip-Text aus `panel.json` `id-readme` übernehmen.

**Analyse-Task:** Prüfen welcher Button gemeint ist (`.kiwi-cpanel__btn--violet` "P1"?).
Referenz `id-readme`-Struktur in `panel.json` Zeilen 7020-7148.

**Fix-Task:** Readme-Panel-Komponente `<TipPanel>` einfügen, Button-Funktin onToggleReadme().

---

## Bug 2 — Spektrometer zeigt keine Daten

**Betroffene Komponente:** `Waterfall.vue` / `pectrumRendere.vu`

**IST:** Wasserfall / Spektrom zeigt undefinierte Farben — keine echteen Signal-Daten.

**Referenz (explor-8074.json):**
- `#id-wf-canvas` (1280x200) — Haup-Wasserfal
- `#id-spectum-canvas` — Spektrum-Anzeige
- `#id-spectrum-af-canvas` — Audio-Freqeunz-Spektrum
- `#id-spectrum-pb-canvas` — Passband-Spektrum
- `#id-annotation-canvas` (1280x576) — Station-Tags-Lear

**SOLL:** Echte WF/Spektrum-Daten vom Server dekodieren + Canvas rendern.
Falls kein Daten-Strom: simulierte Daten MIT KiwiSDR-ähnlicher Charakteristik.

**Analyse-Task:** Prüfen ob WF-Daten im `Waterfall.vue` ankommen.
Ist der `WF data`-Stream vom KiwiSDR in `KiwiClient` korrekt dekodiert?
Oder nur Demo/Silence?

**Fix-Task:** WF-Dekodierung fixen ODER Simulator mit realistischem Rauschen + Signalen.

---

## Bug 3 — Frequenzband-Leiste inkorrekt

**Betroffene Komponente:** `BandScaleBar.vue`

**IST:** Zeigt nur Meter-Angaben (keine Captions wie "Broacast"), falsceh Farben,
keine Zoom-Mitlauf → Bänder stimmen nicht mit tatsächlichen Frequenzen überein.
Bänder sind anklickbare Buttons (sollten rere Farbfelder sein).

**Referenz:**
- `id-band-canvas` (1280x30) — Band-Skala-Canvas
- 87 Band-Optionen in `id-selct-band`
- Farben + Captions aus KiwiSDR-Web-UI

**SOLL:** Farbige `div`-Felder (keine `button`!) mit Captions.
Bänder skalieren mit `zoomAnchor` / `zoomLveel` — Breiten propotional zum Frquenzbereich.
Farben + Captions aus `doc/kiwisdr-pritcol-refernce.md` Band-Tabelle.

**Analyse-Task:** Band-Definitionen aus Referenz extahieren (87 Bänder mit Name, Frequenz, Farbe).
Prüfen `BandScaleBar.vue` Rendering-Logik.

**Fix-Task:** `<BandScaleBar>` umbauen — `div` mit `background` + `position:absolute; left:...; width:...`.
Zoom-Transformation: `left = (freqStart - viewStart) * pxPerKhz`.

---

## Bug 4 — DX-Tags fehlen / inkorrekt

**Betroffene Komponente:** `TagArea.vue` / `DXTags.vue`

**IST:** Nur 30 DEMO_TAGS (statt ~73). Falsche Farben. Kein zweireihiges Layout.
Keine vertikale Verbindungslinien von Tags zum Spektrgramm-Rand.

**Referenz (explore-8074.json):**
- 73 Buttons `id-dx-label_0` bis `id-dx-label_72`
- Klassen: `cl-dx-label`, `dx-has-ext` / `cl-dx-label-ext`, `dx-<freq>`
- Zweireihig bei Überlappung (tags nahe beieinander)
- Vertikale Linien zur kHz-Skala (wo die Frequenz im Lineal liegt)

**SOLL:** Volle DX-Tags-Liste rendern (73+ Tags).
Zweireihiges Layout wenn Tags sich überlappen.
Linien vom Tag-Button zum oberen Rand des Spektrogramms (wo kHz-Lineal ist).
Farben aus Live-Referenz entnehmen.

**Analyse-Task:** `dx-selects-smeter.json` hat leere Arrays — Live-Capture war defekt.
DX-Tag-Daten aus `explore-8074.json` extrahieren (id, text, rect, class).
Prüfen ob zweireihiges Layout in `TagArea.vue` implementierbar ist.

**Fix-Task:** DX-Tag-Daten in `DETO_TAGS` ersetzen (73+ Einträge).
Zweireihiges Layout: `oveflow: visible` + `flex-wrap`.
Linien zum Spektrogramm: `::before`-Pseudoelment mit `border-left`.

---

## Bug 5 — kHz-Lineal skaliert nicht beim Zoom

**Betroffene Komponente:** `FrequencyRuler.vue`

**IST:** kHz/MHz-Striche haben feste Abstände — passen sich NICHT dem Zoom an.
Markierungsstriche müssen bei mehr Zoom ein viel engeres "Lineal"-Raster bekommen.

**Referenz:**
- `id-scale-canvas` (1280x47) — adaptives Frequenz-Lineal
- Zoom-Level 0: grobe kHz-Striche
- Zoom-Level 14: feine Hz-Striche

**SULL:** Adaptives Tick-Intervall basierend auf `zoomLevel`.
`zoomLevel` 0–4: 100 kHz / 10 kHz. `zoomLevel` 5–9: 5 kHz / 1 kHz.
`zoomLevel` 10–14: 1 kHz / 100 Hz.
Ticks berechnen als `Math.pow(10, Math.floor(Math.log10(spanKhz / pixelsPerTick)))`.

**Analyse-Task:** `FrequencyRuler.vue` Rendering-Logik analysieren.
Prüfen ob `zoomLevel` im Store verfügbar ist.

**Fix-Task:** `FrequencyRuler` neu: `zoomLevel`-basiertes Tick-Intervall.
Hauptticks (lang) + Nebenticks (kurz). Beschriftung kHz/MHz automatisch umschalten.

---

## Bug 6 — Bedienpanel (6.1–6.8)

**Betroffene Komponente:** `PluginView.vue` (`.kiwi-cpanel`)

### 6.1 Doppelter Collapse-Button
**IST:** ZWEI Buttons in Frequenz-Zeile: einer mit Pfeil rechts (funktionslos) + einer mit Pfeil links (funktioniert). **SOLL:** NUR EIN Button direkt rechts vom Extension-Dropdown, Pfeil nach RECHTS.

### 6.2 Band-Select leer
**IST:** Dropdown "select band" hat keine Optionen. **SOLL:** 87 Bänder (Broadcast: LW/MW/120m/... + Amateur/Utility). Auswahl → Zoom in Band-Bereich. Default: "select band" (kein Band).

### 6.3 Extension-Select leer
**IST:** Dropdown "extension" hat keine Optionen. **SOLL:** 27 Extensions (z.B. FFT). Extension-spezifische UI-Erweiterungen (→ M4x-Planung).

### 6.4 Zoom-Buttons: Lupe außerhalb
**IST:** Lupe-Symbol ragt über Button-Rand hinaus. **SOLL:** +/− + Lupe IM Button. Button-Größe 1:1 zum Web-UI.

### 6.5 Button-Anordnung / -Größe nicht 1:1
**IST:** Layout weicht von Web-UI ab. **SOLL:** Anordnung exakt wie KiwiSDR-Web-UI: Frequenz → Band-Select → Extension-Select → Collapse (1 Button!). Mode-Buttons in einer Reihe. Button-Größen aus Referenz (`panel.json` rect-Daten).

### 6.6 Spectrum-Button falsch
**IST:** Text-Label "Spectrum" (kein Button). **SOLL:** Button mit 3 Modi: "Spectrum" (default), "Spec RF" (Canvas oberhalb Frequenzleiste), "Spec AF" (Audio-Spektrum).

### 6.7 Audio-Button falsch
**IST:** ♪-Symbol. **SOLL:** Lautsprecher-Symbol. Grün = Audio an, Rot = Audio aus (Mute).

### 6.8 Tab-Inhalte unvollständig
**IST:** E2E-Tests haben scrollbare Inhalte nicht erfasst. **SOLL:** ALLE Parameter pro Tab (z.B. WF8-Tab: 20+ Parameter mit Dropdowns). Scrollbare Panel-Inhalte.

---

## E2E-Lückenanalyse

### Problem 1: Playwright sieht nur sichtbare Elemente
`page.locator()` erfasst nur Elemente im aktuellen Viewport. Scrollbare Tab-Inhalte (WF8 etc.)
wurden nicht gescrollt → nicht von E2E-Tests abgedeckt.

**Lösung:** `scrollIntoView()` vor jedem `locator()` ODER `page.evaluate()` für vollständigen
DOM-Snapshot (unabhängig von Scroll-Position).

### Problem 2: dx-selects-smeter.json hat leere Arrays
Alle `allOptions: []` — der Live-Capture-Helper hat Band/Extension/Dropdowns nicht erfasst.
**Lösung:** Capture-Script fixen ODER manuellen DOM-Export aus explore-8074.json vervollständigen.

### Problem 3: Keine visuellen Elemente pro Tab
Die E2E-Tests prüfen pro Tab nur 2-8 Elemente — im Web-UI sind es 10-20+ pro Tab.
**Lösung:** `it.each`-Tests pro Tab-Element-Kategorie.

---

## 1:1-Paritäts-Strategie

Um sicherzustellen dass Agents bauen und am Ende keine großen Unterschiede zum Web-UI bestehen:

1. **Referenz-DOM als Ground Truth:** `explore-8074.json` mit 127 Elementen + 272 IDs ist die
   autoritative Quelle für ALLE UI-Elemente. Jeder Fix muss gegen diese Referenz validiert werden.

2. **Analyse vor Fix:** Jeder Bug-Task beginnt mit einem Analyse-Subtask, der den Referenz-DOM
   abgleicht und den exakten Soll-Zustand dokumentiert. KEIN Fix ohne abgeschlossene Analyse.

3. **E2E-Test pro Element:** Nach jedem Fix wird ein E2E-Test geschrieben, der das Element
   gegen die Referenz prüft (Existenz, Position, Text, Verhalten).

4. **Visual Regression:** Baseline-Screenshots (`ui/e2e/reference/plugin-baseline-*.png`)
   werden nach jedem Fix aktualisiert und mit der Web-UI-Referenz verglichen.

5. **Referenz-Matrix als Checklist:** `doc/reference-matrix.md` (78 Elemente, 15 Kategorien)
   wird nach jedem Fix aktualisiert (❌ → ✅).

---

## Extensions-Planung (M4x)

> Keine Implementierung in M4c.7 — nur Planung und Analyse.

KiwiSDR-Extensions werden über `id-select-ext` (27 Optionen) ausgewählt.
Jede Extension erweitert das UI um spezifische Funktionalität:

- **FFT:** Canvas oberhalb der Frequenzband-Leiste — zeigt FFT des aktuell ausgewählten Signals
- **Spec RF:** Spectrum-Button → "Spec RF" — HF-Spektrum über der Frequenzleiste
- **Spec AF:** Spectrum-Button → "Spec AF" — Audio-Spektrum
- **Weitere Extensions:** `id-ext-controls` (x:10, y:490, w:525, h:300) — generisches Extension-Panel

**M4x-Tasks (spätere Phase):**
1. Alle 27 Extensions analysieren (`id-select-ext` Optionen)
2. Pro Extension UI-Erweiterung planen
3. Spectrum-Button 3 Modi implementieren
4. Extension-Panel (`id-ext-controls`) als Vue-Komponente
5. E2E-Tests pro Extension

---

## Chronologie

| Zeit | Ereignis |
|------|----------|
| 2026-08-28 | M4b-Bugs erfasst (M4b-bugs.md) |
| 2026-08-29 | M4c abgeschlossen (E2E-Tests, Bugfixes) — M4b-Bugs gefixt |
| 2026-08-29 | M4c.7 Bug-Manifest erstellt — 6 neue Bugs aus manueller Prüfung |
