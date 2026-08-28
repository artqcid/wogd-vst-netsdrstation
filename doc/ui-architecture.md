# NetSDRStation-VST - UI Architecture

_Architecture and inventory of the plugin UI. The UI is a Vue 3 + Vite single
file bundle rendered in an OS-native WebView2 (see `doc/architecture.md` §8).
It mirrors the **KiwiSDR browser interface** (the reference SDR client)._

_Complements `doc/architecture.md` (system architecture), `doc/checklist.md`
(open tasks, see M3), and `doc/workspace-workflow.md` (build/debug)._

## 1. Purpose

The NetSDRStation plugin exposes a KiwiSDR receiver as a DAW audio source. Its
UI therefore has two responsibilities:

1. **Receiver control** — tune, select mode/passband, AGC, noise processing
   (the "radio" controls).
2. **Audio metering** — signal strength (S-meter), connection/stream status.

The M3 milestone wires the receiver pipeline into the processor (M3.1), adds
the full parameter set (M3.2) and builds the Kiwi-style UI (M3.3). This file
is the single inventory of every control element the KiwiSDR browser provides,
so the Vue UI can be implemented against a complete reference.

## 2. Reference

- **Source:** KiwiSDR browser interface (open source,
  `github.com/jks-prv/Beagle_SDR_GPS`, `web/kiwi/`):
  - `kiwi.js` — main UI logic / control panels.
  - `kiwi_ui.js` — DX label editor + SD backup UI.
  - `waterfall.js` — waterfall/spectrum controls.
  - `noise_blank.js` / `noise_filter.js` — NB / NR extension panels.
  - `ant_switch.js`, `kiwi_map.js` — antenna switch + receiver map.
  - `w3_util.js` — the w3 widget library (sliders, selects, checkboxes).
- **Protocol:** KiwiSDR `SET ...` WebSocket commands
  (`github.com/jks-prv/Beagle_SDR_GPS`, `rx/rx_cmd.cpp`); see
  `doc/architecture.md` §6.

> The list below is exhaustive for the standard browser UI. The concrete test
> station (`kphsdr.com:8072`) serves the same standard interface.

## 3. UI Elements

### 3.1 Frequency & Tuning

| Element | Type | Kiwi command | Notes |
|---|---|---|---|
| Frequency input field | text input (kHz) | `SET ... freq=<kHz>` | manual entry, accepts units (`k`/`M`) |
| Step tuning buttons | 6 buttons | `SET ... freq=` | `-10`, `-1`, `-0.1`, `+0.1`, `+1`, `+10` kHz |
| Passband dragger | waterfall overlay | `SET ... freq=` | yellow drag bar on the frequency scale |
| Large frequency display | readout | — | exact Hz/kHz digital readout |

### 3.2 Modulation & Passband

| Element | Type | Kiwi command | Notes |
|---|---|---|---|
| Mode buttons | radio/buttons | `SET mod=<mode>` | 18 modes, see list below |
| Low Cut field | number input (Hz) | `SET ... low_cut=<Hz>` | lower passband edge |
| High Cut field | number input (Hz) | `SET ... high_cut=<Hz>` | upper passband edge |
| Bandwidth field | number input (Hz) | `SET ... low_cut/high_cut` | total passband width |
| Filter reset button | button | — | reset passband to the mode default |

**Mode list (18):** `AM`, `AMN` (AM narrow), `AMW` (AM wide), `USB`, `USN`
(USB narrow), `LSB`, `LSN` (LSB narrow), `CW`, `CWN` (CW narrow), `NBFM`,
`NNFM`, `IQ`, `DRM`, `SAM`, `SAU` (SAM upper), `SAL` (SAM lower), `SAS`
(SAM stereo), `QAM`.

### 3.3 Band Presets & Memory

