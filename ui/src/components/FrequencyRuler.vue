<template>
  <div class="freq-ruler" aria-label="Frequency ruler" @wheel.prevent="onWheel">
    <span class="freq-ruler__db-text">▲ database: stored</span>
    <div class="freq-ruler__scale" ref="scaleEl">
      <template v-for="tick in ticks" :key="tick.freq">
        <span
          class="freq-ruler__tick"
          :style="{ left: tick.pct + '%' }"
        ></span>
        <span
          class="freq-ruler__label"
          :style="{ left: tick.pct + '%' }"
        >{{ tick.label }}</span>
      </template>
    </div>
    <!-- SVG cursor overlay — green (expanded / zoomed-in): passband width drawn in scale units -->
    <svg
      class="freq-ruler__cursor-svg"
      :style="{ left: loPct + '%', width: Math.max(bwPct, 2) + '%' }"
      v-if="isZoomedIn"
      viewBox="0 0 100 26"
      preserveAspectRatio="none"
      @mousedown.prevent="onCursorMouseDown"
    >
      <!-- Body hit area (tune) — full passband width -->
      <rect class="cursor-hit cursor-hit--body"
        x="0" y="0"
        width="100"
        height="26"
        @mousedown.stop="onBodyMouseDown"
      />
      <!-- Left flank hit area (low-cut resize) — x=0..15 in viewBox coords -->
      <rect class="cursor-hit cursor-hit--lo"
        x="0" y="0"
        width="15"
        height="26"
        @mousedown.stop="onLoMouseDown"
      />
      <!-- Right flank hit area (high-cut resize) — x=85..100 in viewBox coords -->
      <rect class="cursor-hit cursor-hit--hi"
        x="85" y="0"
        width="15"
        height="26"
        @mousedown.stop="onHiMouseDown"
      />
      <!-- Upper horizontal bar = exact passband (full viewBox width) -->
      <rect class="cursor-bar-top"
        x="0" y="2"
        width="100"
        height="4"
        fill="#00FF00"
      />
      <!-- Center vertical line = carrier -->
      <line class="cursor-center-line"
        x1="50" y1="2"
        x2="50" y2="24"
        stroke="#00FF00"
        stroke-width="1.5"
      />
      <!-- Left flank (filter roll-off) — body left edge x=15 -->
      <polygon class="cursor-flap cursor-flap--lo"
        points="15,6 15,24 9,24"
        fill="#00FF00"
        opacity="0.7"
      />
      <!-- Right flank (filter roll-off) — body right edge x=85 -->
      <polygon class="cursor-flap cursor-flap--hi"
        points="85,6 85,24 91,24"
        fill="#00FF00"
        opacity="0.7"
      />
    </svg>
    <svg
      class="freq-ruler__cursor-svg"
      :style="{ left: cursorPct + '%', width: cursorWidth + 'px' }"
      v-else
      @mousedown.prevent="onCursorMouseDown"
    >
      <!-- Body hit area (tune) — whole cursor is draggable -->
      <rect class="cursor-hit cursor-hit--body"
        x="0" y="0"
        :width="cursorWidth"
        height="26"
        @mousedown.stop="onBodyMouseDown"
      />
      <!-- Yellow collapsed T/trapezoid shape -->
      <!-- Top horizontal bar -->
      <rect class="cursor-bar-top"
        x="cursorWidth / 2 - 12"
        y="2"
        width="24"
        height="4"
        fill="#ffff00"
      />
      <!-- Left diagonal (trapezoid upper-left) -->
      <polygon class="cursor-flap cursor-flap--lo"
        points="cursorWidth / 2 - 12,6 cursorWidth / 2 - 12,24 cursorWidth / 2 - 18,24"
        fill="#ffff00"
        opacity="0.7"
      />
      <!-- Right diagonal (trapezoid upper-right) -->
      <polygon class="cursor-flap cursor-flap--hi"
        points="cursorWidth / 2 + 12,6 cursorWidth / 2 + 12,24 cursorWidth / 2 + 18,24"
        fill="#ffff00"
        opacity="0.7"
      />
      <!-- Center tick -->
      <line class="cursor-center-line"
        x1="cursorWidth / 2" y1="2"
        x2="cursorWidth / 2" y2="24"
        stroke="#ffff00"
        stroke-width="1.5"
      />
    </svg>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

