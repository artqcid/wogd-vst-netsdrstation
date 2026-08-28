# M4 UI Replication Analysis — KiwiSDR 1:1 Vue Replica

_Stand: 2026-08-28. Vollständige Analyse der KiwiSDR-Browseroberfläche für die 1:1-
Reimplementierung in Vue 3 (`<script setup lang="ts">`). Erstellt durch systematisches
Research des Originalcodes (`github.com/jks-prv/KiwiSDR/web/kiwi/`) und Live-Instanzen
(kiwisdr.com/public). Diese Datei ist die einzige maßgebliche Quelle für das
UI-Design-Replikat; der bestehende `doc/M4-implementation-plan.md` beschreibt die
Backend-Architektur und bleibt unverändert._

---

## 1. Gesamtarchitektur des Originals

### 1.1 Layout-Hierarchie

Das KiwiSDR-Original verwendet ein vertikales Flex-Layout ohne Grid. Alle Bereiche sind
horizontal gestapelte Leisten. Von oben nach unten:

```
┌─────────────────────────────── 100vw ─────────────────────────────────┐
│  Header Bar          ~55px   #EAEAEA   3-Spalten Flex                 │
├───────────────────────────────────────────────────────────────────────┤
│  Band Scale Strip    ~20px   white     Pfeil + farbige Blöcke         │
├───────────────────────────────────────────────────────────────────────┤
│  Tag / DX Area       40-80px #aaa      farbige Frequenz-Tags          │
├───────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  Main Workspace      flex:1  #111      Canvas + schwebendes Panel     │
│   ├─ FrequencyRuler  ~25px             dunkle Leiste mit Ticks        │
│   ├─ Waterfall+Spec  flex:1  #000      HTML5 Canvas                   │
│   │   └─ PlayButton  absolut           violett, linker Rand           │
│   │   └─ PassbandOv  absolut           gelber Pfeil + Passband        │
│   └─ ControlPanel    absolut pos:abs   schwebendes Panel rechts unten │
└───────────────────────────────────────────────────────────────────────┘
```

**Root-CSS (Original `id-kiwi-container`):**
```css
display: flex;
flex-direction: column;
height: 100vh;
width: 100vw;
overflow: hidden;
background: #111;
```

**WICHTIG:** Das Original nutzt KEIN `App.vue`-Scale-Transform (kein 1280×720
fixed surface). Die Vue-App muss auf fluid `100vw × 100vh` umgestellt werden.

### 1.2 Technologie-Stack des Originals

- **CSS-Framework:** W3.CSS (externe CDN) + `web/kiwi/w3_ext.css` (KiwiSDR-Extensions)
- **Widget-Library:** `w3_util.js` — Sliders, Selects, Checkboxes, Icon-Buttons
- **Canvas-Engine:** natives `<canvas>` 2D-Context (ImageData für Waterfall-Pixel)
- **Fonts:** System-sans, Monospace (Consolas) für Frequenz-Readouts
- **Icons:** Font Awesome 4.x (CDN) + Unicode-Zeichen als Fallback

### 1.3 Farb-Palette (aus `kiwi.css` + `w3_ext.css`)

| Variable | Wert | Verwendung |
|---|---|---|
| `--kiwi-bg` | `#111` | Root-Background |
| `--kiwi-panel` | `#222` oder `#575757` | Panel-Hintergrund |
| `--kiwi-panel-light` | `#333` | Leichtere Panel-Variante |
| `--kiwi-border` | `#555` | Borders |
| `--kiwi-text` | `#ddd` oder `white` | Primärer Text |
| `--kiwi-text-muted` | `#909090` | Gedimmter Text |
| `--kiwi-accent` | `#4CAF50` | Selection-Green (KiwiSDR-Standard) |
| `--kiwi-accent-neon` | `#00FF00` | Aktiver Mode-Button |
| `--kiwi-warn` | `#e53935` | Warnung/Fehler |
| `--kiwi-topbar-bg` | `#EAEAEA` | Header-Hintergrund |
| `--kiwi-topbar-text` | `#404040` | Header-Primärtext |
| `--kiwi-input-bg` | `#000` | Frequenz-Input-BG |
| `--kiwi-input-border` | `#4af` | Frequenz-Input-Border |
| `--kiwi-tab-rf` | `#4CAF50` | RF-Tab |
| `--kiwi-tab-wf` | `#e53935` | WF0-Tab |
| `--kiwi-tab-audio` | `#1565c0` | Audio-Tab |
| `--kiwi-tab-agc` | `#6a1b9a` | AGC-Tab |
| `--kiwi-tab-user` | `#00838f` | User-Tab |
| `--kiwi-tab-stat` | `#e65100` | Stat-Tab |