| Element | Type | Notes |
|---|---|---|
| Band dropdown | select | Amateur (160m, 80m, 60m, 40m, 30m, 20m, 17m, 15m, 12m, 10m) |
| Band dropdown (cont.) | select | Broadcast (LW, MW, SW 49m, 41m, 31m, 25m, 19m, 16m, 13m, 11m) |
| Band dropdown (cont.) | select | Utility / time signals (DCF77, WWV, …) |
| Memory / bookmark list | select | saved presets & frequencies |

### 3.4 Waterfall & Spectrum Display

| Element | Type | Kiwi command | Notes |
|---|---|---|---|
| Zoom buttons | buttons | `SET zoom=<0..14>` | `+`, `-`, `Max In`, `Max Out` |
| Zoom level indicator | readout | — | `z0` … `z14` |
| WF Max (dBFS) | slider + number | `SET maxdb=<dB>` | upper color contrast bound |
| WF Min (dBFS) | slider + number | `SET mindb=<dB>` | lower noise floor bound |
| WF Speed | dropdown | `SET wf_speed=<fps>` | Pause, Slow, Med, Fast |
| Color map | dropdown | `SET cmap=<n>` | Default, Rain, Soft, High Contrast, Grayscale, … |
| Display mode toggle | toggle | `SET WF=<0|1>` | WF / Spec / Both |
| Averaging / floor | slider | — | spectrum line smoothing |
| Aperture auto mode | select + slider + button | `SET aperture=...` | algorithm IIR/MMA/EMA/off, parameter, Auto Scale |
| Spec RF passband marker | color + checkbox | `SET spb=...` | marker overlay color + enable |
| Timestamps | select + field | `SET tstamp=...` | off/2s/…/60m/custom; UTC/local |
| Save WF as JPG | button | — | export waterfall snapshot |
| Waterfall FFT window | select | `SET window_func=<n>` | Hanning, Hamming, Blackman-Harris, none |
| Waterfall interpolation | select | `SET interp=<n>` | max, min, last, drop samp, CMA |
| CIC compensation | checkbox | `SET interp=<n>` | CIC filter compensation toggle |

### 3.5 Audio, AGC & Signal Processing

| Element | Type | Kiwi command | Notes |
|---|---|---|---|
| Volume | slider + number | `SET volume=<0..100>` | master output volume |
| Mute | button | `SET volume=0` / local | mutes audio output |
| Audio start overlay | button | — | browser autoplay unlock ("Click to start audio") |
| AGC On/Off | toggle | `SET agc=<0|1>` | automatic gain control |
| AGC Threshold | slider | `SET agc=... thresh=<dB>` | threshold in dB |
| AGC Decay | slider | `SET agc=... decay=<ms>` | decay time in ms |
| AGC Hang | toggle | `SET agc=... hang=<0|1>` | AGC hang |
| AGC Slope / Manual Gain | slider (adv.) | `SET agc=... slope/manGain` | advanced AGC |
| Squelch On/Off | toggle | `SET squelch=<0|1>` | noise squelch |
| Squelch Threshold | slider | `SET squelch=... sq_threshold` | threshold |
| Noise Blanker (NB) | toggle + threshold | `SET nb=<0|1> nb_threshold` | noise blanker (extension `noise_blank.js`) |
| Noise Reduction (NR) | toggle | `SET nr=<0|1>` | denoise (extension `noise_filter.js`) |
| S-Meter | bar + digital | — | S1–S9, +10…+60 dB, dBm value |

### 3.6 Extensions & Special Tools (Extension Panel)

| Element | Type | Notes |
|---|---|---|
| Extension select | dropdown | CW Decoder, WFAX (weather fax), RTTY/FSK, SSTV, tDoA (geolocation), IQ Display, Antenna Switch (if HW) |
| Dynamic control panel | panel | per-tool controls & readouts |

### 3.7 Status & System

| Element | Type | Notes |
|---|---|---|
| User status | readout | active listener slots, e.g. `0/4 Users` |
| GPS sync status | indicator | green/red, GPSDO satellite count |
| Audio buffer indicator | readout | stream / latency status |
| Exact frequency | readout | see §3.1 |

