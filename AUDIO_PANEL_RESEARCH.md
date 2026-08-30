# KiwiSDR Audio Panel - Source Code Research Report

## Files Analyzed

1. **openwebrx.js** (KiwiSDR master, raw GitHub) — main OpenWebRX/KiwiSDR code
2. **kiwi.js** (KiwiSDR master, raw GitHub) — core KiwiSDR framework
3. **kiwi.css** — styles
4. **w3_ext.css** — UI extension styles
5. **Live page** — http://kiwisdr.areg.org.au:8073/ (SPA, minimal initial HTML)

---

## 1. Complete List of Audio Parameters

### 1.1 Volume Control
- **ID:** `id-input-volume`
- **Type:** Slider (w3_slider)
- **Range:** 0–200 (integer)
- **Default:** 50 (from `kiwi_storeRead('last_volume', 50)`)
- **Callback:** `setvolume_cb`
- **Internal representation:** `kiwi.volume` (0–200), `kiwi.volume_f` (0–2.0, gain factor)
- **SET command:** Indirect — gain applied via AudioContext; sent as parameter in audio stream
- **Wheel callback:** `setvolume_wheel_cb` (step 5–15)
- **Code location:** `openwebrx.js:2610-2612`

### 1.2 Squelch
- **ID:** `id-squelch-value`
- **Type:** Slider (w3_slider)
- **Range:** 0–99 for NBFM, 0–40 for other modes
- **Default:** 0
- **Variable:** `squelch` (global, float)
- **Callback:** `set_squelch_cb`
- **Wheel callback:** `set_squelch_wheel_cb` (step 1, multiplier 5)
- **Toggle:** `squelch_zero_cb` — toggles squelch enable/disable (icon turns lime/magenta)
- **SET command:** `snd_send("SET squelch=" + squelch.toFixed(0) + ' param=' + (nbfm ? squelch_threshold : squelch_tail_v[squelch_tail]).toFixed(2))`
- **Code location:** `openwebrx.js:2527, 2645-2660`

**Squelch tail (dropdown):**
- **ID:** `id-squelch-tail`
- **Options:** `['0s', '.2s', '.5s', '1s', '2s']`
- **Values:** `[0, 0.2, 0.5, 1, 2]` (seconds)
- **Callback:** `squelch_tail_cb`
- **Hidden in NBFM modes** (not applicable)

**Pre-record (dropdown):**
- **ID:** `id-pre-rec`
- **Options:** `['0s', '1s', '2s', '5s', '10s']`
- **Values:** `[0, 1, 2, 5, 10]` (seconds)
- **Callback:** `pre_record_cb`
- **Hidden in NBFM modes**

### 1.3 Pan (Stereo Panning)
- **ID:** `id-pan-value`
- **Type:** Slider (w3_slider)
- **Range:** -1.0 to +1.0 (step 0.01)
- **Default:** 0 (from `kiwi_storeRead('last_pan', 0)`)
- **Variable:** `kiwi.pan`
- **Callback:** `setpan_cb`, `setpan_wheel_cb` (step 0.1)
- **Display field:** `id-pan-field` — shows "L=50R", "L=30", "R=20", or "L=R"
- **SET/action:** `audio_set_pan(pan)` — uses Web Audio `StereoPannerNode.pan.value`
- **Visibility:** Hidden unless audio panner available (`audio_panner_ui_init()`)

### 1.4 Noise Blanker (NB)
- **ID:** `nb_algo` (dropdown in "Noise" section)
- **Type:** Dropdown select
- **Options (menu_s):** `['off', 'std', 'Wild']`
- **Display names (algo_s):** `['(none)', 'standard', 'Wild algo']`
- **Internal enum:** `NB_NONE:0, NB_STD:1, NB_WILD:2`
- **Callback:** `nb_algo_cb`
- **More button:** `noise_blank_view()` — opens detailed controls in `id-nblank-more`
- **Code location:** `openwebrx.js:2523, kiwi.js (noise_blank section):1465-1494`

**NB Detailed Parameters (in id-nblank-more):**

| Parameter | Type | Range | Default | Callback |
|-----------|------|-------|---------|----------|
| `noise_blank.gate` | Slider | 100–5000 (µsec) | 100 | `noise_blank_gate_cb` |
| `noise_blank.threshold` | Slider | 0–100 (%) | 50 | `noise_blank_threshold_cb` |
| `noise_blank.thresh` (Wild) | Slider | 0.05–3.0 | 0.95 | `noise_blank_thresh_cb` |
| `noise_blank.taps` (Wild) | Slider | 6–40 | 10 | `noise_blank_taps_cb` |
| `noise_blank.impulse_samples` (Wild) | Slider | 3–41 | 7 | `noise_blank_impulse_samples_cb` |
| `noise_blank.test` | Dropdown | `['test off','test on: pre filter (std)','test on: post filter (Wild)']` | 0 | `noise_blank_test_cb` |
| `noise_blank.test_gain` | Slider | -90–0 dB | 0 | `noise_blank_test_gain_cb` |
| `noise_blank.test_width` | Slider | 1–30 samples | 1 | `noise_blank_test_width_cb` |
| `noise_blank.wf` | Checkbox | 0/1 (waterfall blanker) | 1 | `noise_blank_wf_cb` |