const props = withDefaults(defineProps<{
  /** Low frequency of visible window in kHz */
  viewLowKhz?: number
  /** High frequency of visible window in kHz */
  viewHighKhz?: number
  /** current frequency (store.freqKhz) */
  cursorKhz?: number
  /** lower passband edge in Hz */
  lowCutHz?: number
  /** upper passband edge in Hz */
  highCutHz?: number
  /** 0..14 (store.wfZoom) */
  zoomLevel?: number
}>(), {
  viewLowKhz: 0,
  viewHighKhz: 30000,
  cursorKhz: 7000,
  lowCutHz: -4900,
  highCutHz: 4900,
  zoomLevel: 0,
})

const emit = defineEmits<{
  (e: 'tune', freqKhz: number): void
  (e: 'zoom', delta: number, anchorFrac: number): void
  (e: 'low-cut', hz: number): void
  (e: 'high-cut', hz: number): void
}>()

interface Tick {
  freq: number   // kHz
  label: string
  pct: number    // 0-100
}

const scaleEl = ref<HTMLDivElement | null>(null)

/**
 * Generate tick marks at sensible intervals.
 * At full zoom (0–30 MHz): every 5 MHz.
 * Could be extended for zoom levels later.
 */
const ticks = computed<Tick[]>(() => {
  const low  = props.viewLowKhz
  const high = props.viewHighKhz
  const span = high - low
  if (span <= 0) return []

  // Choose step based on span
  let stepKhz: number
  if (span >= 20000)      stepKhz = 5000   // 5 MHz
  else if (span >= 10000) stepKhz = 2000   // 2 MHz
  else if (span >= 5000)  stepKhz = 1000   // 1 MHz
  else if (span >= 1000)  stepKhz = 200    // 200 kHz
  else if (span >= 200)   stepKhz = 50     // 50 kHz
  else if (span >= 50)    stepKhz = 10     // 10 kHz
  else if (span >= 10)    stepKhz = 2      // 2 kHz
  else if (span >= 2)     stepKhz = 0.5    // 500 Hz
  else                    stepKhz = 0.1    // 100 Hz

  const result: Tick[] = []
  const start = Math.ceil(low / stepKhz) * stepKhz

  for (let f = start; f <= high; f += stepKhz) {
    const pct = ((f - low) / span) * 100
    result.push({
      freq: f,
      label: formatFreq(f),
      pct,
    })
  }
  return result
})

function formatFreq(khz: number): string {
  if (khz === 0) return '0'
  if (khz >= 1000) return `${(khz / 1000).toFixed(khz % 1000 === 0 ? 0 : 1)} MHz`
  if (khz >= 1)    return `${khz % 1 === 0 ? khz : khz.toFixed(1)} kHz`
  return `${Math.round(khz * 1000)} Hz`
}

// ---- Cursor overlay computed positions ----

const MIN_PASSBAND_HZ = 4        // min_passband
const LOW_CUT_LIMIT_HZ = -6000   // low_cut_limit (12 kHz Audio-Rate)
const HIGH_CUT_LIMIT_HZ = 6000   // high_cut_limit
const cursorWidth = 40           // yellow collapsed cursor width (iconic T-shape)

const isZoomedIn = computed(() =>
  props.cursorKhz >= props.viewLowKhz && props.cursorKhz <= props.viewHighKhz
)

const cursorPct = computed(() => {
  const span = props.viewHighKhz - props.viewLowKhz
  if (span <= 0) return 0
  return ((props.cursorKhz - props.viewLowKhz) / span) * 100
})

// bwPct as 0..100 over scale (used by green SVG: left=loPct%, width=bwPct%)
const loPct = computed(() => {
  const span = props.viewHighKhz - props.viewLowKhz
  if (span <= 0) return 0
  return ((props.cursorKhz + props.lowCutHz / 1000 - props.viewLowKhz) / span) * 100
})

const hiPct = computed(() => {
  const span = props.viewHighKhz - props.viewLowKhz
  if (span <= 0) return 0
  return ((props.cursorKhz + props.highCutHz / 1000 - props.viewLowKhz) / span) * 100
})

const bwPct = computed(() => hiPct.value - loPct.value)

// ---- Drag state machine ----

type DragType = 'cursor' | 'lo' | 'hi'

interface DragState {
  type: DragType
  startX: number
  startFreq: number
  startLo: number
  startHi: number
}

let dragState: DragState | null = null

function onCursorMouseDown(e: MouseEvent) {
  // Legacy: full-cursor drag (body zone) — delegated from body hit area
  onBodyMouseDown(e)
}