## 4. Mapping to VST3 parameters (M3.2)

Not every UI element maps 1:1 to a VST3 parameter. The VST3 parameter model
(`source/vst/common/paramdefinitions.h`) is the DAW-automatable subset; the
UI is a thin view over it plus display-only readouts.

| UI element | VST3 parameter (M3.2) | Automatable |
|---|---|---|
| Frequency input / step buttons / dragger | `freqKhz` | yes |
| Mode buttons | `mode` (enum) | yes |
| Low Cut / High Cut / Bandwidth | `lowCut`, `highCut` | yes |
| Volume | `volume` | yes |
| AGC On/Off / Threshold / Decay / Hang / Slope / ManGain | `agcOn`, `agcThresh`, `agcDecay`, `agcHang`, `agcSlope`, `agcManGain` | yes |
| Squelch On/Off / Threshold | `squelchOn`, `squelchThreshold` | yes |
| Noise Blanker | `nbOn`, `nbThreshold` | yes |
| Noise Reduction | `nrOn` | yes |
| Waterfall on/off, speed, zoom, max/min, comp | `wfOn`, `wfSpeed`, `wfZoom`, `wfMaxDb`, `wfMinDb`, `wfComp` | yes |
| S-Meter | — (display only) | no |
| GPS / user / buffer status | — (display only) | no |
| Band presets / memory | — (UI-side convenience) | no |
| Extensions (CW, WFAX, RTTY, …) | — (deferred, see §5) | no |

## 5. Scope decisions for the plugin UI

- **M3.3 implements** the Core + AGC + waterfall-on/off subset (see
  `doc/checklist.md` M3.3); the remaining parameters stay DAW-automatable with
  default UI values.
- **Deferred (not in M3):** waterfall full display (Spectrum/Waterfall
  rendering), color maps, FFT window/interpolation, timestamps, JPG export,
  band presets/bookmarks, and the extension decoder panels (CW/WFAX/RTTY/
  SSTV/tDoA/IQ). These require spectrum data the audio-only pipeline does not
  yet deliver.
- The **S-Meter** is display-only and is planned as an early UI addition since
  it needs only the audio level (available after M3.1).

## 6. Top-level layout & window behaviour

### 6.1 Tabs (M5)

The plugin UI is organised into two tabs:

- **Tab 1 "SDR Stations"** — station selection. Fetches the public KiwiSDR
  receiver directory and shows it as a scrollable list (name, location,
  frequency coverage, SNR, user count, online/offline). Clicking a station
  connects to it.
- **Tab 2 "KIWI UI"** — the receiver interface (M4, the §3 element set).

Default state: **no station loaded**. Tab 2 then shows only the message
"please select station first"; the receiver controls/readouts render only
after a station is connected (see `doc/checklist.md` M5).

### 6.2 Resizable window (M4.1 Grundbedingung)

The editor must be freely resizable by dragging the bottom-right corner
(standard VST3 host behaviour). On the C++ side the host resize is forwarded
to the WebView2 widget so the view fills the client area (extends FIX-22); on
the UI side the Vue layout is fully responsive (fluid grid/flex, no
hard-coded pixel dimensions) and reflows continuously at any size. The only
clamp is the documented `kMinWidth`/`kMinHeight` floor
(`source/editor/plugin_editor.cpp`).

---

## 7. Complete GUI Element Inventory (KiwiSDR reference → Vue implementation)

_Updated 2026-08-28. Every GUI element that exists in the KiwiSDR web UI or
must exist in the VST plugin UI. "Impl" = implementation status:_
_✅ done · ⚠️ partial · ❌ missing · 🔧 needs fix_