**NB SET commands:**
```
snd_send('SET nb algo=' + algo)
snd_send('SET nb type=0 param=0 pval=' + gate)    // for std
snd_send('SET nb type=0 param=1 pval=' + threshold)
snd_send('SET nb type=0 param=0 pval=' + thresh)  // for wild
snd_send('SET nb type=0 param=1 pval=' + taps)
snd_send('SET nb type=0 param=2 pval=' + impulse_samples)
snd_send('SET nb type=0 en=' + (algo ? 1 : 0))
snd_send('SET nb type=1 en=' + wf)
snd_send('SET nb type=2 param=0 pval=' + Math.pow(10, test_gain/20))  // test
snd_send('SET nb type=2 param=1 pval=' + test_width)
snd_send('SET nb type=2 en=' + test)
```

### 1.5 Noise Filter (NR)
- **ID:** `nr_algo` (dropdown in "Noise" section)
- **Type:** Dropdown select
- **Options (menu_s):** `['off', 'wdsp', 'LMS', 'spec']`
- **Display names (algo_s):** `['(none)', 'wdsp LMS', 'original LMS', 'spectral NR']`
- **Internal enum:** `NR_OFF:0, NR_WDSP:1, NR_ORIG:2, NR_SPECTRAL:3`
- **Callback:** `nr_algo_cb`
- **More button:** `noise_filter_view()` — opens detailed controls in `id-nfilter-more`
- **Code location:** `openwebrx.js:2523, kiwi.js (noise_filter section):1500-1539`

**NF Detailed Parameters (in id-nfilter-more):**

| Algorithm | Parameter | Type | Range | Default | Callback |
|-----------|-----------|------|-------|---------|----------|
| WDSP Denoise | `wdsp_de_taps` | Slider | 16–128 | 64 | `nf_wdsp_taps_cb` |
| WDSP Denoise | `wdsp_de_delay` | Slider | 2–128 | 16 | `nf_wdsp_delay_cb` |
| WDSP Denoise | `wdsp_de_gain` | Slider | 1–20 | 10 | `nf_wdsp_gain_cb` |
| WDSP Denoise | `wdsp_de_leakage` | Slider | 1–23 | 7 | `nf_wdsp_leakage_cb` |
| WDSP Autonotch | `wdsp_an_taps` | Slider | 16–128 | 64 | `nf_wdsp_taps_cb` |
| WDSP Autonotch | `wdsp_an_delay` | Slider | 2–128 | 16 | `nf_wdsp_delay_cb` |
| WDSP Autonotch | `wdsp_an_gain` | Slider | 1–20 | 10 | `nf_wdsp_gain_cb` |
| WDSP Autonotch | `wdsp_an_leakage` | Slider | 1–23 | 7 | `nf_wdsp_leakage_cb` |
| Original Denoise | `orig_de_delay` | Slider | 1–200 | 1 | `noise_filter_delay_cb` |
| Original Denoise | `orig_de_beta` | Slider | 0.0001–0.150 | 0.05 | `noise_filter_beta_cb` |
| Original Denoise | `orig_de_decay` | Slider | 0.90–1.0 | 0.98 | `noise_filter_decay_cb` |
| Original Autonotch | `orig_an_delay` | Slider | 1–200 | 48 | `noise_filter_delay_cb` |
| Original Autonotch | `orig_an_beta` | Slider | 0.0001–0.150 | 0.125 | `noise_filter_beta_cb` |
| Original Autonotch | `orig_an_decay` | Slider | 0.90–1.0 | 0.99915 | `noise_filter_decay_cb` |
| Spectral | `spec_gain` | Slider | -30–30 dB | 0 | `nf_spectral_gain_cb` |
| Spectral | `spec_alpha` | Slider | 0.90–0.99 | 0.95 | `nf_spectral_alpha_cb` |
| Spectral | `active_snr` | Slider | 2–30 dB | 30 | `nf_spectral_asnr_cb` |
| All | `denoise` | Checkbox | 0/1 | 0 | `noise_filter_cb` |
| All | `autonotch` | Checkbox | 0/1 | 0 | `noise_filter_cb` |

**NF SET commands:**
```
snd_send('SET nr algo=' + algo)
snd_send('SET nr type=' + type + ' param=0 pval=' + p0)
snd_send('SET nr type=' + type + ' param=1 pval=' + p1)
snd_send('SET nr type=' + type + ' param=2 pval=' + p2)
snd_send('SET nr type=' + type + ' param=3 pval=' + p3)
snd_send('SET nr type=' + type + ' en=' + en)
```
where type is `NR_DENOISE:0` or `NR_AUTONOTCH:1`.

### 1.6 De-emphasis
- **ID:** `id-deemp` (two instances: `id-deemp1-ofm` for AM/FM, `id-deemp1-nfm` for NBFM)
- **Type:** Dropdown select
- **AM/FM options (de_emphasis_s):** `['off', '75 uS', '50 uS']`
- **NBFM options (de_emphasis_nfm_s):** `['off', 'on', '+LF']`
- **Internal values:** `de_emphasis` (0–2), `de_emphasis_nfm` (0–2)
- **Callback:** `de_emp_cb(path, idx, first, nfm)` — sends `snd_send('SET de_emp=' + idx + ' nfm=' + (nfm ? 1 : 0))`
- **Code location:** `openwebrx.js:2524-2525, 2617`

