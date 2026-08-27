# M4 Implementation Plan — KiwiSDR UI Parity (Vue)

_Cross-reference: `doc/checklist.md` M4 · `doc/ui-architecture.md` §3–§6 ·
`doc/architecture.md` §5 · `doc/coding-standards.md`_

## Overview

M4 is a 1:1 re-implementation of the KiwiSDR browser interface (`kphsdr.com:8072`)
inside the Vue 3 plugin UI. After M4 the VST is operable exactly like the web
UI — same controls, same readouts, same visual language.

**Fundamental requirement (Grundbedingung, applies to all M4 work):** the
editor must be freely resizable by dragging the bottom-right corner. The UI
must reflow continuously at any size. No M4 step is "done" until the resized
state renders correctly.

**KiwiSDR UI source reference:**
- HTML skeleton: `web/kiwi/` in `jks-prv/Beagle_SDR_GPS` (served dynamically;
  structure derived from `admin.html` and JS-injected panels).
- Layout framework: W3.CSS (external CDN) + `web/kiwi/w3_ext.css` (KiwiSDR
  extensions). Key CSS: dark background `#222`/`#333`, accent green `#4CAF50`,
  white text, 11–14pt fonts. Sliders use `appearance:none` + custom 18 px
  thumb.
- Widget library: `w3_util.js` — sliders, selects, checkboxes, icon buttons.
- Reference sections: `doc/ui-architecture.md` §3 (complete element inventory),
  §4 (parameter mapping), §6 (tab layout and resize).

---

## Chronological implementation order

### Step 1 — M4.1: Resizable window (Grundbedingung — implement FIRST)

All subsequent UI work depends on having a resizable container. Implement this
before any panel or component work.

#### 1a. C++ side — forward `WM_SIZE` to the WebView2 widget

The current `onSize` path in `plugin_editor.cpp` calls `webview.set_size()`
which resizes the parent window — not the widget. The webview/webview 0.12.0
widget child window must be resized explicitly (extends FIX-22).

```cpp
// plugin_editor.cpp  onSize():
HWND widget = static_cast<HWND>(webviewHost_.widget());
RECT client{};
GetClientRect(frame, &client);
MoveWindow(widget, 0, 0,
           client.right - client.left,
           client.bottom - client.top,
           TRUE);
```

Also patch `webview_editor.cpp::attach()`: after creating the webview, call
`MoveWindow` once with the parent's current client rect so the widget is sized
immediately (not waiting for the first `WM_SIZE`).

Platform wrappers needed for macOS/Linux (defer to cross-platform milestone):
- macOS: resize the `NSView` via `setFrame`.
- Linux (GTK): `gtk_widget_set_size_request` + container resize.

#### 1b. UI side — fully responsive Vue layout

Replace any hard-coded pixel dimensions in the current Vue components with
fluid flex/grid layouts.

**Strategy — use CSS Grid:**

```css
/* ui/src/assets/master.css */
.kiwi-layout {
  display: grid;
  grid-template-rows: auto 1fr auto; /* header / main / status */
  grid-template-columns: 1fr;
  height: 100%;
  min-height: 0;
}
.kiwi-controls-row {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
  align-items: flex-start;
}
.kiwi-panel {
  flex: 1 1 220px;  /* shrink/grow, min 220 px */
  min-width: 0;
}
```

All panels become flex items that wrap at narrow widths. This is how the real
KiwiSDR UI reflows its panels on smaller screens.

**Min size:** `kMinWidth = 640px`, `kMinHeight = 400px` (update
`source/editor/plugin_editor.cpp`). Below this the editor scrolls rather than
clips.

#### 1c. Test

- Manual: drag corner in VST3PluginTestHost from 640×400 to full-screen and
  back; no clipping, no 0×0 widget.
- Vitest: mount `App.vue` at viewport 640×400, 1024×600, 1920×1080 — assert
  all panels visible.

**Files:**
- `source/editor/plugin_editor.cpp` (onSize, min size constant)
- `source/webview/webview_editor.cpp` (attach + widget resize)
- `ui/src/assets/master.css`
- `ui/src/App.vue`

---

### Step 1.5 — M4.1.5: Schema-based Bridge API (type-safe contract)

**Requirement:** Define a well-specified, schema-based API contract between the
C++ backend (VST processor/controller) and the Vue.js frontend (WebView UI).
The API must enable modern decoupling, automatic type/validator generation, and
maintainable evolution. Inspired by OpenAPI/Swagger for REST APIs, but adapted
for the embedded UI ↔ C++ message-passing architecture.