### 7.1 Header Bar (`PluginView.vue .kiwi-header`)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Kiwi bird logo (green circle) | branding, no interaction | ✅ | SVG inline |
| Station name + sub-info | shows connected station, antenna, SNR | ⚠️ | SNR missing |
| Host/center info | station hostname:port, share text | ⚠️ | static text |
| StationInput (URL field + Connect/Disconnect) | user enters station URL, triggers connect | ✅ | |
| Status badge | connection state (Connecting/Connected/Error) | ⚠️ | text only, no colour |
| Your name / callsign input | sent to KiwiSDR server as user ID | ⚠️ | field exists, not wired |
| UTC clock | displays current UTC time, updates every second | ✅ | |
| Local time + timezone | local time + tz name | ✅ | |

### 7.2 Band Scale Strip (`BandScaleBar.vue .kiwi-bandscale`)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Band blocks (proportional width) | shows each frequency band proportional to actual bandwidth | 🔧 | Bug 3: hardcoded positions |
| Band block colours | Broadcast=teal, Amateur=pink/red, Utility=yellow, Maritime=blue | ⚠️ | colours exist, layout wrong |
| Scroll arrows (◀ ▶) | pan band scale left/right if wider than viewport | ❌ | not implemented |
| Click on band block | sets `freqKhz` to band centre, adjusts zoom to show full band | ❌ | Bug 3 |
| Zoom-reactive layout | blocks reposition/resize when `wfZoom` changes | ❌ | Bug 8 |

### 7.3 DX Tag / Station Label Area (`TagArea.vue .kiwi-tagarea`)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| DX tags (coloured pills) | shows broadcast stations/DX spots at their frequencies | ⚠️ | demo data only |
| Tag position (proportional) | x-position derived from `freqKhz` relative to visible span | ❌ | Bug 8 |
| Click on tag → jump to frequency | sets `store.freqKhz` to `tag.freqKhz` | ❌ | Bug 4 |
| Tag popup on click | modal with station name, frequency, language, time info | ❌ | Bug 4 |
| EiBi/SWBC dataset | real broadcast schedule data | ❌ | only 7 hard-coded demo tags |
| Multi-row layout (staggered) | tags that overlap stack into multiple rows | ⚠️ | partial |

### 7.4 Frequency Ruler (`FrequencyRuler.vue .kiwi-freq-ruler`)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Tick marks + frequency labels | dynamic, computed from loKhz/hiKhz + zoom | ⚠️ | static 0–30 MHz |
| Zoom-reactive ticks | more ticks at higher zoom levels | ❌ | Bug 8 |
| Frequency cursor (yellow, low zoom) | Λ-shaped yellow dragger at current `freqKhz` | ❌ | Bug 5 |
| Frequency cursor (green, high zoom) | bracket showing passband LoKhz–HiKhz | ❌ | Bug 5 |
| Drag cursor to tune | mousedown+move → `store.setParam('freqKhz', ...)` | ❌ | Bug 5 |
| Drag Lo/Hi edge | mousedown+move on bracket edges → `lowCut`/`highCut` | ❌ | Bug 5 |
| Ctrl+Wheel zoom | zoom in/out centred on cursor position | ❌ | Bug 2 |
| Database label | shows active DX database name ("database: EiBi-A26") | ⚠️ | static text |

### 7.5 Waterfall Canvas (`Waterfall.vue`)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Waterfall pixel rows | each frame pushed from DSP, scrolls downward | ✅ | |
| Spectrum overlay | spectrum line above waterfall (optional mode) | ❌ | not implemented |
| Passband shade (green overlay) | semi-transparent green rectangle between Lo/Hi cut | ✅ | `drawOverlay()` |
| Frequency cursor line (yellow) | vertical yellow line at `cursorKhz` | ✅ | `drawOverlay()` |
| Ctrl+Wheel zoom | zoom in/out at mouse anchor position → `store.wfZoom` | ❌ | Bug 2 (emitted but not handled) |
| Wheel zoom without Ctrl | pan left/right | ❌ | not implemented |
| Span-reactive rendering | bins mapped to `loKhz..hiKhz` visible range | 🔧 | Bug 8: no spanKhz prop |
| Click to tune | click sets `freqKhz` to clicked frequency | ❌ | not implemented |
| Play button (left edge) | violet circle with ► ; unlocks browser audio, starts stream | 🔧 | shows `?` instead of icon |

