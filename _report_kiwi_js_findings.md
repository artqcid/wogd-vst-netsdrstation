# KiwiSDR UI JS Research — Findings (read-only)

Sources inspected:
- `C:\Users\marku\AppData\Local\Temp\opencode\kiwisdr.min.js` (213 KB, minified, binary-ish)
- CSS: `tool_0461840ba001KHPbM4pwyEwAyp` (not used for answers, only context)

## Key observation about the JS file

`kiwisdr.min.js` is **not plain text JavaScript**. Reading it as UTF-8 produces binary garbage; reading as raw bytes shows high-entropy content with null bytes and no readable identifier strings as a contiguous text stream. A full-token extraction across the whole file yields only ≈2,500-6,000 unique “alphanumeric tokens” (`[A-Za-z_]\w*`), and almost all of them are short (2-4 chars) or contain non-ASCII characters.

Because of this, **the specific JS rendering logic you want is NOT directly extractable by grep/Select-String** from this file unless combined with de-minification or de-obfuscation outside the scope of a read-only grep task.

## What IS findable (identifier-level)

From a full token scan, the following identifiers **do appear** in `kiwisdr.min.js` (case-insensitive, from the file content itself):

- `AM`
- `CW`
- `IQ`
- `dB`
- `dx`
- `tz`
- `tx`
- `rx`
- `bw`
- `if`
- `rf`
- `lo`
- `af`
- `fm`
- `file`
- `js`
- `ws`
- `w3`
- `id`
- `push`

Notably **absent** (not found at all in the token scan):

- Any of these control-mode strings: `AMN`, `AMW`, `USB`, `USN`, `LSB`, `LSN`, `CWN`, `NBFM`, `NNFM`, `SAM`, `SAU`, `SAL`, `SAS`, `QAM`, `DRM`
- Any of these UI strings: `Mute`, `Volume`, `AGC`, `Squelch`, `Band`, `Waterfall`, `Zoom`, `Colormap`, `Speed`, `Frequency`, `Mode`, `Select`, `Passband`, `Scale`, `Topbar`, `Control`
- Band tags: `160m`, `80m`, `40m`, `30m`, `20m`, `17m`, `15m`, `12m`, `10m`, `6m`, `MW`, `LW`, `DCF77`, `WWV`, `49m`, `41m`, `31m`, `25m`, `19m`, `16m`, `13m`, `11m`

So **none of the descriptive control labels or band names you care about appear as literal string tokens in this minified blob** — if they are present at all, they are either:
- minified/obfuscated inside packed structures,
- embedded in non-ASCII punycode-style tokens, or
- loaded from separate sources not captured here.

## What the CSS file tells us (context only)

The CSS file strongly implies the intended DOM/layout structure:

- `#id-topbar` — header at top
- `#id-band-container` / `#id-band-canvas` — band layer above waterfall
- `#id-scale-container` / `#id-scale-canvas` — frequency scale above waterfall, with `passband-adjust-*` elements (`cf`, `cut`, etc.)
- `#id-control` / `#id-control-inner` — right side control panel (z-index high)
- Inside `#id-control` there are CSS hooks like:
  - `#id-freq-form`, `.id-control-freq1`, `.id-control-freq2`, `.id-mouse-freq`, `.id-step-freq`
  - `.id-control-mode`
  - `.id-control-zoom`
  - `.id-control-smeter` / `#id-smeter-scale` / `.id-smeter-ovfl` / `.id-smeter-attn` / `.id-smeter-dbm-value` / `.id-smeter-dbm-units`
- There is a `class-passband-adjust-cf`, `class-passband-adjust-cut`, and tooltips, plus a 20px-high drag area in `#id-scale-canvas`

CSS also mentions:
- `.class-button`, `.class-button-small`, `.class-slider`, etc.
- W3.CSS extensions (kiwi/w3_ext.min.css)
- OpenWebRX-derived layout constants: band container height 110px, band canvas 30px, dx container 80px, scale container 47px, scale canvas 47px.

## Control panel (`#id-control`) — best inference

Based on CSS hooks and token presence, the right control panel likely contains groups similar to:

1. Frequency entry area (`#id-freq-form`, `.id-control-freq1/.freq2`, step/mouse freq)
2. Mode selection area (`.id-control-mode`)
3. Zoom/page controls (`.id-control-zoom`)
4. S-meter area (`.id-control-smeter`)
5. Possibly additional buttons/menus (`#id-button-user`, `#id-button-func`, `#id-button-more`)

But I could **not** confirm exact control order, exact labels, or exact JS widget creation calls (`W3.create`, `addControl`, etc.) because those identifiers are not present as readable strings in the minified JS.

## Band tags (`#id-band-container` / `#id-band-canvas`)

- **No literal band names** (160m/80m/40m/.../WWV/DCF77) found in the JS token set.
- Therefore I **cannot** give an authoritative band→frequency mapping extracted from this JS file.
- The CSS strongly suggests band tags are **canvas-drawn or HTML-buttons inside band-container**, with a separate 30px band canvas plus an 80px dx container for DX cluster info.

## Mode buttons

- Only mode-like tokens present: `AM`, `CW`, `IQ`, `fm`.
- No `USB/LSB/CWN/NBFM/NNFM/SAM/SAU/SAL/SAS/QAM/DRM` tokens found.
- Therefore: **cannot confirm the exact mode list or order or whether they are drawn in control panel vs band/scale canvas.**

## Frequency scale / passband adjusters

CSS indicates:

- `#id-scale-canvas` supports a repeati-ax background image and an overlay passband region.
- Passband adjusters are 20px high draggable elements:
  - `.class-passband-adjust-cf` — center frequency adjust (cursor: ew-resize)
  - `.class-passband-adjust-cut` — cut/edge adjust (cursor: ew-resize)
  - Tooltips for each
- The “yellow drag bar” concept is consistent with `.class-passband-adjust-*` being draggable on the scale canvas.

But again, I could **not** find the JS that implements the passband drag logic or the actual numeric scale drawing in any readable form inside this minified file.

## Summary verdict

| Question | Answer from kiwisdr.min.js |
|----------|----------------------------|
| Control panel contents & order | Not directly readable; inferred from CSS hooks only |
| Band tags & band→freq mapping | Not found |
| Mode button list & placement | Only `AM`, `CW`, `IQ`, `fm` tokens found; others missing |
| Frequency scale & passband adjusters JS | CSS shows structure; JS logic not extractable from this file in readable form |

If you want the real answer, you’ll need either:
- the non-minified/unpacked `kiwisdr.min.js`, or
- a de-obfuscated/extracted version, or
- the original source (e.g., `kiwisdr.js` / `openwebrx.js`) that produced this minified blob.

Given the read-only constraint, I did not attempt any de-minification transforms that modify or re-save the file.

Would you like me to (next, still read-only) try to locate a non-minified version of the KiwiSDR/OpenWebRX JS in this workspace or in another known local path, or to locate a KiwiSDR/OpenWebRX source mirror online?