#### Context & Challenges

The current bridge protocol (`source/vst/common/bridge_protocol.h`) uses
manual JSON parsing and hand-written parameter name constants (`kUiParamFreqKhz`
etc.). This approach works but has scaling issues:

1. **No compile-time type safety** between TypeScript and C++ — parameter names
   and value types are duplicated manually in both languages.
2. **No runtime validation schema** — malformed messages from the UI are caught
   only deep inside the C++ parser.
3. **Difficult to evolve** — adding a new parameter requires manual edits in 5
   places: C++ constant, C++ parser, TypeScript service, TypeScript type, Vue
   component.
4. **No versioning** — breaking changes to message formats are not tracked.
5. **No automatic documentation** — UI developers must read C++ headers to
   understand the protocol.

#### Research Summary (2026 State-of-the-Art)

Modern schema-based API design uses a **single source of truth** (a schema
file) to generate both server-side and client-side code. Common approaches:

- **OpenAPI / Swagger** — REST API standard; generates server stubs + client
  SDKs. Not directly applicable (VST plugins don't expose HTTP endpoints), but
  the *philosophy* (schema → generated code) is transferable.
- **JSON Schema** — Language-agnostic schema definition; mature tooling for
  TypeScript (e.g. `json-schema-to-typescript`, `ajv`) and C++ (e.g.
  `nlohmann/json` + validators). **Widely adopted** as the interop format.
- **Zod (TypeScript)** — Runtime validator + type inference. Excellent DX for
  TypeScript-first projects. Can be converted to JSON Schema via
  `zod-to-json-schema` for cross-language use.
- **Protocol Buffers / FlatBuffers** — Binary schema languages with C++ and
  TypeScript codegen. **Overkill** for VST embedded UI (adds build complexity,
  not WebView-native).

**Best fit for this project:** **JSON Schema as the single source of truth**,
with TypeScript/Zod validators generated for the UI and C++ parsers/validators
generated for the backend.

#### Recommended Architecture

```
schema/bridge.schema.json  (single source of truth)
  │
  ├──> generate-ts.sh  ──> ui/src/generated/bridge.ts (TypeScript types + Zod validators)
  │                         ├─ export type BridgeSetParameter = { id: string; value: number }
  │                         └─ export const BridgeSetParameterSchema = z.object({...})
  │
  └──> generate-cpp.sh ──> source/vst/common/generated/bridge_schema.h (C++ structs + validators)
                            ├─ struct BridgeSetParameter { std::string id; double value; }
                            └─ bool validate(const nlohmann::json&, BridgeSetParameter&)
```

**Key principle:** Developers edit **only** `schema/bridge.schema.json`. Both
TypeScript and C++ code are auto-generated at build time.

#### Implementation Plan

**Phase 1: Schema definition**

Create `schema/bridge.schema.json` (JSON Schema Draft 2020-12) defining all
bridge message types:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://netsdrstation.dev/schemas/bridge.json",
  "title": "NetSDRStation Bridge Protocol",
  "description": "Message contract between Vue.js UI and C++ VST processor",
  "definitions": {
    "SetParameterMessage": {
      "type": "object",
      "required": ["type", "data"],
      "properties": {
        "type": { "const": "setParameter" },
        "data": {
          "type": "array",
          "minItems": 2,
          "maxItems": 2,
          "items": [
            { "type": "string", "enum": ["freqKhz", "lowCut", "highCut", "volume", "..."] },
            { "type": "number" }
          ]
        }
      }
    },
    "SetStationMessage": { ... },
    "DisconnectMessage": { ... },
    "UpdateVueStateMessage": { ... }
  }
}
```

**Phase 2: TypeScript code generation**

Use `json-schema-to-typescript` + `zod-to-json-schema` (inverse direction: JSON
Schema → Zod).

```sh
# schema/generate-ts.sh (called by Vite build hook)
npx json-schema-to-typescript schema/bridge.schema.json \
  --output ui/src/generated/bridge.ts
npx json-schema-to-zod schema/bridge.schema.json \
  --output ui/src/generated/bridge-validators.ts