function onBodyMouseDown(e: MouseEvent) {
  dragState = { type: 'cursor', startX: e.clientX, startFreq: props.cursorKhz, startLo: 0, startHi: 0 }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onLoMouseDown(e: MouseEvent) {
  dragState = { type: 'lo', startX: e.clientX, startFreq: 0, startLo: props.lowCutHz, startHi: props.highCutHz }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onHiMouseDown(e: MouseEvent) {
  dragState = { type: 'hi', startX: e.clientX, startFreq: 0, startLo: props.lowCutHz, startHi: props.highCutHz }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onDragMove(e: MouseEvent) {
  if (!dragState) return
  const span = props.viewHighKhz - props.viewLowKhz
  const el = scaleEl.value
  if (!el || span <= 0) return
  const rect = el.getBoundingClientRect()
  const pxPerKhz = rect.width / span
  const deltaHz = (e.clientX - dragState.startX) * pxPerKhz * 1000

  if (dragState.type === 'cursor') {
    const newFreq = Math.max(props.viewLowKhz, Math.min(props.viewHighKhz, dragState.startFreq + deltaHz / 1000))
    emit('tune', newFreq)
  } else if (dragState.type === 'lo') {
    const newLo = clamp(dragState.startLo + deltaHz, LOW_CUT_LIMIT_HZ, dragState.startHi - MIN_PASSBAND_HZ)
    emit('low-cut', newLo)
  } else if (dragState.type === 'hi') {
    const newHi = clamp(dragState.startHi + deltaHz, dragState.startLo + MIN_PASSBAND_HZ, HIGH_CUT_LIMIT_HZ)
    emit('high-cut', newHi)
  }
}

function clamp(v: number, lo: number, hi: number): number {
  return Math.max(lo, Math.min(hi, v))
}

function onDragUp() {
  dragState = null
  window.removeEventListener('mousemove', onDragMove)
  window.removeEventListener('mouseup', onDragUp)
}

// ---- Wheel zoom handler ----

function onWheel(e: WheelEvent) {
  e.preventDefault()
  const el = scaleEl.value
  if (!el) return
  const rect = el.getBoundingClientRect()
  const offsetX = e.clientX - rect.left
  const anchorFrac = Math.max(0, Math.min(1, offsetX / (rect.width || 1)))
  const delta = e.deltaY > 0 ? -1 : 1
  emit('zoom', delta, anchorFrac)
}
</script>

<style scoped>
.freq-ruler {
  display: flex;
  align-items: stretch;
  height: 26px;
  background: #2a2a2a;
  flex-shrink: 0;
  border-bottom: 1px solid #444;
  overflow: hidden;
}

.freq-ruler__db-text {
  font-size: 9px;
  color: #FFD700;
  padding: 2px 6px 0;
  flex-shrink: 0;
  white-space: nowrap;
  align-self: center;
}

.freq-ruler__scale {
  flex: 1;
  position: relative;
  overflow: hidden;
  z-index: 1;
}

.freq-ruler__tick {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 1px;
  background: rgba(255, 255, 255, 0.35);
}

.freq-ruler__label {
  position: absolute;
  bottom: 2px;
  font-size: 9px;
  color: white;
  white-space: nowrap;
  transform: translateX(-50%);
  pointer-events: none;
}

/* SVG cursor overlay */
.freq-ruler__cursor-svg {
  position: absolute;
  top: 0;
  height: 26px;
  z-index: 2;
  overflow: visible;
  pointer-events: none;  /* root passes through, hit-rects handle events */
  transform: translateX(-50%);
}

.cursor-hit {
  fill: transparent;
  pointer-events: auto;
  cursor: ew-resize;
}

.cursor-hit--body {
  cursor: ew-resize;  /* tune drag */
}

.cursor-hit--lo,
.cursor-hit--hi {
  cursor: ew-resize;  /* flank resize */
}

.cursor-bar-top {
  pointer-events: none;
}

.cursor-center-line {
  pointer-events: none;
}

.cursor-flap {
  pointer-events: none;
}

/* Legacy bracket/cursor styles preserved for any external CSS selector — now unused */
.freq-ruler__cursor-arrow {
  display: none;
}

.freq-ruler__cursor::after {
  display: none;
}

.freq-ruler__cursor-bracket {
  display: none;
}

.freq-ruler__bracket-body,
.freq-ruler__bracket-handle {
  display: none;
}
</style>