### 1.7 Compression
- **ID:** `id-button-compression`
- **Type:** Toggle button (lime/white color)
- **Internal variable:** `btn_compression` (0/1)
- **Callback:** `toggle_or_set_compression`
- **Visual:** Button text "Comp", colored lime when on, white when off
- **SET command:** `snd_send('SET compression=' + btn_compression.toFixed(0))`
- **Visibility:** Hidden in IQ/stereo modes (`w3_els('id-button-compression', function(el){w3_disable(el, disabled);})`)
- **Code location:** `openwebrx.js:2525-2526, 2670-2674`

### 1.8 Passband (PB) Sliders
These appear in the audio panel below the main controls:

| ID | Label | Type | Range | Step | Callback |
|----|-------|------|-------|------|----------|
| `id-pb-lo` | PB low | Slider | Mode-dependent (see below) | 10 Hz | `setpb_cb` |
| `id-pb-hi` | PB high | Slider | Mode-dependent | 10 Hz | `setpb_cb` |
| `id-pb-center` | PB center | Slider | Mode-dependent | 10 Hz | `setpb_cb` |
| `id-pb-width` | PB width | Slider | 0–audio_input_rate | 10 Hz | `setpb_cb` |

**PB range logic (from setpb_cb):**
- For USB modes: min=0, max=audio_input_rate/2
- For LSB modes: min=-audio_input_rate/2, max=0
- For other modes: min=-audio_input_rate/2 (or 0 for width), max=audio_input_rate/2 (or audio_input_rate for width)

**Default button:** `id-button-pb-default` → `pb_default_cb()` → `restore_passband(cur_mode)`

**SET command:** `ext_set_passband(_lo, _hi)` which eventually sends modulation parameters to server.

### 1.9 SAM (Synchronous AM) Carrier
- **ID:** `id-sam-carrier-container` (hidden unless SAM mode)
- **PLL speed dropdown:** `id-sam-pll` → options `['DX', 'med', 'fast']` → values `[0, 1, 2]`
- **PLL reset button:** `sam_pll_reset_cb` → `snd_send('SET sam_pll=-1')`
- **Code location:** `openwebrx.js:2528, 2661-2662`

### 1.10 Channel Null / Overload Mute
- **Channel null dropdown:** `id-chan-null` → options `['normal', 'null LSB', 'null USB']` → values `[0, 1, 2]`
- **Overload mute dropdown:** `id-ovld-mute` → options `['off', 'on']` → values `[0, 1]`
- **SET commands:**
  - `snd_send('SET ovld_mute=' + (owrx.ovld_mute ? 1 : 0))`
  - Channel null handled via `ext_set_mode(cur_mode)`

### 1.11 AGC (in Audio Panel)
While AGC has its own tab ("AGC"), it's closely related. Key audio AGC params:
- Manual gain: `id-input-man-gain` (0–120, step 1, default 50)
- Threshold: `id-input-threshold` (-130–0 dBm, step 1, default -100)
- Threshold CW: `id-input-threshCW` (-130–0 dBm, step 1, default -130)
- Slope: `id-input-slope` (0–10, step 1, default 6)
- Decay: `id-input-decay` (20–5000 msec, step 10, default 1000)
- AGC on/off: `id-button-agc` toggle
- Hang: `id-button-hang` toggle

**SET command:** `snd_send('SET agc=' + agc + ' hang=' + hang + ' thresh=' + thold + ' slope=' + slope + ' decay=' + decay + ' manGain=' + manGain)`

---

## 2. HTML Structure of the Audio Tab Panel

### 2.1 Container Structure (built in init_panels/optbar setup)

The audio tab is built as an **optbar panel** (one of several tabs: last, Off, Stat, User, AGC, Audio, WF, RF). The HTML is constructed programmatically by `w3_innerHTML` calls.

**Main container:**
- `id-optbar-audio` — the audio optbar content div

**Top section (id-audio-content):**
```
id-audio-content (w3-margin-R-6)
├── s (Noise row): w3_inline_percent('w3-valign/w3-last-halign-end', 17% width)
│   ├── Noise blanker dropdown: w3_select('blanker', nb_algo, ...)
│   ├── "More" button (noise_blank_view)
│   ├── Noise filter dropdown: w3_select('filter', nr_algo, ...)
│   └── "More" button (noise_filter_view)
├── Volume row (id-vol): w3_inline_percent('id-vol w3-valign w3-margin-T-2 w3-hide/class-slider', 17% width)
│   ├── Volume slider: id-input-volume
│   ├── De-emphasis dropdown (ofm): id-deemp id-deemp1-ofm
│   └── De-emphasis dropdown (nfm): id-deemp id-deemp1-nfm (hidden)
├── Volume+Compression row (id-vol-comp): w3_inline_percent('id-vol-comp w3-valign w3-margin-T-2/class-slider', 17% width)
│   ├── Volume slider: id-input-volume
│   ├── De-emphasis dropdown (ofm): id-deemp id-deemp2-ofm
│   ├── De-emphasis dropdown (nfm): id-deemp id-deemp2-nfm (hidden)
│   └── Compression button: id-button-compression
├── Pan row (id-pan): w3_inline_percent('id-pan w3-valign w3-hide/class-slider', 17% width)
│   ├── Pan slider: id-pan-value
│   ├── Pan display field: id-pan-field
│   └── Compression button: id-button-compression
├── Squelch row: w3_inline_percent('id-squelch w3-valign/class-slider', 15% width)
│   ├── Squelch toggle icon: fa-caret-down (id-squelch-label)
│   ├── Squelch slider: id-squelch-value
│   ├── Squelch field display: id-squelch-field
│   ├── Pre-record dropdown: id-pre-rec (hidden in NBFM)
│   └── Squelch tail dropdown: id-squelch-tail (hidden in NBFM)
├── SAM carrier container (id-sam-carrier-container, hidden unless SAM):
│   ├── SAM label + carrier display
│   ├── PLL speed dropdown: id-sam-pll
│   └── PLL reset button
└── PB default row: w3_inline('w3-margin-T-2 w3-valign w3-halign-space-between/class-slider')
    ├── PB default button: w3_button('PB default', pb_default_cb)
    ├── Channel null dropdown: id-chan-null (hidden)
    └── Ovld mute dropdown: id-ovld-mute
```