---

## 2. Header Bar (A) — Exakt-Spezifikation

### 2.1 Layout

```
┌─ Logo+Info (flex:0 auto) ──┬── Host-Info (flex:1, zentriert) ──┬── User+Zeit (flex:0 auto) ─┐
│ [🐦] SA4BNA                │  SA4BNA - LA8GKA KIWI receiver 1  │ Your name or callsign:     │
│       LA8GKA KIWI 1        │  Share antenne...                  │ [________________]         │
│       SNR: 35 dB           │  sa4bna.hopto.org:8074            │ 12:34:56 UTC               │
│       Ant: 100 mtr beverage│                                    │ 14:34:56                   │
│                            │                                    │ CEST (UTC+2)               │
└────────────────────────────┴──────────────────────────────────────┴────────────────────────────┘
```

### 2.2 CSS-Details

```css
.kiwi-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  height: 55px;
  padding: 0 12px;
  background: #EAEAEA;
  color: #404040;
  flex-shrink: 0;
}

.kiwi-header__left {
  display: flex;
  align-items: center;
  gap: 8px;
}

.kiwi-header__logo {
  width: 40px;
  height: 40px;
  flex-shrink: 0;
}

.kiwi-header__title-stack {
  display: flex;
  flex-direction: column;
  font-size: 11px;
  line-height: 1.3;
}

.kiwi-header__title { font-weight: bold; font-size: 13px; }
.kiwi-header__sub { color: #606060; font-size: 10px; }

.kiwi-header__center {
  flex: 1;
  text-align: center;
  font-size: 11px;
  color: #505050;
}

.kiwi-header__link {
  color: #2196F3;
  text-decoration: underline;
  cursor: pointer;
}

.kiwi-header__right {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 10px;
  color: #606060;
}

.kiwi-header__callsign-input {
  background: white;
  border: 1px solid #ccc;
  padding: 2px 6px;
  font-size: 11px;
  width: 140px;
}

.kiwi-header__time-utc {
  font-size: 14px;
  font-weight: bold;
  color: #909090;
}

.kiwi-header__time-local { font-size: 10px; color: #909090; }
.kiwi-header__timezone { font-size: 8px; color: #909090; }
```

### 2.3 Kiwi-Logo (SVG)

Das originale Logo ist ein grüner Kiwi-Vogel. Für Vue wird ein vereinfachtes
Inline-SVG verwendet:

```html
<!-- ui/src/assets/kiwi-logo.svg -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 40 40">
  <circle cx="20" cy="20" r="19" fill="#4CAF50" stroke="#2e7d32" stroke-width="1"/>
  <!-- Kiwi-Vogelform — stilisiert -->
  <ellipse cx="22" cy="22" rx="12" ry="9" fill="#2e7d32"/>
  <circle cx="30" cy="18" r="4" fill="#2e7d32"/>
  <line x1="34" y1="17" x2="40" y2="16" stroke="#2e7d32" stroke-width="2"/>
  <circle cx="32" cy="17" r="1.5" fill="white"/>
</svg>
```

### 2.4 Vue-Komponent `KiwiHeader.vue`

```vue
<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'

const callsign = ref('')
const utcTime = ref('')
const localTime = ref('')
const tzName = ref('')

let timer: number

function updateTime() {
  const now = new Date()
  utcTime.value = now.toISOString().slice(11, 19) + ' UTC'
  localTime.value = now.toLocaleTimeString()
  tzName.value = Intl.DateTimeFormat().resolvedOptions().timeZone
}

onMounted(() => { updateTime(); timer = setInterval(updateTime, 1000) })
onBeforeUnmount(() => clearInterval(timer))
</script>
```