```

Generated `ui/src/generated/bridge.ts`:
```ts
export type BridgeSetParameter = {
  type: "setParameter";
  data: [string, number];
};
```

Generated `ui/src/generated/bridge-validators.ts`:
```ts
import { z } from "zod";
export const BridgeSetParameterSchema = z.object({
  type: z.literal("setParameter"),
  data: z.tuple([z.enum(["freqKhz", "lowCut", ...]), z.number()]),
});
```

**Phase 3: C++ code generation**

Use a custom Python script (or C++ template engine) to convert JSON Schema →
C++ structs + validators. The `nlohmann/json` library (already used for bridge
parsing) supports schema validation via external libraries, but **manual
codegen is simpler** for this use case.

```sh
# schema/generate-cpp.py (called by CMake)
python3 schema/generate-cpp.py schema/bridge.schema.json \
  --output source/vst/common/generated/bridge_schema.h
```

Generated `bridge_schema.h`:
```cpp
#pragma once
#include <nlohmann/json.hpp>
#include <optional>

namespace netsdr::schema {

struct BridgeSetParameter {
  std::string type;  // must be "setParameter"
  std::array<std::variant<std::string, double>, 2> data;
};

// Validates JSON and populates `out` if valid.
inline bool parseSetParameter(const nlohmann::json& j, BridgeSetParameter& out) {
  if (!j.contains("type") || j["type"] != "setParameter") return false;
  if (!j.contains("data") || !j["data"].is_array() || j["data"].size() != 2) return false;
  // ... enum check for data[0], type check for data[1] ...
  out.type = j["type"];
  out.data[0] = j["data"][0].get<std::string>();
  out.data[1] = j["data"][1].get<double>();
  return true;
}

}  // namespace netsdr::schema
```

**Phase 4: Integrate into build**

- CMake: add `add_custom_command` to run `generate-cpp.py` before compiling
  `bridge_protocol.cpp`.
- Vite: add `vite-plugin-run` to run `generate-ts.sh` before TypeScript
  compilation.

**Phase 5: Refactor existing bridge code**

Replace manual parsers in `bridge_protocol.cpp` with generated validators:

```cpp
// Before (manual):
bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out) {
  auto j = nlohmann::json::parse(message, nullptr, false);
  if (j.is_discarded()) return false;
  // ... 20 lines of manual checks ...
}

