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
> station (`g8ure.ddns.net:8078`) serves the same standard interface.

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