---

## 3. Band Scale Strip (B) — Exakt-Spezifikation

### 3.1 Layout

```
[◄]  [MW]  [49m][41m]  [31m 25m 22m 19m 16m 13m 11m]  [80m 60m 40m 30m 20m 17m 15m 12m 10m]  [►]
 0                      5 MHz                           10 MHz                          25 MHz   30 MHz
```

### 3.2 Band-Daten (aus KiwiSDR `kiwi.js`)

```typescript
// Broadcast bands (SW)
const BROADCAST_BANDS = [
  { label: 'LW',  freq: 0.2,  color: '#FF9800' },  // orange
  { label: 'MW',  freq: 0.6,  color: '#FF9800' },
  { label: '49m', freq: 5.9,  color: '#4fc3f7' },   // hellblau
  { label: '41m', freq: 7.2,  color: '#4fc3f7' },
  { label: '31m', freq: 9.5,  color: '#4fc3f7' },
  { label: '25m', freq: 11.6, color: '#4fc3f7' },
  { label: '22m', freq: 13.6, color: '#4fc3f7' },
  { label: '19m', freq: 15.1, color: '#4fc3f7' },
  { label: '16m', freq: 17.5, color: '#4fc3f7' },
  { label: '13m', freq: 21.5, color: '#4fc3f7' },
  { label: '11m', freq: 25.6, color: '#4fc3f7' },
]

// Amateur bands
const AMATEUR_BANDS = [
  { label: '160m', freq: 1.85,  color: '#ef5350' },  // rot
  { label: '80m',  freq: 3.7,   color: '#ef5350' },
  { label: '60m',  freq: 5.35,  color: '#ef5350' },
  { label: '40m',  freq: 7.1,   color: '#ef5350' },
  { label: '30m',  freq: 10.1,  color: '#ef5350' },
  { label: '20m',  freq: 14.2,  color: '#ef5350' },
  { label: '17m',  freq: 18.1,  color: '#ef5350' },
  { label: '15m',  freq: 21.2,  color: '#ef5350' },
  { label: '12m',  freq: 24.9,  color: '#ef5350' },
  { label: '10m',  freq: 28.0,  color: '#ef5350' },
]
```

### 3.3 Positionierungs-Logik

```typescript
// Frequenz in % umrechnen (0–30 MHz Gesamtbereich)
function freqToPercent(freqMhz: number, totalMhz = 30): number {
  return (freqMhz / totalMhz) * 100
}
```

### 3.4 CSS

```css
.band-scale {
  height: 20px;
  background: white;
  position: relative;
  display: flex;
  align-items: center;
  flex-shrink: 0;
  overflow: hidden;
}

.band-scale__arrow {
  font-size: 10px;
  padding: 0 4px;
  cursor: pointer;
  color: #555;
  flex-shrink: 0;
}

.band-scale__inner {
  flex: 1;
  position: relative;
  height: 100%;
}

.band-scale__block {
  position: absolute;
  height: 16px;
  top: 2px;
  border-radius: 3px;
  font-size: 9px;
  font-weight: bold;
  padding: 1px 2px;
  white-space: nowrap;
  cursor: pointer;
  color: black;
  overflow: hidden;
}
```

---

## 4. Tag / DX Area (C) — Exakt-Spezifikation

### 4.1 Layout

DX-Labels werden über die Frequenz positioniert. Im Original kommen sie aus dem
Server (`SET DX_UPD` / `dx.json`). Für M4 werden statische Demo-Tags verwendet.

```
┌──────────────────── grauer Hintergrund ────────────────────────────────────────────┐
│ [NAVTEX] [FT8][FT4] [FAX]     [CW-Bake]  [RTTY]  [WWV]   [CHU] [STA] [Sup]      │
│ lime      lime      yellow     lime       pink    orange  orange red   hotpink     │
└────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Farbcodes (aus `kiwi_ui.js` / `cl-dx-label`)

| Tag-Typ | Hintergrund | Text |
|---|---|---|
| NAVTEX/FT8/FT4 | `#4CAF50` (lime) | schwarz |
| FAX | `yellow` | schwarz |
| RTTY | `#f06292` (pink) | schwarz |
| STA (Station) | `orange` | schwarz |
| Sup (Suppressed) | `hotpink` | schwarz |
| Amateur | `#ef5350` (red) | weiß |
| Broadcast | `#4fc3f7` (hellblau) | schwarz |

