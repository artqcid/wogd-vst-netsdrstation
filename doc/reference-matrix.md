---
type: Reference Matrix
title: Reference Matrix M4 — KiwiSDR Live ↔ PluginView 1:1
description: Mapping of every live KiwiSDR UI element to its PluginView equivalent; check-off status column
status: stable
generated:
  by: human:marku
  at: 2026-08-28
verified:
  by: human:marku
  at: 2026-08-29
tags: [m4, reference, ui-mapping, kiwisdr, check-off]
stale_after: 2026-09-30
---

# Reference Matrix M4 — KiwiSDR Live (8074) ↔ PluginView 1:1

## Quelle
Live-KiwiSDR: `http://kphsdr.com:8074` (KPH Kiwi74), Splash per Center-Click dismiss, Callsign "TestUser"

---

## 1. HEADER / TOPBAR

| # | Live (ID/Text) | Plugin (Selector) | Verhalten/SOLL | Geprüft |
|---|---|---|---|---|
| 1.1 | `#id-rx-title` "KPH Kiwi74" | `.kiwi-header__title` "NetSDRStation" | Stationstitel anzeigen | ❌ |
| 1.2 | `#id-rx-desc` Grid/SNR/Map | `.kiwi-header__sub` + `.kiwi-header__center-status` | Standort/Status | ❌ |
| 1.3 | `#id-rx-antenna` "Antenna: TCI-530..." | `.kiwi-header__sub` "Antenna: KiwiSDR broadband" | Antennen-Info | ❌ |
| 1.4 | `#id-owner-info` "MRHS" | — | Besitzer-Info | ❌ |
| 1.5 | `#id-ident-input1` (text, value="TestUser") | `.kiwi-header__callsign-input` (placeholder "callsign") | Callsign-Eingabe | ❌ |
| 1.6 | UTC time inline | `.kiwi-header__time-utc` | UTC-Zeit (aktualisiert) | ❌ |
| 1.7 | Local time inline | `.kiwi-header__time-local` | Lokalzeit | ❌ |
| 1.8 | Timezone inline | `.kiwi-header__timezone` | Zeitzone | ❌ |
| 1.9 | Kiwi-Logo (IMG #id-left-logo) | SVG `.kiwi-header__logo` | Logo | ❌ |

## 2. BAND SCALE

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 2.1 | `#id-band-canvas` (1280x30) | `<BandScaleBar>` | Bandblöcke + Pfeile | ❌ |
| 2.2 | Klick auf Band → Frequenz ändert | `@tune` → `onBandTune` | Tune | ❌ |
| 2.3 | Pfeile (← →) für Pan | `@pan` → `onPan` | Pan links/rechts | ❌ |

## 3. DX TAGS

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 3.1 | 73 Buttons `id-dx-label_0..72` | `<TagArea>` mit DEMO_TAGS (30 Stk) | DX-Tags anzeigen | ✅ (implementiert) |
| 3.2 | Klick DX-Tag → Frequenz (auf diesem Kiwi kein Tune) | `@tune` → `onTagTune` | Tune auf Tag-Freq | ✅ (TagPopup) |

## 4. FREQUENZ-BEREICH

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 4.1 | `#id-freq-input` Wert "7020.000" | `.kiwi-cpanel__freq-input` | Frequenz anzeigen/bearbeiten | ❌ |
| 4.2 | `#id-select-band` (87 Optionen) | `.kiwi-cpanel__select[aria-label="Band"]` | Band-Auswahl | ❌ |
| 4.3 | `#id-select-ext` (27 Optionen) | `.kiwi-cpanel__select[aria-label="Extension"]` | Extension-Auswahl | ❌ |
| 4.4 | VFO-Button `id-freq-vfo` "A" | — | VFO A/B toggle | ❌ (fehlt in Plugin?) |
| 4.5 | Play-Button im Panel `▶` | `.kiwi-cpanel__play-btn` | Audio Play | ❌ |
| 4.6 | Floating Play Button | `.kiwi-play-btn` ▶ links am Canvas | Audio Play | ❌ |

## 5. MODE BUTTONS (8)

| # | Live (ID) | Plugin (Text) | Verhalten | Geprüft |
|---|---|---|---|---|
| 5.1 | `#0-id-mode-col` "AM" | `.kiwi-cpanel__mode-btn` "AM" | Mode=0 | ❌ |
| 5.2 | `#1-id-mode-col` "SAM" | ".kiwi-cpanel__mode-btn" "SAM" | Mode=13 | ❌ |
| 5.3 | `#2-id-mode-col` "DRM" | ".kiwi-cpanel__mode-btn" "DRM" | Mode=12 | ❌ |
| 5.4 | `#3-id-mode-col` "LSB" | ".kiwi-cpanel__mode-btn" "LSB" | Mode=5 | ❌ |
| 5.5 | `#4-id-mode-col` "USB" | ".kiwi-cpanel__mode-btn" "USB" | Mode=3 | ❌ |
| 5.6 | `#5-id-mode-col` "CW" | ".kiwi-cpanel__mode-btn" "CW" | Mode=7 | ❌ |
| 5.7 | `#6-id-mode-col` "NBFM" | ".kiwi-cpanel__mode-btn" "NBFM" | Mode=9 | ❌ |
| 5.8 | `#7-id-mode-col` "IQ" | ".kiwi-cpanel__mode-btn" "IQ" | Mode=11 | ❌ |
| 5.9 | Aktiver Mode: Klasse `w3-hold-done` | Klasse `kiwi-cpanel__mode-btn--active` | Visuelles Highlight | ❌ |

## 6. STEP / ZOOM BUTTONS

| # | Live | Plugin (Text) | Verhalten | Geprüft |
|---|---|---|---|---|
| 6.1 | — | `title="−10 kHz"` | stepFreq(-1, 10) | ❌ |
| 6.2 | — | `title="−1 kHz"` | stepFreq(-1, 1) | ❌ |
| 6.3 | — | `title="−0.1 kHz"` | stepFreq(-1, 0.1) | ❌ |
| 6.4 | — | `title="+0.1 kHz"` | stepFreq(1, 0.1) | ❌ |
| 6.5 | — | `title="+1 kHz"` | stepFreq(1, 1) | ❌ |
| 6.6 | — | `title="+10 kHz"` | stepFreq(1, 10) | ❌ |
| 6.7 | — | `title="Zoom in"` 🔍+ | onZoom(1) | ❌ |
| 6.8 | — | `title="Zoom out"` 🔍− | onZoom(-1) | ❌ |
| 6.9 | — | `title="Max zoom out"` ↖↙ | onZoomTo(0) | ❌ |
| 6.10 | — | `title="Max zoom in"` ↗↘ | onZoomTo(14) | ❌ |
| 6.11 | — | `title="Zoom to band"` ↔ | onZoomToBand() | ❌ |
| 6.12 | — | `title="Pan left"` ◀ | onPan(-1) | ❌ |
| 6.13 | — | `title="Pan right"` ▶ | onPan(1) | ❌ |
| 6.14 | — | `title="CIC compensation"` ↺ | onToggleCic() | ❌ |
| 6.15 | — | `title="Reset"` ↻ | onResetWf() | ❌ |
| 6.16 | — | `title="Audio"` ♪ | onToggleAudio() | ❌ |
| 6.17 | — | `title="Menu"` ☰ | (noch nicht implementiert) | ❌ |
| 6.18 | — | `.kiwi-cpanel__icon--cyan` "A" | (Users) | ❌ |
| 6.19 | — | `.kiwi-cpanel__icon--green` "✓" | (Status) | ❌ |
| 6.20 | — | `.kiwi-cpanel__icon--green` "9" | (Active receivers) | ❌ |

## 7. SUB-TABS (7)

| # | Live (Nav-ID) | Plugin (button Text) | Inhalt | Geprüft |
|---|---|---|---|---|
| 7.1 | `#id-nav-optbar-rf` | "RF" | RF-Einstellungen (Mode/Attn) | ❌ |
| 7.2 | `#id-nav-optbar-wf` | "WF0" | WF ceil/floor/rate, Spec Δ, Auto Scale, Spec Color, P1 | ❌ |
| 7.3 | `#id-nav-optbar-audio` | "Audio" | Volume, Mute, NR, Compression, De-emphasis | ❌ |
| 7.4 | `#id-nav-optbar-agc` | "AGC" | AGC On/Off, Threshold, Decay, Hang, Slope, Man Gain | ❌ |
| 7.5 | `#id-nav-optbar-user` | "User" | Squelch, NB, Squelch threshold, NB threshold | ❌ |
| 7.6 | `#id-nav-optbar-stat` | "Stat" | GPS, Users, Buffer, SNR | ❌ |
| 7.7 | `#id-nav-optbar-off` | "Off" | MUTE, Audio disabled | ❌ |
| 7.8 | Aktiver Tab: Klasse `w3-hold-done` | Klasse `kiwi-cpanel__tab-btn--active` | Visuelles Highlight | ❌ |

## 8. WF0-TAB-INHALT (Live vs Plugin)

| # | Live (ID) | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 8.1 | `#id-input-ceildb` range (min=-10,max=0,wert=-5) | `.kiwi-cpanel__slider` (WF ceil) | wfMaxDb slider | ❌ |
| 8.2 | Anzeige WF ceil value | `.kiwi-cpanel__ctrl-val` | +X dB | ❌ |
| 8.3 | `#id-input-floordb` range (min=-160,max=-60) | `.kiwi-cpanel__slider` (WF floor) | wfMinDb slider | ❌ |
| 8.4 | Anzeige WF floor value | `.kiwi-cpanel__ctrl-val` | X dB | ❌ |
| 8.5 | `#id-slider-rate` range (min=0,max=4) | `.kiwi-cpanel__slider` (WF rate) | wfSpeed slider | ❌ |
| 8.6 | Anzeige WF rate (slow/med/fast/max) | `.kiwi-cpanel__ctrl-val` | Text | ❌ |
| 8.7 | `#id-wf-sp-slider` range Spec Δ | `.kiwi-cpanel__slider` (Spec Δ) | wfSpSlider | ❌ |
| 8.8 | "AutoScale"-Button `<button id-button-wf-autoscale>` | `.kiwi-cpanel__btn--green` "Auto Scale" | Auto Scale | ❌ |
| 8.9 | "SpecColor"-Button `<button id-button-spec-color>` | `.kiwi-cpanel__btn--gray` "Spec Color" | Spec Color | ❌ |
| 8.10 | "P1"-Button `<button id-button-spec-peak0>` | `.kiwi-cpanel__btn--violet` "P1" | Peak 1 | ❌ |
| 8.11 | Colormap-Bar (Gradient) | `.kiwi-cpanel__colormap` | Colormap-Anzeige | ❌ |

## 9. AUDIO-TAB-INHALT

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 9.1 | Volume-Slider (min=0,max=100) | `.kiwi-cpanel__slider` Volume | volume slider | ❌ |
| 9.2 | Volume-Anzeige "%" | `.kiwi-cpanel__ctrl-val` | XX% | ❌ |
| 9.3 | Mute-Button (toggle mute) | `.kiwi-cpanel__btn--red` Mute/Unmute | setParam('mute') | ❌ |
| 9.4 | NR-Button (toggle nrOn) | `.kiwi-cpanel__btn` NR ON/OFF | setParam('nrOn') | ❌ |
| 9.5 | Compression-Button "Comp" | `.kiwi-cpanel__btn--gray` "OFF" | "Not yet implemented" | ❌ |
| 9.6 | De-emphasis-Button | `.kiwi-cpanel__btn--gray` "OFF" | "Not yet implemented" | ❌ |

## 10. AGC-TAB-INHALT

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 10.1 | AGC-Button (toggle) | `.kiwi-cpanel__btn` AGC ON/OFF | setParam('agcOn') | ❌ |
| 10.2 | Threshold-Slider (min=-140,max=0) | `.kiwi-cpanel__slider` Threshold | agcThresh | ❌ |
| 10.3 | Decay-Slider (min=20,max=5000,step=10) | `.kiwi-cpanel__slider` Decay | agcDecay | ❌ |
| 10.4 | Hang-Button (toggle) | `.kiwi-cpanel__btn` Hang ON/OFF | setParam('agcHang') | ❌ |
| 10.5 | Slope-Slider (min=0,max=100) | `.kiwi-cpanel__slider--sm` Slope | agcSlope | ❌ |
| 10.6 | Man Gain-Slider (min=0,max=100) | `.kiwi-cpanel__slider` Man Gain | agcManGain | ❌ |

## 11. USER-TAB-INHALT

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 11.1 | Squelch-Button (toggle) | `.kiwi-cpanel__btn` Squelch ON/OFF | setParam('squelchOn') | ❌ |
| 11.2 | Squelch-Slider (min=0,max=1,step=0.01) | `.kiwi-cpanel__slider--sm` Squelch | squelchThr | ❌ |
| 11.3 | NB-Button (toggle) | `.kiwi-cpanel__btn` NB ON/OFF | setParam('nbOn') | ❌ |
| 11.4 | NB-Slider (min=0,max=100) | `.kiwi-cpanel__slider--sm` NB | nbThresh | ❌ |

## 12. STAT-TAB-INHALT

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 12.1 | GPS-Status | `.kiwi-cpanel__ctrl-val` "locked (8 sats)" | Anzeige | ❌ |
| 12.2 | Users | `.kiwi-cpanel__ctrl-val` "0/4" | Anzeige | ❌ |
| 12.3 | Buffer | `.kiwi-cpanel__ctrl-val--green` "OK" | Anzeige | ❌ |
| 12.4 | SNR | `.kiwi-cpanel__ctrl-val` "32 dB" | Anzeige | ❌ |

## 13. OFF-TAB-INHALT

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 13.1 | MUTE-Button (set mute=1) | `.kiwi-cpanel__btn--red` "MUTE" | setParam('mute', 1) | ❌ |
| 13.2 | Hinweis "Audio output disabled" | span "Audio output disabled" | Anzeige | ❌ |

## 14. ROW 11 — DROPDOWNS + S-METER

| # | Live (Select-ID) | Plugin (Select) | Optionen | Geprüft |
|---|---|---|---|---|
| 14.1 | `#id-wf.cmap` Colormap | `.kiwi-cpanel__select--sm` (1. Select) | Kiwi, CSDR, grey, linear, turbo, SdrDx, cust1-4 | ❌ |
| 14.2 | `#id-wf.aper` Aperture | `.kiwi-cpanel__select--sm` (2. Select) | man, auto | ❌ |
| 14.3 | `#id-wf_filter` WF Filter | `.kiwi-cpanel__select--sm` (3. Select) | IIR, MMA, EMA, off | ❌ |
| 14.4 | `#id-spec_filter` Spec Filter | `.kiwi-cpanel__select--sm` (4. Select) | IIR, MMA, EMA, off | ❌ |
| 14.5 | `id-button-spec-peak1` "P2" | `.kiwi-cpanel__btn--violet` "P2" | Peak 2 | ❌ |
| 14.6 | `#id-smeter-scale` Canvas (355x37) | `.kiwi-cpanel__smeter` | S-Meter Anzeige | ❌ |
| 14.7 | dBm-Wert (signalLevel) | `.kiwi-cpanel__smeter-dbm` | Signalpegel | ❌ |
| 14.8 | S-Skala (S1..S9, +10..+60) | `.kiwi-cpanel__smeter-labels` Labels | S-Skala | ❌ |

## 15. CANVAS — WASSERFALL

| # | Live | Plugin | Verhalten | Geprüft |
|---|---|---|---|---|
| 15.1 | `#id-wf-canvas` (1280x200) | `<Waterfall>` | Wasserfall | ❌ |
| 15.2 | `#id-annotation-canvas` (1280x576) | (innen in Waterfall) | Annotation | ❌ |
| 15.3 | `id-scale-canvas` (1280x47) | `<FrequencyRuler>` | Frequenz-Lineal | ❌ |
| 15.4 | `id-band-canvas` (1280x30) | `<BandScaleBar>` | Band-Skala | ❌ |
| 15.5 | Ctrl+Wheel → Zoom | `@zoom` → `onWfZoom` | Zoom + Anchor | ❌ |
| 15.6 | Ruler-Klick → Tune | `@tune` → `onFreqRulerTune` | Frequenz setzen | ❌ |

---

## Zusammenfassung

**Gesamt: 78 Elemente** (inkl. Sub-Elemente in Tabs)
**Geprüft (✅):** 2 (TagPopup implementiert, Tag-Klick)
**Nicht geprüft (❌):** 76

**E2E-Tests zu schreiben:** 
- Header (1.1-1.9): 9 Tests
- BandScale (2.1-2.3): 3 Tests
- DX Tags (3.1-3.2): 2 Tests (✅ Popup)
- Freq-Bereich (4.1-4.6): 6 Tests
- Mode (5.1-5.9): 9 Tests
- Step/Zoom/Icons (6.1-6.20): 20 Tests
- Sub-Tabs (7.1-7.8): 8 Tests
- WF0 (8.1-8.11): 11 Tests
- Audio (9.1-9.6): 6 Tests
- AGC (10.1-10.6): 6 Tests
- User (11.1-11.4): 4 Tests
- Stat (12.1-12.4): 4 Tests
- Off (13.1-13.2): 2 Tests
- Dropdowns + S-Meter (14.1-14.8): 8 Tests
- Canvas (15.1-15.6): 6 Tests