**Passband sliders section (s1):**
```
id-optbar-audio (continued)
└── s1 (w3_div):
    ├── PB low slider: id-pb-lo (20% width, slpct=65%)
    ├── PB high slider: id-pb-hi (20% width)
    ├── PB center slider: id-pb-center (20% width)
    └── PB width slider: id-pb-width (20% width)
```

**Bottom sections (separated by hr):**
- `id-nblank-more` — Noise blanker detailed controls (populated by `noise_blank_controls_refresh()`)
- `id-nfilter-more` — Noise filter detailed controls (populated by `noise_filter_controls_refresh()`)
- `id-ntest-more` — Noise blanker test controls

### 2.2 Key Element IDs

| Element ID | Purpose |
|------------|---------|
| `id-optbar-audio` | Audio optbar container |
| `id-audio-content` | Top audio control row container |
| `id-vol` | Volume-only row (hidden when compression enabled) |
| `id-vol-comp` | Volume+compression row |
| `id-pan` | Pan control row (hidden unless stereo panner available) |
| `id-squelch` | Squelch control row |
| `id-sam-carrier-container` | SAM mode carrier controls (hidden unless SAM) |
| `id-nblank-more` | Noise blanker detail expandable section |
| `id-nfilter-more` | Noise filter detail expandable section |
| `id-ntest-more` | Noise blanker test section |
| `id-input-volume` | Volume slider input |
| `id-deemp` | De-emphasis dropdown (multiple instances) |
| `id-pan-value` | Pan slider input |
| `id-pan-field` | Pan display field (L/R percentage) |
| `id-squelch-value` | Squelch slider input |
| `id-squelch-field` | Squelch value display ("off" or "X dB") |
| `id-pre-rec` | Pre-record time dropdown |
| `id-squelch-tail` | Squelch tail length dropdown |
| `id-button-compression` | Compression toggle button |
| `id-pb-lo`, `id-pb-hi`, `id-pb-center`, `id-pb-width` | Passband sliders |
| `id-pb-lo-val`, `id-pb-hi-val`, `id-pb-center-val`, `id-pb-width-val` | Passband value displays |
| `id-chan-null` | Channel null dropdown |
| `id-ovld-mute` | Overload mute dropdown |

---

## 3. CSS Class Names and Styles Used

### 3.1 Layout Classes (w3.css / w3_ext.css)

| Class | Purpose |
|-------|---------|
| `w3-valign` | Vertical alignment (flexbox) |
| `w3-halign-space-between` | Horizontal: space between items |
| `w3-last-halign-end` | Right-align last item |
| `w3-inline` / `w3-show-inline` | Inline display |
| `w3-inline-percent` | Percentage-based inline layout |
| `w3-margin-T-2`, `w3-margin-T-8`, etc. | Top margin variants |
| `w3-margin-R-6`, `w3-margin-R-16`, etc. | Right margin variants |
| `w3-margin-L-8`, `w3-margin-L-10`, etc. | Left margin variants |
| `w3-margin-B-8` | Bottom margin |
| `w3-margin-LR-16` | Left+right margin |
| `w3-margin-between-16` | Margin between items |
| `w3-padding-tiny`, `w3-padding-smaller`, `w3-padding-1` | Padding variants |
| `w3-hide` | Display:none (used to conditionally hide controls) |
| `w3-show-inline-block` | Inline-block display |
| `w3-center` | Centered text |
| `w3-hcenter` | Horizontal center container |
| `w3-flex-noshrink` | Flex item that doesn't shrink |
| `w3-gap-16` | Gap between flex items |
| `w3-section` | Section divider style |
| `w3-col-percent` | Column percentage layout |
| `w3-div` | Generic div with classes |
| `w3_divs` | Multiple divs |

### 3.2 KiwiSDR-Specific Classes (kiwi.css / w3_ext.css)