// After (generated):
#include "vst/common/generated/bridge_schema.h"
bool parseSetParameterMessage(const std::string& message, BridgeSetParameter& out) {
  auto j = nlohmann::json::parse(message, nullptr, false);
  if (j.is_discarded()) return false;
  return netsdr::schema::parseSetParameter(j, out);
}
```

#### Benefits

1. **Single source of truth** — schema file is the authoritative contract.
2. **Type safety** — TypeScript and C++ types guaranteed to match (generated
   from same schema).
3. **Runtime validation** — Zod (UI) and generated validators (C++) catch
   malformed messages early.
4. **Automatic documentation** — JSON Schema can be rendered as API docs (e.g.
   via `docusaurus-plugin-json-schema`).
5. **Versioning** — schema file can be versioned; breaking changes trigger
   major version bump.
6. **Reduced boilerplate** — adding a new parameter = one schema edit, not 5
   manual file edits.

#### Risks & Mitigation

- **Risk:** Code generation adds build complexity.
  **Mitigation:** Use simple, transparent scripts (Python for C++, standard npm
  packages for TS). Commit generated files to git so builds work without
  codegen toolchain (regenerate only when schema changes).
- **Risk:** JSON Schema learning curve for team.
  **Mitigation:** Start with a minimal schema (2-3 message types), expand
  incrementally. JSON Schema syntax is simpler than TypeScript or C++.
- **Risk:** Performance overhead of runtime validation.
  **Mitigation:** Zod validation is <1 ms for typical messages. C++ validators
  are simple if-checks (no regex, no complex logic). Measure if needed.

#### Success Criteria

- [ ] `schema/bridge.schema.json` defines all M4 message types (setParameter,
      setStation, disconnect, updateVueState).
- [ ] TypeScript types and Zod validators auto-generated and used in
      `pluginService.ts`.
- [ ] C++ structs and validators auto-generated and used in
      `bridge_protocol.cpp`.
- [ ] Build process (CMake + Vite) runs code generation automatically.
- [ ] All existing bridge tests pass with the new generated code (no behavior
      change).
- [ ] Documentation: README in `schema/` explaining the workflow.

#### Files

**New:**
- `schema/bridge.schema.json` (JSON Schema definition)
- `schema/generate-ts.sh` (TypeScript codegen script)
- `schema/generate-cpp.py` (C++ codegen script)
- `schema/README.md` (developer guide)
- `ui/src/generated/bridge.ts` (generated TypeScript types)
- `ui/src/generated/bridge-validators.ts` (generated Zod schemas)
- `source/vst/common/generated/bridge_schema.h` (generated C++ validators)

**Modified:**
- `CMakeLists.txt` (add custom command for C++ codegen)
- `ui/vite.config.ts` (add plugin for TypeScript codegen)
- `source/vst/common/bridge_protocol.cpp` (use generated validators)
- `ui/src/services/pluginService.ts` (use generated Zod validators)

**Dependencies:**
- `json-schema-to-typescript` (npm, dev)
- `json-schema-to-zod` (npm, dev, or alternative: `zod-to-json-schema` inverse)
- `nlohmann/json` (already in project)
- Python 3.8+ (for C++ codegen script; already available in dev env)

#### Alternative: Zod-first (TypeScript-centric approach)

If the team prefers TypeScript-first development, an alternative is:

1. Define schemas in **Zod** (TypeScript).
2. Generate JSON Schema via `zod-to-json-schema` (for documentation).
3. Generate C++ manually from the Zod definitions (no automatic C++ codegen).

**Tradeoff:** Better TypeScript DX, but loses C++ type safety (manual C++
edits required). **Not recommended** for this project because the C++ side
(VST processor) is the source of truth for parameters.

---

### Step 2 — M4.2: UI scaffold & component library

**Prerequisite:** M4.1.5 (schema-based API) must be complete before
implementing UI components — all TypeScript types and Zod validators must be
generated and integrated into `pluginService.ts`.

Build the visual framework before implementing individual panels.

#### 2a. Dark SDR theme

Define a CSS custom-property palette in `master.css` mirroring the KiwiSDR
colour scheme:

```css
:root {
  --kiwi-bg:       #222;
  --kiwi-panel:    #333;
  --kiwi-border:   #555;
  --kiwi-text:     #ddd;
  --kiwi-accent:   #4CAF50;   /* w3-selection-green */
  --kiwi-warn:     hsl(0, 97%, 57%);  /* w3-red2 */
  --kiwi-input-bg: #444;
  --kiwi-font-sm:  11px;
  --kiwi-font-md:  12px;
  --kiwi-font-lg:  14px;
}
```

#### 2b. Component primitives

Create one `.vue` file per widget (all in `ui/src/components/`):

| Component | Props | Emits | Notes |
|-----------|-------|-------|-------|
| `KSlider.vue` | `modelValue`, `min`, `max`, `step`, `unit?`, `label?` | `update:modelValue` | `appearance:none`, 3 px track, 18 px thumb (w3_ext pattern) |
| `KNumberInput.vue` | `modelValue`, `min`, `max`, `step`, `unit?` | `update:modelValue` | Arrow-key increment supported |
| `KSelect.vue` | `modelValue`, `options: {value,label}[]` | `update:modelValue` | Dark styled `<select>` |
| `KToggle.vue` | `modelValue`, `label?` | `update:modelValue` | Checkbox-style toggle with green active state |
| `KButton.vue` | `label`, `active?` | `click` | Active = green background |
| `KReadout.vue` | `value`, `unit?`, `digits?` | — | Monospace readout (Consolas) |
| `KPanel.vue` | `title?` | — | Wrapper with `--kiwi-panel` bg + `--kiwi-border` border |
| `KStatusBadge.vue` | `state: 'ok'|'warn'|'error'`, `label` | — | Coloured dot + text |

> **Alternative — use a UI library:** Instead of building all primitives from
> scratch, consider [Naive UI](https://www.naiveui.com/) or
> [Element Plus](https://element-plus.org/) (both MIT) and theme them with the
> KiwiSDR palette. This reduces component work but adds a ~300 kB dependency
> that must be inlined by `vite-plugin-singlefile`. Recommend building the
> primitives manually (they are simple) to keep the bundle lean.

#### 2c. State store (`ui/src/store/kiwiStore.ts`)

Use Pinia (already implied by the Vue 3 stack, MIT license) for a central
reactive state store:

```ts
// ui/src/store/kiwiStore.ts
export const useKiwiStore = defineStore('kiwi', {
  state: () => ({
    station: '',
    connected: false,
    freqKhz: 14100.0,
    mode: 'am',
    lowCut: -4900,
    highCut: 4900,
    agcOn: true,
    agcThresh: -100,
    agcDecay: 1000,
    agcHang: false,
    agcSlope: 0,
    agcManGain: 0,
    volume: 1.0,
    mute: false,
    squelchOn: false,
    squelchThreshold: 0.0,
    nbOn: false, nbThreshold: 0.5,
    nrOn: false,
    wfOn: true, wfSpeed: 2, wfZoom: 0,
    wfMaxDb: -30, wfMinDb: -130, wfComp: false,
    signalLevel: -140,   // dBm, display only
    userCount: '?',      // display only
    gpsSync: false,      // display only
  }),
  actions: {
    setParam(name: string, value: number) {
      pluginService.setParameter(name, value);
      // optimistic update
      (this as any)[name] = value;
    },
  },
});
```

`pluginService.ts` already handles the bridge; add Pinia as the reactive
layer on top.

**Add Pinia to the project:**
```
npm install pinia
```
Update `ui/src/main.ts`: `app.use(createPinia())`.

#### 2d. Panel shell layout

The main `PluginView.vue` becomes a layout shell:

```
┌─────────────────── kiwi-layout ──────────────────┐
│ header: [Logo / station name / status badge]      │
├────────────────────────────────────────────────────┤
│ controls-row (flex-wrap):                          │
│  [FreqPanel] [ModePanel] [AGCPanel] [AudioPanel]  │
│  [WaterfallPanel] [ExtensionPanel]                 │
├────────────────────────────────────────────────────┤
│ waterfall canvas (flex: 1 1 auto)                  │
├────────────────────────────────────────────────────┤
│ status bar: [S-meter] [users] [GPS] [buffer]       │
└────────────────────────────────────────────────────┘
```

**Files:**
- `ui/src/components/*.vue` (new primitives)
- `ui/src/store/kiwiStore.ts` (new)
- `ui/src/views/PluginView.vue` (restructure as layout shell)
- `ui/src/services/pluginService.ts` (no change, bridge is stable)
- `ui/src/main.ts` (add Pinia)
- `ui/package.json` (add pinia)

**Tests:**
- Vitest: each primitive renders, emits correct value on interaction.
- Vitest: Pinia store `setParam` calls `pluginService.setParameter` with the
  right args.

---

### Step 3 — M4.3: Frequency & Tuning panel

The most important control panel; implement before mode/AGC panels.

#### Layout

```
┌── Frequency ──────────────────────────────────┐
│  [←10]  [←1]  [←0.1]   14100.000  [+0.1] [+1] [+10]  │
│  [large digital readout: 14,100.000 kHz]       │
└────────────────────────────────────────────────┘
```

#### Implementation

- `FreqPanel.vue`:
  - Step buttons call `store.setParam('freqKhz', store.freqKhz + delta)`.
  - `KNumberInput` (with `step=0.001`, `unit='kHz'`) bound to `store.freqKhz`.
  - Large `KReadout` showing the frequency in the KiwiSDR 7-digit format.
  - Keyboard shortcut: focus the input, type a frequency, press Enter.
- Passband dragger overlay (on the waterfall): defer to M4.7 (waterfall).

#### Bridge mapping

`store.setParam('freqKhz', val)` → `pluginService.setParameter('freqKhz', val)`
→ bridge `{"type":"setParameter","data":["freqKhz", val]}` → C++
`applyParamValue(kParamFreqKhz, normalized)` → `KiwiBridge::onFreq` →
`kiwiSetModFreqCommand(mode, lowCut, highCut, freqKhz)` → WebSocket.

**Files:**
- `ui/src/components/FreqPanel.vue`

**Tests:**
- Vitest: step buttons emit correct delta-adjusted values; clamped to
  `[0.001, 30000]` range.
- Vitest: manual text entry updates the store.

---

### Step 4 — M4.4: Modulation & Passband panel

#### Layout

```
┌── Mode & Passband ──────────────────────────────────────────┐
│  [AM] [AMN] [AMW] [USB] [USN] [LSB] [LSN] [CW] [CWN]       │
│  [NBFM] [NNFM] [IQ] [DRM] [SAM] [SAU] [SAL] [SAS] [QAM]   │
│  Low: [-4900 Hz]   High: [4900 Hz]   BW: [9800 Hz]  [Reset] │
└─────────────────────────────────────────────────────────────┘
```

#### Implementation

- `ModePanel.vue`:
  - Mode buttons: 18 `KButton` components in two rows; active = green.
  - Selecting a mode updates `store.mode` and applies the default passband
    for that mode (define a `MODE_DEFAULTS` map in `ModePanel.vue`).
  - `KNumberInput` ×3 for `lowCut`, `highCut`, bandwidth (bandwidth is
    derived = highCut − lowCut, read-only or auto-updates the others).
  - Reset button restores mode defaults.

#### Default passband table

| Mode | Low Cut | High Cut |
|------|---------|----------|
| AM   | -4900   | +4900 |
| AMN  | -2500   | +2500 |
| USB  | 300     | 2700 |
| LSB  | -2700   | -300 |
| CW   | 300     | 800 |
| CWN  | 400     | 600 |
| NBFM | -6000   | +6000 |
| IQ   | -5000   | +5000 |
| *(etc.)* | | |

**Files:**
- `ui/src/components/ModePanel.vue`

**Tests:**
- Vitest: each mode button sets mode + correct default passband.
- Vitest: Low Cut / High Cut inputs clamp to valid range.
- Vitest: bandwidth field stays in sync when Low/High Cut change.

---

### Step 5 — M4.5: Band presets & memory

#### Implementation

- `BandPanel.vue`:
  - Three `KSelect` dropdowns: Amateur, Broadcast, Utility/timesig.
  - Each `<option>` carries a frequency value; selecting one fires
    `store.setParam('freqKhz', bandFreq)`.
  - Memory / bookmark list: a `<ul>` of saved `{label, freqKhz, mode}` objects
    stored in `localStorage` (browser storage, no C++ change needed).
  - Save current: button that appends `{store.freqKhz, store.mode}` to the
    bookmark list.
  - Delete bookmark: trash icon next to each entry.

#### Band frequency table (key entries)

| Band | Frequency (kHz) |
|------|----------------|
| 160 m (amateur) | 1850 |
| 80 m | 3700 |
| 40 m | 7100 |
| 20 m | 14200 |
| MW broadcast | 720 |
| SW 49 m | 6100 |
| SW 31 m | 9700 |
| DCF77 | 77.5 |
| WWV | 10000 |

> **Alternative:** Fetch the complete band plan from a JSON file bundled in
> `ui/public/bands.json`. This makes the list editable without a code change.

**Files:**
- `ui/src/components/BandPanel.vue`
- `ui/public/bands.json` (optional — recommended for maintainability)

**Tests:**
- Vitest: band select emits correct `freqKhz`.
- Vitest: bookmark save/delete persists in `localStorage`.

---

### Step 6 — M4.6: Audio, AGC & signal processing panel

#### Layout

```
┌── Audio ──────────────────────────────────────────────┐
│ Volume: [slider ────────────────────── 80%] [Mute]    │
├── AGC ────────────────────────────────────────────────┤
│ [AGC On] Thresh: [-100 dB] Decay: [1000 ms] [Hang]   │
│ Slope: [0 dB]   ManGain: [0 dB]                       │
├── Processing ─────────────────────────────────────────┤
│ [Squelch On] Threshold: [slider]                      │
│ [NB On] NB Thresh: [slider]   [NR On]                 │
└───────────────────────────────────────────────────────┘
```

#### S-Meter

```
S-Meter: [S1─S2─S3─S4─S5─S6─S7─S8─S9─+10─+20─+30] -90 dBm
```

- The S-meter value is a **display-only** readout pushed from C++ via the
  bridge (`{"type":"level","data":[-90.0]}`).
- `SMeter.vue`: horizontal `<canvas>` bar; S1–S9 in grey, +10–+60 dB in
  yellow/red. Update on every `onMessage('level', val)`.
- Drive: the audio thread computes RMS over each `process()` block, converts to
  dBm, and posts to the UI via `webview_->eval(...)` at max 10 Hz (rate-limited
  via `RateLimiter` on the audio thread — but note: `eval` is NOT safe from the
  audio thread; post to the worker thread first, then eval).

> **Audio thread → UI bridge safety:** `webview_->eval()` is NOT RT-safe. The
> correct path is:
> 1. Audio thread computes RMS → writes to an `std::atomic<float>`.
> 2. Worker thread polls the atomic at 10 Hz → calls `eval()`.
> This pattern avoids any blocking call on the audio thread.

**Files:**
- `ui/src/components/AudioPanel.vue` (volume, mute, AGC, squelch, NB, NR)
- `ui/src/components/SMeter.vue`
- `source/vst/processor/plugin_processor.cpp` (RMS computation + atomic store)
- `source/editor/plugin_editor.cpp` (worker-thread polling → `eval("setLevel(...)");`)

**Tests:**
- Vitest: AGC/squelch/NB/NR toggles call `setParam` with correct ID.
- Vitest: S-meter updates bar position from a mocked `onMessage('level', val)`.
- Unit test: `plugin_processor` RMS computation matches expected value.

---

### Step 7 — M4.7: Waterfall & spectrum display

This is the most complex step and has a **dependency** on a separate waterfall
data stream. Do not block other steps on it.

#### 7a. Network layer extension — waterfall stream

KiwiSDR provides a second WebSocket stream for FFT/waterfall data
(`STREAM_WATERFALL` command). This is separate from the audio stream.

Add to `KiwiClient`:
```cpp
void startWaterfallStream();  // sends SET STREAM_WATERFALL
// Calls onWaterfallFrame(std::vector<uint8_t>) when data arrives
```

The waterfall frame is a compressed FFT bin array (typically 1024 bins).

> **Alternative for M4:** If the waterfall stream is too complex for M4,
> implement a **simulated spectrum** using the audio RMS + a simple
> periodogram computed from the audio buffer (using a DFT of the last N PCM
> samples). This gives a rough spectrum display without a second connection and
> can be replaced by the real waterfall stream in a later patch.

#### 7b. Waterfall canvas component

- `Waterfall.vue`: `<canvas>` that scrolls the waterfall image downward at the
  configured speed.
- Each new FFT frame is rendered as a horizontal line at the top.
- Colour map: map bin magnitude to an RGB colour using a `ColorMap` class
  (Default, Rain, Grayscale — define as simple lookup tables).
- Overlay: frequency cursor (yellow vertical line at `store.freqKhz`) and
  passband shading (`store.lowCut`–`store.highCut`).

#### 7c. Controls (M4.7 panel)

`WaterfallPanel.vue`:

| Control | Component | Notes |
|---------|-----------|-------|
| Zoom +/- | `KButton` (4 buttons) | Sends `wfZoom` parameter |
| WF Max dB | `KSlider` | Range -10..0 dBFS |
| WF Min dB | `KSlider` | Range -160..-60 dBFS |
| WF Speed | `KSelect` | Pause / Slow / Med / Fast |
| Color map | `KSelect` | Default / Rain / Grayscale |
| Display mode | `KToggle` group | WF / Spec / Both |
| CIC Comp | `KToggle` | wfComp param |

**Files:**
- `ui/src/components/Waterfall.vue`
- `ui/src/components/WaterfallPanel.vue`
- `source/network/kiwi_client.h/.cpp` (waterfall stream)
- `source/network/kiwi_commands.h/.cpp` (`kiwiWaterfallCommand`)

**Tests:**
- Vitest: `Waterfall.vue` renders a known FFT frame with correct pixel values.
- Integration: `KiwiClient` sends `STREAM_WATERFALL` frame; mock server
  returns a frame; `onWaterfallFrame` called with the frame data.

---

### Step 8 — M4.8: Status & extension panel

#### Status bar (always visible)

```
[S-meter] │ 0/4 Users │ GPS: ✓ │ Buffer: OK │ 14100.000 kHz
```

`StatusBar.vue`:
- User count: pushed from bridge (`{"type":"status","data":{"users":"0/4"}}`).
- GPS sync: pushed from bridge (`{"type":"status","data":{"gps":true}}`).
- Buffer/stream health: computed from jitter buffer fill level (pushed from
  C++ at 2 Hz).
- Exact frequency: `store.freqKhz` formatted to 3 decimals.

#### Extension select + panel

`ExtensionPanel.vue`:
- `KSelect` with options: CW Decoder, WFAX, RTTY/FSK, SSTV, tDoA, IQ Display,
  Antenna Switch (if present).
- Each extension panel is a separate `CwPanel.vue`, `WfaxPanel.vue`, etc.
  All are stubbed with "Coming soon" in M4; actual decoder DSP is deferred.
- Exception: `IqPanel.vue` can show an I/Q scatter plot with data already
  available from the audio stream.

**Files:**
- `ui/src/components/StatusBar.vue`
- `ui/src/components/ExtensionPanel.vue`
- `ui/src/components/extensions/` (stub panel components)
- `source/editor/plugin_editor.cpp` (push status messages at 2 Hz)

**Tests:**
- Vitest: `StatusBar.vue` updates user count from a mocked bridge message.
- Vitest: extension dropdown switches the displayed panel.

---

### Step 9 — M4.9: UI parity acceptance

Side-by-side comparison of the Vue UI against `kphsdr.com:8072`:

1. Open the real KiwiSDR in Chrome.
2. Open the VST plugin in VST3PluginTestHost.
3. Verify every control in `doc/ui-architecture.md` §3 is present and functional.
4. Resize the plugin editor; confirm reflow at narrow and wide sizes.
5. Run Playwright E2E tests.

**Playwright E2E test suite (`ui/e2e/`):**

| Test | Assertion |
|------|-----------|
| `freq-tuning.spec.ts` | Step buttons update frequency display; Enter in input updates store. |
| `mode-select.spec.ts` | Clicking USB applies default USB passband. |
| `agc.spec.ts` | AGC toggle sends `setParameter("agcOn", ...)` via bridge. |
| `band-presets.spec.ts` | Band dropdown sends correct `freqKhz`. |
| `resize.spec.ts` | At 640×400 all panels visible; at 1920×1080 waterfall fills space. |

**Files:**
- `ui/e2e/*.spec.ts`
- `doc/workspace-workflow.md` §3.7 (new manual acceptance section)

---

## Technology recommendations

| Choice | Recommendation | Alternative |
|--------|---------------|-------------|
| **Bridge API schema** | **JSON Schema (single source of truth)** | **Zod-first (TS-centric, but loses C++ type safety)** |
| **TS codegen** | **`json-schema-to-typescript` + `json-schema-to-zod`** | **Manual Zod definitions + `zod-to-json-schema`** |
| **C++ codegen** | **Custom Python script (`schema/generate-cpp.py`)** | **Manual C++ (no type safety, error-prone)** |
| **Runtime validation (TS)** | **Zod (40M+ npm downloads, best DX)** | **Ajv (faster, but worse DX), Valibot (smaller bundle)** |
| **Runtime validation (C++)** | **Generated validators (`nlohmann/json`)** | **JSON Schema validator library (overkill)** |
| State management | Pinia (MIT, official Vue 3 state library) | Vuex 4 (older, more boilerplate) |
| CSS approach | CSS custom properties + Flexbox/Grid (no Tailwind) | Tailwind CSS (adds 50 kB to bundle) |
| Canvas waterfall | Plain `<canvas>` + `ImageData` | WebGL shader (better perf, higher complexity) |
| Icons | Unicode / `data:svg` inline | FontAwesome (large, CDN-loaded) |
| Bundle | `vite-plugin-singlefile` (already in use) | Keep as is (important: Pinia adds ~7 kB inline) |
| E2E testing | Playwright (already planned as T2) | Cypress (heavier, slower) |

---

## Risk register

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| **Schema codegen adds build complexity** | **Medium** | **Use transparent scripts (Python for C++, standard npm for TS). Commit generated files to git so builds work without codegen (regenerate only when schema changes).** |
| **JSON Schema learning curve for team** | **Low** | **Start with minimal schema (2-3 message types), expand incrementally. JSON Schema syntax simpler than C++ or TypeScript.** |
| **Runtime validation performance overhead** | **Low** | **Zod validation <1 ms for typical messages. C++ validators are simple if-checks (no regex). Measure if needed.** |
| WebView2 canvas performance for 60 fps waterfall | Medium | Use `requestAnimationFrame` + `ImageData.set` (typed array, no per-pixel JS); profile early. |
| Waterfall stream protocol complexity | Medium | Implement simulated spectrum first; real waterfall stream as a follow-on. |
| `vite-plugin-singlefile` bundle size after Pinia | Low | Profile: Pinia ~7 kB; w3_ext ~12 kB; total expected < 120 kB inlined. |
| macOS/Linux resize (FIX-22 port) | Low | Platform wrappers clearly isolated in `webview_editor.cpp`; implement as soon as cross-platform build is active. |
| KiwiSDR waterfall binary format undocumented | Medium | Reference `waterfall.js` (`web/kiwi/waterfall.js`) in the upstream repo for the exact frame decoding. |