### 7.6 Control Panel (`PluginView.vue .kiwi-cpanel`)

#### Row 1 — Frequency + Dropdowns

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Frequency input field | text entry in kHz, Enter/blur → `store.freqKhz` | ✅ | |
| Band select dropdown | selects amateur/broadcast/utility band → sets freq+zoom | ❌ | Bug 6.5: placeholder only |
| Extension select dropdown | opens extension decoder panel | ❌ | Bug 6.6: placeholder only |
| Play/audio button | start/stop audio stream | ⚠️ | no icon, no function |

#### Row 2 — Toolbar Icons

| Element | Function | Impl | Note |
|---------|----------|------|------|
| ≡ Menu icon | opens settings/menu overlay | ❌ | no function |
| A (cyan circle) | user count / ID indicator | ❌ | static, no function |
| ✓ (green) | connection/GPS lock indicator | ❌ | static |
| 9 (green number) | active receiver slot count | ❌ | static |
| Zoom in (+🔍) | `wfZoom + 1` | ⚠️ | icon missing (`?`) |
| Zoom out (−🔍) | `wfZoom - 1` | ⚠️ | icon missing (`?`) |
| Max zoom in (↘↗) | `wfZoom = 14` | ⚠️ | wrong icon + order (Bug 6.2) |
| Max zoom out (↖↙) | `wfZoom = 0` | ⚠️ | wrong icon + order (Bug 6.2) |
| Zoom to band (↔) | set zoom/freq to show current band fully | ❌ | Bug 6.2 |
| Pan left (◀) | shift visible range left | ❌ | Bug 6.2 |
| Pan right (▶) | shift visible range right | ❌ | Bug 6.2 |
| CIC comp toggle | toggle `store.wfComp` | ⚠️ | icon missing |
| "Spectrum" label | WF/Spec/Both display mode selector | ❌ | static label, no toggle |
| Reset (red 🔄) | reset WF params to defaults | ❌ | no function |
| Audio (green 🔊) | audio on/off | ❌ | no function |

#### Row 3 — Mode Buttons

| Element | Function | Impl | Note |
|---------|----------|------|------|
| AM button | set mode=0, apply AM default passband | ✅ | |
| SAM button | set mode=13 | ✅ | |
| DRM button | set mode=12 | ✅ | |
| LSB button | set mode=5 | ✅ | |
| USB button | set mode=3 | ✅ | |
| CW button | set mode=7 | ✅ | |
| NBFM button | set mode=9 | ✅ | |
| IQ button | set mode=11 | ✅ | |
| AMN, AMW, USN, LSN, CWN, NNFM, SAU, SAL, SAS, QAM | remaining 10 modes | ❌ | not shown in panel |

#### Row 4 — Navigation / Frequency Step Buttons

| Element | Function | Impl | Note |
|---------|----------|------|------|
| −10 kHz button (large) | `freqKhz − 10` | ❌ | Bug 6.3 |
| −1 kHz button (medium) | `freqKhz − 1` | ⚠️ | `stepFreq(-1)` exists |
| −0.1 kHz button (small) | `freqKhz − 0.1` | ❌ | Bug 6.3 |
| +0.1 kHz button (small) | `freqKhz + 0.1` | ❌ | Bug 6.3 |
| +1 kHz button (medium) | `freqKhz + 1` | ⚠️ | `stepFreq(1)` exists |
| +10 kHz button (large) | `freqKhz + 10` | ❌ | Bug 6.3 |

#### Row 5 — Sub-Tabs (RF / WF0 / Audio / AGC / User / Stat / Off)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Tab buttons (7) | switch active tab | ✅ | colours correct |
| Tab label "WF9" | shows waterfall slot number | ❌ | shows "WF0" always |

#### Row 6 — Colormap bar

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Colormap gradient bar | preview of active colormap | ⚠️ | empty div |
| Click on bar | opens colormap selector? | ❌ | |