### 4.3 Demo-Daten für M4

```typescript
const DEMO_DX_LABELS = [
  { freq: 0.518,  label: 'NAVTEX', color: '#4CAF50', textColor: 'black' },
  { freq: 7.074,  label: 'FT8',    color: '#4CAF50', textColor: 'black' },
  { freq: 10.136, label: 'FT8',    color: '#4CAF50', textColor: 'black' },
  { freq: 7.035,  label: 'FAX',    color: 'yellow',  textColor: 'black' },
  { freq: 14.230, label: 'SSTV',   color: '#f06292', textColor: 'black' },
  { freq: 10.000, label: 'WWV',    color: 'orange',  textColor: 'black' },
  { freq: 25.670, label: 'STA',    color: 'orange',  textColor: 'black' },
]
```

---

## 5. Main Workspace — Exakt-Spezifikation

### 5.1 Frequenz-Lineal

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ ▲ database: stored │  0 kHz    │   5 MHz    │  10 MHz    │  25 MHz   │  30 MHz   │
│   (#FFD700 dreieck)│           │            │            │            │           │
└──────────────────────────────────────────────────────────────────────────────────┘
```

```css
.freq-ruler {
  height: 25px;
  background: #333;
  position: relative;
  flex-shrink: 0;
  display: flex;
  align-items: flex-end;
}

.freq-ruler__tick {
  position: absolute;
  bottom: 0;
  width: 1px;
  height: 8px;
  background: white;
}

.freq-ruler__label {
  position: absolute;
  bottom: 10px;
  font-size: 9px;
  color: white;
  transform: translateX(-50%);
}

.freq-ruler__db-text {
  font-size: 9px;
  color: #FFD700;  /* gelb */
  padding: 2px 4px;
}
```

### 5.2 Waterfall Canvas

```css
.waterfall-container {
  flex: 1;
  position: relative;
  background: #000;
  overflow: hidden;
}

.waterfall-canvas {
  display: block;
  width: 100%;
  height: 100%;
}
```

**Floating Play Button:**
```css
.play-button-float {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%);
  width: 36px;
  height: 44px;
  background: #7c4dff;   /* violett */
  color: white;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 16px;
  border-radius: 0 6px 6px 0;
  cursor: pointer;
  z-index: 20;
  user-select: none;
}
```

### 5.3 Mouse-Wheel-Zoom-Logik

```typescript
// Exakt am Canvas/Container-Element hängen (NICHT global window)
function onWheel(e: WheelEvent) {
  e.preventDefault()
  const anchorX = e.offsetX  // Mausposition als Zoom-Anker
  const direction = e.deltaY > 0 ? -1 : 1
  const newZoom = Math.max(0, Math.min(14, store.wfZoom + direction))
  // Berechne neue Mittenfrequenz basierend auf Anker
  const canvasWidth = (e.target as HTMLElement).clientWidth
  const anchorFrac = anchorX / canvasWidth  // 0..1
  const oldSpanKhz = zoomToSpan(store.wfZoom)
  const newSpanKhz = zoomToSpan(newZoom)
  const freqAtAnchor = store.freqKhz - oldSpanKhz / 2 + anchorFrac * oldSpanKhz
  // Neue Mitte so, dass Anker-Frequenz gleich bleibt
  const newCenter = freqAtAnchor - anchorFrac * newSpanKhz + newSpanKhz / 2
  store.wfZoom = newZoom
  store.setParam('freqKhz', Math.max(0, Math.min(30000, newCenter)))
}

function zoomToSpan(zoom: number): number {
  // KiwiSDR: z0 = full 30 MHz, z14 = ~2 kHz
  return 30000 / Math.pow(2, zoom)  // kHz
}
```

### 5.4 Passband Filter Overlay

```vue
<div class="passband-overlay" @mousedown="onDragStart" :class="{ dragging: isDragging }">
  <!-- Gelber Pfeil am oberen Rand -->
  <div class="passband-cursor" :style="{ left: cursorX + 'px' }">▲</div>
  <!-- Passband-Shading -->
  <div class="passband-shade"
    :style="{ left: passbandLeft + 'px', width: passbandWidth + 'px' }">
  </div>