| Class | Purpose |
|-------|---------|
| `class-slider` | Slider styling wrapper |
| `class-button` | Button styling |
| `w3-text-css-orange` | Orange text (used for labels like "Volume", "Squelch", "Pan") |
| `w3-text-css-orange cl-closer-spaced-label-text` | Orange label for "Noise" header |
| `w3-text-red` | Red text (used for dropdown labels like "blanker", "filter") |
| `w3-text-aqua` | Aqua text (used for section headers like "Noise blanker", "Noise filter") |
| `w3-text-white` | White text |
| `w3-text-yellow-highlight` | Yellow highlight text |
| `w3-text-css-lime` | Lime green text |
| `w3-y-16` | Custom yellow background |
| `cl-cpanel-hr` | Control panel horizontal rule (thick border) |
| `cl-closer-spaced-label-text` | Tighter label spacing |
| `w3-wheel-shift` | Wheel (scroll) event modifier class |
| `w3-momentary` | Momentary button style |
| `w3-noactive` | Button that doesn't change color on click |
| `w3-hold` | Hold-style button |
| `w3-btn-right` | Button aligned right |
| `w3-margin-T-8`, `w3-margin-T-2` | Margin top variants |
| `id-button-compression` | Compression button ID (also used as CSS hook) |
| `id-squelch-label` | Squelch label ID (color changes based on squelch state) |

### 3.3 Slider Styles (from w3_ext.css)