#### Tab Content: RF / WF0

| Element | Function | Impl | Note |
|---------|----------|------|------|
| WF ceil slider | `wfMaxDb` (−10..0 dB) | ✅ | |
| WF floor slider | `wfMinDb` (−160..−60 dB) | ✅ | |
| WF rate slider | `wfSpeed` (pause/slow/med/fast/max) | ✅ | |
| Spec Δ slider | spectrum gain (0..2) | ⚠️ | not wired to store |
| Auto Scale button | auto-fit WF ceil/floor to signal | ❌ | no function |
| Spec Color button | toggle spectrum line colour | ❌ | no function |
| P1 button | save WF preset 1 | ❌ | no function |
| Colormap dropdown | select colormap (Kiwi/Rain/Grey/…) | ⚠️ | no effect on Waterfall |
| Aperture dropdown | IIR/MMA/EMA/off → `wfComp` algorithm | ❌ | no function |
| Timestamp dropdown | off/2s/5s/… | ❌ | no function |
| FFT window dropdown | IIR/MMA/EMA | ❌ | no function |
| P2 button | save WF preset 2 | ❌ | no function |

#### Tab Content: Audio (not yet implemented)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Volume slider | `volume` (0..100) | ❌ | Bug 6.4 |
| Mute button | `volume = 0` | ❌ | |
| Compression toggle | audio compression on/off | ❌ | |
| De-emphasis toggle | de-emphasis filter | ❌ | |
| NR (Noise Reduction) toggle | `nrOn` | ❌ | |

#### Tab Content: AGC (not yet implemented)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| AGC On/Off toggle | `agcOn` | ❌ | Bug 6.4 |
| Threshold slider | `agcThresh` (−140..0 dB) | ❌ | |
| Decay slider | `agcDecay` (20..5000 ms) | ❌ | |
| Hang toggle | `agcHang` | ❌ | |
| Slope slider | `agcSlope` | ❌ | |
| Manual Gain slider | `agcManGain` | ❌ | |

#### Tab Content: User (not yet implemented)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| Squelch On/Off | `squelchOn` | ❌ | Bug 6.4 |
| Squelch threshold | `squelchThreshold` | ❌ | |
| NB (Noise Blanker) On/Off | `nbOn` | ❌ | |
| NB threshold | `nbThreshold` | ❌ | |

#### Tab Content: Stat (not yet implemented)

| Element | Function | Impl | Note |
|---------|----------|------|------|
| GPS lock status | GPS satellite count + lock indicator | ❌ | Bug 6.4 |
| User count | active listeners / max slots | ❌ | |
| Audio buffer status | stream buffer health | ❌ | |
| SNR readout | signal-to-noise ratio | ❌ | |

#### S-Meter Footer

| Element | Function | Impl | Note |
|---------|----------|------|------|
| S1–S9 scale labels | reference marks for signal strength | ✅ | |
| +10/+20/+40/+60 labels | above-S9 extension | ✅ | |
| dBm digital readout | exact signal level | ✅ | |
| S-meter bar (fill) | animated bar proportional to signal level | ✅ | |

---

## 8. Known Bugs (M4b)

See `doc/M4b-bugs.md` for the full bug list with root causes, fix estimates,
and implementation order. Summary:

| Bug | Short title | Priority |
|-----|-------------|----------|
| 1 | Scale-Transform must be removed (App.vue) | Critical |
| 2 | Ctrl+Wheel zoom not connected to store | High |
| 3 | BandScaleBar wrong layout + no click-to-tune | High |
| 4 | TagArea no click-to-tune, no popup | Medium |
| 5 | FrequencyRuler no cursor/dragger | High |
| 6 | ControlPanel: icons missing, buttons wrong/unimplemented | Medium |
| 7 | Visual deviations from original | Low |
| 8 | Zoom/span architecture: no spanKhz in store/props | High |
| 9 | All Playwright E2E tests outdated | High |