</div>
```

```typescript
function onDragStart(e: MouseEvent) {
  isDragging.value = true
  document.addEventListener('mousemove', onDragMove)
  document.addEventListener('mouseup', onDragEnd)
}

function onDragMove(e: MouseEvent) {
  if (!isDragging.value) return
  const deltaX = e.movementX  // Pixel-Delta
  const freqPerPixel = spanKhz.value / canvasWidth.value
  store.setParam('freqKhz', store.freqKhz + deltaX * freqPerPixel)
}
```

---

## 6. Control Panel — Vollständige Zeile-für-Zeile-Spezifikation

### 6.1 Container

```css
.control-panel {
  position: absolute;
  bottom: 15px;
  right: 0;
  z-index: 100;
  width: 360px;
  background: #222;
  border-radius: 8px 0 0 8px;
  border: 1px solid #555;
  border-right: none;
  color: #ddd;
  font-size: 11px;
  overflow: hidden;
  transition: transform 0.3s ease;
}

.control-panel--closed {
  transform: translateX(100%);
}

.control-panel__tab {
  position: absolute;
  left: -20px;
  top: 50%;
  transform: translateY(-50%);
  width: 20px;
  height: 60px;
  background: #444;
  border-radius: 6px 0 0 6px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  color: white;
  font-size: 12px;
  border: 1px solid #555;
  border-right: none;
}
```

### 6.2 Zeile 1: Frequenz-Input + Dropdowns + Play

```
┌────────────────────────────────────────────────────────────────────────┐
│ [ 7771.96  ] [select band ∨] [extension ∨]                      [▶]  │
│  schwarz/weiß  grau select      grau select                  grau btn  │
└────────────────────────────────────────────────────────────────────────┘
```

```css
.panel-row-1 {
  display: flex;
  gap: 4px;
  align-items: center;
  padding: 4px 8px;
}

.freq-input {
  background: #000;
  color: white;
  border: 1px solid #4af;
  padding: 2px 6px;
  font-size: 13px;
  font-family: Consolas, monospace;
  width: 90px;
}

.panel-select {
  background: #444;
  color: #ddd;
  border: 1px solid #666;
  font-size: 10px;
  padding: 2px 2px;
  flex: 1;
}

.play-btn-panel {
  width: 28px;
  height: 22px;
  background: #555;
  color: white;
  border: 1px solid #777;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-size: 10px;
}
```

### 6.3 Zeile 2: Mini-Icons

```
[≡] [●A] [↗] [●9]  [⊖][⊖][⊕][⊕]  [⊘]  Spectrum  [↻]red  [🔊]green
```

```typescript
const MINI_ICONS = [
  { icon: '≡', action: 'menu',    title: 'Menu' },
  { icon: 'A', action: 'dx',      title: 'DX Labels', cyan: true },
  { icon: '↗', action: 'link',    title: 'Open link', green: true },
  { icon: '9', action: 'users',   title: 'Users', green: true },
  // Zoom buttons
  { icon: '⊖', action: 'zoomOut2', title: 'Zoom out max' },
  { icon: '⊖', action: 'zoomOut1', title: 'Zoom out' },
  { icon: '⊕', action: 'zoomIn1',  title: 'Zoom in' },
  { icon: '⊕', action: 'zoomIn2',  title: 'Zoom in max' },
  { icon: '⊘', action: 'cicComp', title: 'CIC Comp toggle' },
]
```

### 6.4 Zeile 3: Mode-Buttons

```
[AM] [SAM] [DRM] [LSB] [USB] [CW] [NBFM] [IQ]
```

Aktiver Modus (z.B. `LSB`): `background: #00FF00; color: #000; font-weight: bold`
Inaktive Modi: `background: #444; color: #888; border: 1px solid #666`