The range slider styling in `w3_ext.css` is extensive:
- `input[type=range]` — base styling, width 100%, height 22px
- `input[type=range]::-webkit-slider-runnable-track` — track (3px height, gray #808080)
- `input[type=range]::-webkit-slider-thumb` — thumb (18px, white, 1px border #808080)
- `input[type=range]::-moz-range-track` — Firefox track
- `input[type=range]::-moz-range-thumb` — Firefox thumb (16px)
- Focus states for both Webkit and Mozilla

### 3.4 Dropdown Styles

- `.w3-select-menu` — Select styling (width auto, cursor pointer, white background, 1px border)
- `.w3-menu-container` — Dropdown menu container
- `.w3-menu-item` — Menu item (padding 4px 16px)
- `.w3-menu-item-selected` — Selected item (green background)
- `.w3-menu-item-hover:hover` — Hover state (blue background, white text)

### 3.5 Visibility/Toggle Patterns

- `w3-hide` — Hidden elements (used for mode-dependent controls)
- `w3_show_hide()` / `w3_hide2()` — JavaScript functions to toggle visibility
- CSS transitions on opacity for some elements

---

## 4. Raw Code Excerpts for Slider/Dropdown Rendering

### 4.1 Volume Slider (from openwebrx.js:2524-2526)

```javascript
w3_inline_percent('id-vol w3-valign w3-margin-T-2 w3-hide/class-slider w3-last-halign-end',
   w3_text('w3-text-css-orange','Volume'), 17,
   w3_slider('id-input-volume w3-wheel-shift','','',kiwi.volume,0,200,1,'setvolume_cb'), 50,
   '&nbsp;', 8,
   w3_div('', w3_select('id-deemp id-deemp1-ofm w3-text-red||title="de-emphasis"','','de-<br>emp','de_emphasis',de_emphasis,de_emphasis_s,'de_emp_cb',0),
      w3_select('id-deemp id-deemp1-nfm w3-text-red w3-hide||title="de-emphasis"','','de-<br>emp','de_emphasis_nfm',de_emphasis_nfm,de_emphasis_nfm_s,'de_emp_cb',1))
)
```

### 4.2 Squelch Row (from openwebrx.js:2527)

```javascript
w3_inline_percent('id-squelch w3-valign/class-slider w3-last-halign-end',
   w3_text('id-squelch-label','Squelch'), 15,
   w3_icon('||title="toggle squelch"', 'fa-caret-down', 22, 'lime', 'squelch_zero_cb', 1), 3,
   w3_slider('id-squelch-value w3-wheel-shift','','',squelch,0,99,1,'set_squelch_cb'), 42,
   w3_div('id-squelch-field w3-center class-slider'), 14,
   w3_select('id-pre-rec w3-margin-R-4 w3-hide w3-text-red||title="pre-record time"','','pre','pre_record',pre_record,pre_record_s,'pre_record_cb'), 16,
   w3_select('id-squelch-tail w3-hide w3-text-red||title="squelch tail length"','','tail','squelch_tail',squelch_tail,squelch_tail_s,'squelch_tail_cb')
)
```

### 4.3 Noise Blanker Dropdown + More Button (from openwebrx.js:2523)

```javascript
s = w3_inline_percent('w3-valign/w3-last-halign-end',
   w3_text('w3-text-css-orange cl-closer-spaced-label-text','Noise'), 17,
   w3_select('w3-text-red||title="noise blanker selection"','','blanker','nb_algo',0,noise_blank.menu_s,'nb_algo_cb','m'), 24,
   w3_div('w3-hcenter', w3_button('class-button w3-text-orange||title="noise blanker parameters"','More','noise_blank_view')), 19,
   w3_div('w3-hcenter ', w3_select('w3-text-red||title="noise filter selection"','','filter','nr_algo',0,noise_filter.menu_s,'nr_algo_cb','m')), 27,
   w3_button('class-button w3-text-orange||title="noise filter parameters"','More','noise_filter_view')
)
```

### 4.4 Pan Slider (from openwebrx.js:2526)

```javascript
w3_inline_percent('id-pan w3-valign w3-hide/class-slider w3-last-halign-end',
   w3_text('w3-text-css-orange','Pan'), 17,
   w3_slider('id-pan-value w3-wheel-shift','','',kiwi.pan,-1,1,0.01,'setpan_cb'), 50,
   '&nbsp;', 3,
   w3_div('id-pan-field'), 8,
   '&nbsp;', 7,
   w3_button('id-button-compression class-button w3-hcenter||title="compression"','Comp','toggle_or_set_compression')
)
```

### 4.5 Noise Blanker Detail Controls (from kiwi.js:1467-1473)

```javascript
function noise_blank_controls_refresh() {
   var s = '';
   if (noise_blank.algo != noise_blank.NB_NONE)
      s = w3_checkbox('w3-margin-B-8/w3-label-inline w3-label-not-bold w3-text-css-orange/','Waterfall std blanker','noise_blank.wf',noise_blank.wf,'noise_blank_wf_cb');
   switch (noise_blank.algo) {
      case noise_blank.NB_STD:
         s += w3_slider('','Gate','noise_blank.gate',noise_blank.gate,100,5000,100,'noise_blank_gate_cb') +
            w3_slider('','Threshold','noise_blank.threshold',noise_blank.threshold,0,100,1,'noise_blank_threshold_cb');
         break;
      case noise_blank.NB_WILD:
         if (ext_is_IQ_or_stereo_curmode())
            s += 'No Wild algorithm blanking in IQ or stereo modes';
         else
            s += w3_slider('','Threshold','noise_blank.thresh',noise_blank.thresh,0.05,3,0.05,'noise_blank_thresh_cb') +
               w3_slider('','Taps','noise_blank.taps',noise_blank.taps,6,40,1,'noise_blank_taps_cb') +
               w3_slider('','Samples','noise_blank.impulse_samples',noise_blank.impulse_samples,3,41,2,'noise_blank_impulse_samples_cb');
         break;
   }
   var controls_html = w3_div('id-noise-blanker-controls w3-margin-right w3-text-white',
      w3_divs('/w3-tspace-8',
         w3_inline('w3-gap-16/',
            w3_div('w3-text-aqua','<b>Noise blanker</b>'),
            w3_select('w3-text-red||title="noise blanker selection"','','type','nb_algo',noise_blank.algo,noise_blank.menu_s,'nb_algo_cb','m'),
            w3_button('w3-padding-tiny w3-yellow','Defaults','noise_blank_load_defaults'),
            w3_button('id-noise-blanker-help-btn w3-btn-right w3-green w3-small w3-padding-small','help','noise_blank_help')
         ),
         w3_div('w3-margin-LR-16', s)
      )
   );
   w3_innerHTML('id-nblank-more', controls_html);
   // ... test controls in id-ntest-more
}
```

### 4.6 Noise Filter Detail Controls (from kiwi.js:1502-1510)

```javascript
function noise_filter_controls_html() {
   var s = '';
   switch (noise_filter.algo) {
      case noise_filter.NR_WDSP:
         s = w3_inline('w3-margin-between-16',
            w3_checkbox('w3-label-inline w3-text-css-orange/','Denoiser','noise_filter.denoise',noise_filter.denoise,'noise_filter_cb')) +
            w3_div('w3-section',
               w3_slider('','Taps','noise_filter.wdsp_de_taps',noise_filter.wdsp_de_taps,16,128,1,'nf_wdsp_taps_cb'),
               w3_slider('','Delay','noise_filter.wdsp_de_delay',noise_filter.wdsp_de_delay,2,128,1,'nf_wdsp_delay_cb'),
               w3_slider('','Gain','noise_filter.wdsp_de_gain',noise_filter.wdsp_de_gain,1,20,1,'nf_wdsp_gain_cb'),
               w3_slider('','Leakage','noise_filter.wdsp_de_leakage',noise_filter.wdsp_de_leakage,1,23,1,'nf_wdsp_leakage_cb')
            ) +
            // ... autonotch sliders
         );
         break;
      case noise_filter.NR_ORIG:
         // ... original LMS sliders
         break;
      case noise_filter.NR_SPECTRAL:
         s = w3_div('w3-section',
            w3_slider('','Gain','noise_filter.spec_gain',noise_filter.spec_gain,-30,30,1,'nf_spectral_gain_cb'),
            w3_slider('','Alpha','noise_filter.spec_alpha',noise_filter.spec_alpha,0.90,0.99,0.01,'nf_spectral_alpha_cb'),
            w3_slider('','Active SNR','noise_filter.active_snr',noise_filter.active_snr,2,30,1,'nf_spectral_asnr_cb')
         );
         break;
   }
   // ... build full HTML with header and dropdown
   return controls_html;
}
```

### 4.7 PB Default Button + Passband Sliders (from openwebrx.js:2529)

```javascript
w3_inline('w3-margin-T-2 w3-valign w3-halign-space-between/class-slider',
   w3_button('w3-padding-tiny w3-yellow||title="restore passband"','PB default','pb_default_cb'),
   w3_inline('', w3_select('id-chan-null w3-text-red w3-margin-R-6 w3-hide||title="channel null"','','channel<br>null','owrx.chan_null',owrx.chan_null,owrx.chan_null_s,'chan_null_cb'),
      w3_select('id-ovld-mute w3-text-red||title="overload mute"','','ovld<br>mute','owrx.ovld_mute',owrx.ovld_mute,owrx.ovld_mute_s,'ovld_mute_cb')
   )
);
var slpct = 65; var step = 10;
var s1 = w3_div('', w3_inline_percent('w3-valign w3-margin-T-2/class-slider',
   w3_text('w3-text-css-orange','PB low'), 20,
   w3_slider('id-pb-lo w3-wheel-shift','','',0,0,0,step,'setpb_cb',owrx.PB_LO), slpct,
   w3_div('id-pb-lo-val w3-margin-L-10')
), w3_inline_percent('w3-valign w3-margin-T-2/class-slider',
   w3_text('w3-text-css-orange','PB high'), 20,
   w3_slider('id-pb-hi w3-wheel-shift','','',0,0,0,step,'setpb_cb',owrx.PB_HI), slpct,
   w3_div('id-pb-hi-val w3-margin-L-10')
), w3_inline_percent('w3-valign w3-margin-T-2/class-slider',
   w3_text('w3-text-css-orange','PB center'), 20,
   w3_slider('id-pb-center w3-wheel-shift','','',0,0,0,step,'setpb_cb',owrx.PB_CENTER), slpct,
   w3_div('id-pb-center-val w3-margin-L-10')
), w3_inline_percent('w3-valign w3-margin-T-2/class-slider',
   w3_text('w3-text-css-orange','PB width'), 20,
   w3_slider('id-pb-width w3-wheel-shift','','',0,0,0,step,'setpb_cb',owrx.PB_WIDTH), slpct,
   w3_div('id-pb-width-val w3-margin-L-10')
));
```

### 4.8 Compression Toggle Button (from openwebrx.js:2525-2526, 2670-2674)

```javascript
// Button rendering (two variants for different layouts):
w3_button('id-button-compression class-button w3-hcenter||title="compression"','Comp','toggle_or_set_compression')

// Toggle handler:
function toggle_or_set_compression(set, val) {
   if (recording) return;
   if (isNumber(set))
      btn_compression = kiwi_toggle(set, val, btn_compression, 'last_compression');
   else
      btn_compression ^= 1;
   w3_els('id-button-compression', function(el) {
      el.style.color = btn_compression ? 'lime' : 'white';
      el.style.visibility = 'visible';
      freqset_select();
   });
   // ...
   snd_send('SET compression=' + btn_compression.toFixed(0));
}
```

---

## 5. Constants and Configuration Tables

### 5.1 De-emphasis Options (openwebrx.js:2617)

```javascript
var de_emphasis = 0, de_emphasis_nfm = 0;
var de_emphasis_s = ['off', '75 uS', '50 uS'];       // AM/FM
var de_emphasis_nfm_s = ['off', 'on', '+LF'];        // NBFM
```

### 5.2 Squelch Tail Options (openwebrx.js:2645)

```javascript
var squelch_tail = 0;
var squelch_tail_s = ['0s', '.2s', '.5s', '1s', '2s'];
var squelch_tail_v = [0, 0.2, 0.5, 1, 2];
```

### 5.3 Pre-record Options (openwebrx.js:2645)

```javascript
var pre_record = 0;
var pre_record_s = ['0s', '1s', '2s', '5s', '10s'];
var pre_record_v = [0, 1, 2, 5, 10];
```

### 5.4 Noise Blanker Configuration (kiwi.js:1465)

```javascript
var noise_blank = {
   algo: 0,
   algo_s: ['(none)', 'standard', 'Wild algo'],
   menu_s: ['off', 'std', 'Wild'],
   width: 400,
   height: [185, 310, 350],
   NB_OFF: 0,
   blanker: 0,
   test: 0,
   test_s: ['test off', 'test on: pre filter (std)', 'test on: post filter (Wild)'],
   test_gain: 0,
   test_width: 1,
   wf: 1,
   NB_BLANKER: 0,
   NB_WF: 1,
   NB_CLICK: 2,
   NB_NONE: 0,
   NB_STD: 1,
   gate: 100,
   threshold: 50,
   NB_WILD: 2,
   thresh: 0.95,
   taps: 10,
   impulse_samples: 7
};
```

### 5.5 Noise Filter Configuration (kiwi.js:1501)

```javascript
var noise_filter = {
   algo: 0,
   algo_s: ['(none)', 'wdsp LMS', 'original LMS', 'spectral NR'],
   menu_s: ['off', 'wdsp', 'LMS', 'spec'],
   width: 400,
   height: [100, 475, 400, 185],
   NR_OFF: 0,
   denoise: 0,
   autonotch: 0,
   NR_DENOISE: 0,
   NR_AUTONOTCH: 1,
   NR_WDSP: 1,
   wdsp_de_taps: 64,
   wdsp_de_delay: 16,
   wdsp_de_gain: 10,
   wdsp_de_leakage: 7,
   wdsp_an_taps: 64,
   wdsp_an_delay: 16,
   wdsp_an_gain: 10,
   wdsp_an_leakage: 7,
   NR_ORIG: 2,
   orig_de_delay: 1,
   orig_de_beta: 0.05,
   orig_de_decay: 0.98,
   orig_an_delay: 48,
   orig_an_beta: 0.125,
   orig_an_decay: 0.99915,
   NR_SPECTRAL: 3,
   spec_gain: 0,
   spec_alpha: 0.95,
   active_snr: 30
};
```

### 5.6 KiwiSDR Core Audio Variables (kiwi.js)

```javascript
var kiwi = {
   // ...
   volume: 50,
   volume_f: 1e-6,
   muted: 1,         // mute until muted_initially state determined
   unmuted_color: 'lime',
   pan: 0,
   // ...
};
```

### 5.7 Passband Sliders Step and Layout (openwebrx.js:2529)

```javascript
var slpct = 65;  // percentage of remaining width for slider track
var step = 10;   // Hz step for passband sliders
// PB_LO=0, PB_HI=1, PB_CENTER=2, PB_WIDTH=3 (owrx constants)
```

### 5.8 URL Parameters for Audio (openwebrx.js:kiwi_main_ready)

```
vol=X      — set volume (0–200)
mute=X     — set muted initially
sqth=X     — set squelch threshold
click      — enable noise blanker click
audio=X    — audio measurement delay enable
blen=X     — audio buffer min length (ms)
```

---

## 6. Key Callback Functions (Summary)

| Callback | Element | Purpose |
|----------|---------|---------|
| `setvolume_cb` | `id-input-volume` | Sets `kiwi.volume`, clamps to 0–200, stores cookie |
| `setvolume_wheel_cb` | volume slider | Wheel adjustment (step 5–15) |
| `setpan_cb` | `id-pan-value` | Sets `kiwi.pan`, calls `audio_set_pan()`, stores cookie |
| `setpan_wheel_cb` | pan slider | Wheel adjustment (step 0.1) |
| `set_squelch_cb` | `id-squelch-value` | Sets `squelch`, updates display field, sends SET |
| `set_squelch_wheel_cb` | squelch slider | Wheel adjustment (step 1, mult 5) |
| `squelch_zero_cb` | squelch icon | Toggles squelch enable/disable |
| `squelch_tail_cb` | `id-squelch-tail` | Sets tail length, sends SET |
| `pre_record_cb` | `id-pre-rec` | Sets pre-record time, reinitializes buffer |
| `de_emp_cb` | `id-deemp` | Sets de-emphasis, sends `SET de_emp=... nfm=...` |
| `nb_algo_cb` | noise blanker dropdown | Sets NB algorithm, sends SET, refreshes detail view |
| `nr_algo_cb` | noise filter dropdown | Sets NR algorithm, sends SET, refreshes detail view |
| `toggle_or_set_compression` | `id-button-compression` | Toggles compression on/off, sends SET |
| `pb_default_cb` | PB default button | Restores passband to mode defaults |
| `setpb_cb` | PB sliders | Sets passband parameters, sends to server |
| `audio_panner_ui_init` | (init) | Shows/hides pan controls based on SteererPanner availability |

---

## 7. Observations for Implementing Audio Panel in VST

1. **Volume:** 0–200 scale, default 50. Internally mapped to gain factor (0–2.0). The "0" volume doesn't truly mute — it sets `volume_f = 1e-6` (near-zero gain).

2. **Squelch:** 0–99 scale (or 0–40 for non-NBFM). Has enable/disable toggle. Different parameter sent depending on mode (threshold for NBFM, tail length for others). Tail options: 0, 0.2, 0.5, 1, 2 seconds. Pre-record: 0, 1, 2, 5, 10 seconds.

3. **Pan:** -1.0 to +1.0 with 0.01 step. Uses Web Audio `StereoPannerNode`. Display shows percentage L/R. Center snap: values between -0.1 and +0.1 are forced to 0.

4. **Noise Blanker:** Three algorithms (off, standard, Wild). Standard has gate (100–5000 µs) and threshold (0–100%). Wild has threshold (0.05–3.0), taps (6–40), impulse_samples (3–41). Test pulse feature with gain (-90 to 0 dB) and width (1–30 samples). Waterfall blanker checkbox.

5. **Noise Filter:** Four algorithms (off, wdsp LMS, original LMS, spectral NR). Each has different parameter sets. WDSP has separate denoise and autonotch parameter sets (taps, delay, gain, leakage). Original LMS has delay, beta, decay for both denoise and autonotch. Spectral has gain (-30 to 30 dB), alpha (0.90–0.99), active_snr (2–30 dB).

6. **De-emphasis:** AM/FM has 3 options (off, 75µS, 50µS). NBFM has 3 options (off, on, +LF). Separate dropdowns shown based on mode.

7. **Compression:** Simple on/off toggle. Hidden in IQ/stereo modes.

8. **Passband sliders:** Mode-dependent ranges. 10 Hz step. Width slider goes up to audio_input_rate. Default button restores mode defaults.

9. **SAM mode:** Additional carrier controls with PLL speed (DX/med/fast) and reset button.

10. **Channel null:** Three options (normal, null LSB, null USB) — affects SAM processing.

11. **Overload mute:** Simple on/off toggle.

12. **UI Pattern:** Controls are organized in rows with percentage-based widths. Sliders use `w3-wheel-shift` class for mouse wheel support. Dropdowns use `w3-text-red` styling. Labels use `w3-text-css-orange`. Detail sections expand below the main row with `hr` separators.