```typescript
const MODE_LABELS_SHORT = ['AM', 'SAM', 'DRM', 'LSB', 'USB', 'CW', 'NBFM', 'IQ']
// Mapping auf KiwiSDR mode-Index (vollständige 18-Moden-Liste in kiwiStore.ts)
const MODE_INDEX_MAP: Record<string, number> = {
  'AM': 0, 'SAM': 11, 'DRM': 8, 'LSB': 3, 'USB': 2, 'CW': 4, 'NBFM': 6, 'IQ': 7
}
```

### 6.5 Zeile 4: Navigations-Buttons

```
[⊕] [⊖] [⇔] [⇕] [◁()] [()▷]
```

| Button | Icon | Aktion |
|---|---|---|
| Zoom In | `⊕` oder `fa-search-plus` | `store.wfZoom++` |
| Zoom Out | `⊖` oder `fa-search-minus` | `store.wfZoom--` |
| Pan/Expand | `↔` | Passband erweitern |
| Reset | `↕` | Passband auf Mode-Default |
| Freq-Step- | `◁()` | `freqKhz -= stepKhz` |
| Freq-Step+ | `()▷` | `freqKhz += stepKhz` |

```css
.nav-btn {
  width: 28px;
  height: 26px;
  background: #3a3a3a;
  color: #ccc;
  border: 1px solid #555;
  border-radius: 3px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-size: 13px;
}
.nav-btn:hover { background: #4a4a4a; color: white; }
.nav-btn:active { background: #555; }
```

### 6.6 Zeile 5: Farbige Sub-Tabs

```
[RF] [WF0] [Audio] [AGC] [User] [Stat] [Off]
grün  rot    blau  violett cyan   amber schwarz
```

```typescript
const TABS = [
  { id: 'RF',    label: 'RF',    bg: '#4CAF50', fg: 'black' },
  { id: 'WF0',   label: 'WF0',   bg: '#e53935', fg: 'white' },
  { id: 'Audio', label: 'Audio', bg: '#1565c0', fg: 'white' },
  { id: 'AGC',   label: 'AGC',   bg: '#6a1b9a', fg: 'white' },
  { id: 'User',  label: 'User',  bg: '#00838f', fg: 'white' },
  { id: 'Stat',  label: 'Stat',  bg: '#e65100', fg: 'white' },
  { id: 'Off',   label: 'Off',   bg: '#111',    fg: '#888' },
] as const
```

### 6.7 Zeile 6: Colormap-Bar (Rainbow-Gradient)

```css
.colormap-bar {
  height: 12px;
  width: 100%;
  background: linear-gradient(to right,
    #000000,   /* Schwarz */
    #0000ff,   /* Blau */
    #00ffff,   /* Cyan */
    #00ff00,   /* Grün */
    #ffff00,   /* Gelb */
    #ff0000,   /* Rot */
    #ff00ff,   /* Magenta */
    #ffffff    /* Weiß */
  );
  cursor: crosshair;
  flex-shrink: 0;
}
```

Klick → X-Position → berechne dB-Wert und setze `wfMaxDb` (rechte Hälfte) oder
`wfMinDb` (linke Hälfte).

### 6.8 Zeilen 7–10: WF0-Tab-Controls

**Gemeinsames Zeilen-Layout:**
```css
.panel-control-row {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 3px 8px;
  min-height: 24px;
}

.panel-label {
  width: 50px;
  font-size: 10px;
  color: #aaa;
  flex-shrink: 0;
}

.panel-value {
  width: 40px;
  font-size: 10px;
  color: #ddd;
  text-align: right;
  flex-shrink: 0;
}

.panel-btn-green {
  background: #4CAF50;
  color: black;
  border: none;
  padding: 1px 6px;
  font-size: 10px;
  cursor: pointer;
  border-radius: 2px;
}

.panel-btn-gray {
  background: #555;
  color: #ddd;
  border: 1px solid #777;
  padding: 1px 6px;
  font-size: 10px;
  cursor: pointer;
  border-radius: 2px;
}

.panel-btn-violet {
  background: #7c4dff;
  color: white;
  border: none;
  padding: 1px 6px;
  font-size: 10px;
  cursor: pointer;
  border-radius: 2px;
}
```

| Zeile | Label | Slider-Range | Wert-Text | Button |
|---|---|---|---|---|
| 7 | `WF ceil` | `wfMaxDb` (-10..0) | `+5 dB` | Grün `Auto Scale` |
| 8 | `WF floor` | `wfMinDb` (-160..-60) | `0 dB` | Grau `Spec Color` |
| 9 | `WF rate` | `wfSpeed` (0..4) | `fast` | — |
| 10 | `Spec Δ` | `apertureParam` (0..2) | `0.2 gain` | Violett `P1` |

### 6.9 Zeile 11: 4 Dropdowns + P2

```
[Kiwi ∨] [auto ∨] [off ∨] [IIR ∨]  ▼  [P2]violett
```

```typescript
const ROW11_DROPDOWNS = [
  { id: 'cmap',    label: 'Kiwi',  options: ['Kiwi', 'Rain', 'Soft', 'BW', 'Grey'] },
  { id: 'aper',    label: 'auto',  options: ['auto', 'IIR', 'MMA', 'EMA', 'off'] },
  { id: 'tstamp',  label: 'off',   options: ['off', '2s', '5s', '10s', '1m', '60m'] },
  { id: 'winf',    label: 'IIR',   options: ['IIR', 'MMA', 'EMA', 'off'] },
]
```

### 6.10 Footer: S-Meter

```
S1   S3   S5   S7   S9   +10  +20  +40  +60    -94.8 dBm
[===========================▮         ] grün→rot
```

S-Meter-Canvas:
- Breite: 100% des Panels, Höhe: 20px
- Gradient: links `#4CAF50` (grün) → Mitte `#ffeb3b` (gelb) → rechts `#f44336` (rot)
- Indikator: hellgrüner senkrechter Balken bei `(store.signalLevel + 127) / 127 * width`
- Text-Legende: `S1` (S1–S9 bei 6-dB-Abständen), `+10`, `+20`, `+40`, `+60`
- Aktueller dBm-Wert rechts: `font-size: 10px; color: #4CAF50`

---

## 7. Slider-Styling (w3_ext-Pattern)

Das Original nutzt `appearance: none` mit custom Thumb. Für Vue:

```css
/* Global in kiwi-theme.css */
input[type="range"].kiwi-slider {
  -webkit-appearance: none;
  appearance: none;
  height: 3px;
  background: #555;
  border-radius: 2px;
  outline: none;
  cursor: pointer;
  flex: 1;
}

input[type="range"].kiwi-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: #4CAF50;
  cursor: pointer;
  border: 2px solid #333;
}

input[type="range"].kiwi-slider::-moz-range-thumb {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: #4CAF50;
  cursor: pointer;
  border: 2px solid #333;
}
```

---

## 8. Vue-Komponentenstruktur nach M4

```
App.vue                      — 100vw × 100vh fluid (kein Scale-Transform)
└─ PluginView.vue            — Flex-Kolumne, Root-Container
   ├─ KiwiHeader.vue         — Header Bar (M4.2) [NEU]
   ├─ BandScaleBar.vue       — Band Scale Strip (M4.3) [NEU]
   ├─ TagArea.vue            — Tag / DX Area (M4.4) [NEU]
   └─ MainWorkspace.vue      — Flex:1, position:relative [NEU oder PluginView inline]
      ├─ FrequencyRuler.vue  — Frequenz-Lineal am Canvas-Top (M4.5) [NEU]
      ├─ Waterfall.vue       — Canvas, Mouse-Wheel-Zoom (M4.6) [ERWEITERN]
      ├─ PassbandOverlay.vue — Drag-Drop Passband (M4.7) [NEU]
      └─ ControlPanel.vue    — Schwebendes Panel, alle Rows (M4.8–M4.17) [NEU]
         ├─ Row1: FreqInput + Selects + PlayBtn
         ├─ Row2: MiniIcons
         ├─ Row3: ModeButtons
         ├─ Row4: NavButtons
         ├─ Row5: SubTabs
         ├─ Row6: ColormapBar
         ├─ Row7–10: Slider-Controls (v-show="activeTab === 'WF0'")
         ├─ Row11: Dropdowns + P2
         └─ Footer: SMeter.vue (ANPASSEN)
```

**Zu behaltende Komponenten (minimal anpassen):**
- `Waterfall.vue` — Canvas-Logik beibehalten, Zoom+MouseWheel ergänzen
- `SMeter.vue` — Canvas-Logik beibehalten, in Panel einbetten
- `kiwiStore.ts` — unverändert (alle Parameter bereits vorhanden)
- `pluginService.ts` — unverändert (Bridge fertig)

**Zu entfernende Komponenten:**
- `KPanel.vue` — wird nicht mehr gebraucht (kein Panel-Wrapper-Konzept)
- `AudioPanel.vue`, `WaterfallPanel.vue`, `ExtensionPanel.vue` — gehen in `ControlPanel.vue` auf
- `ModePanel.vue`, `BandPanel.vue` — Logik wandert in Panel-Rows oder `ControlPanel.vue`
- `FreqPanel.vue` — wandert in Panel Row 1

**Zu behaltenden Komponenten (Primitives):**
- `KSlider.vue`, `KButton.vue`, `KSelect.vue`, `KToggle.vue`, `KReadout.vue` — beibehalten
- `StationInput.vue`, `StatusBar.vue`, `KStatusBadge.vue` — beibehalten

---

## 9. App.vue: Scale-Transform entfernen

Das bisherige Design (1280×720 fixed canvas, uniform scale) passt NICHT zum
KiwiSDR-Replikat (fluid, responsive). `App.vue` muss vereinfacht werden:

**Vorher (ENTFERNEN):**
```vue
<div class="ui-surface" :style="{
  width: REF_WIDTH + 'px',
  height: REF_HEIGHT + 'px',
  transform: `scale(${scale})`,
}">
```

**Nachher:**
```vue
<template>
  <PluginView />
</template>

<style>
html, body, #app {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;
}
</style>
```

---

## 10. Bekannte Abweichungen & Kompromisse

| Aspekt | Original | Vue-Replikat M4 |
|---|---|---|
| DX-Labels | Server-seitig, dynamisch | Statische Demo-Daten |
| Kiwi-Vogel-Logo | Originalgrafik (PNG) | Stilisiertes SVG |
| Font Awesome Icons | CDN (fa-*) | Unicode-Zeichen + inline SVG |
| W3.CSS | Externes CDN | Eigene CSS-Variablen (Subset) |
| Echte Wasserfall-Daten | KiwiSDR WebSocket-Stream | Aus Store (M3 Bridge) |
| Extension-Panels | 15+ Extensions | Stubs (M5+) |
| Mobile Layout | Kein mobiles Layout | Kein mobiles Layout (VST) |
| Admin-Panel | Separate HTML-Seite | Nicht implementiert |

---

## 11. Implementierungsreihenfolge

1. **M4.18** — CSS-Palette zuerst (alle anderen Komponenten bauen darauf auf)
2. **M4.1** — Root-Layout umstellen (App.vue + PluginView.vue fluid)
3. **M4.2** — Header Bar (sofort sichtbar, gibt Gefühl für das Layout)
4. **M4.8** — Panel-Container erstellen (als leeres, schwebendes Div)
5. **M4.9 – M4.17** — Panel Rows der Reihe nach füllen
6. **M4.3** — Band Scale Strip
7. **M4.4** — Tag Area
8. **M4.5** — Frequenz-Lineal
9. **M4.6** — Waterfall Mouse-Wheel-Zoom
10. **M4.7** — Passband Overlay
11. **M4.19** — Tests

---

## 12. Referenz-Links

- Quellcode: https://github.com/jks-prv/KiwiSDR/tree/master/web/kiwi
- Live-CSS: https://raw.githubusercontent.com/jks-prv/KiwiSDR/master/web/kiwi/kiwi.css
- Live-CSS: https://raw.githubusercontent.com/jks-prv/KiwiSDR/master/web/kiwi/w3_ext.css
- Live-JS: https://raw.githubusercontent.com/jks-prv/KiwiSDR/master/web/kiwi/kiwi.js
- Waterfall-JS: https://raw.githubusercontent.com/jks-prv/KiwiSDR/master/web/kiwi/waterfall.js
- Öffentliche Instanzen: http://rx.kiwisdr.com (Liste), http://kiwisdr.com/public
