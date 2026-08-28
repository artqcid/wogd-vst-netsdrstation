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
    <!-- Cursor overlay (positioned absolutely over the scale) -->
    <div class="freq-ruler__cursor" v-if="zoomLevel < 9"
      :style="{ left: cursorPct + '%' }"
      @mousedown.prevent="onCursorMouseDown">
      <div class="freq-ruler__cursor-arrow"></div>
    </div>
    <div class="freq-ruler__cursor-bracket" v-else
      :style="{ left: loPct + '%', width: bwPct + '%' }"
      @mousedown.prevent="onCursorMouseDown">
      <div class="freq-ruler__bracket-handle freq-ruler__bracket-handle--left"
        @mousedown.stop="onLoMouseDown"></div>
      <div class="freq-ruler__bracket-body"></div>
      <div class="freq-ruler__bracket-handle freq-ruler__bracket-handle--right"
        @mousedown.stop="onHiMouseDown"></div>
    </div>
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
  else                    stepKhz = 10     // 10 kHz

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
  if (khz === 0) return '0 kHz'
  if (khz % 1000 === 0) return `${khz / 1000} MHz`
  if (khz >= 1000) return `${(khz / 1000).toFixed(1)} MHz`
  return `${khz} kHz`
}

// ---- Cursor overlay computed positions ----

const cursorPct = computed(() => {
  const span = props.viewHighKhz - props.viewLowKhz
  if (span <= 0) return 0
  return ((props.cursorKhz - props.viewLowKhz) / span) * 100
})

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
  dragState = { type: 'cursor', startX: e.clientX, startFreq: props.cursorKhz, startLo: 0, startHi: 0 }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onLoMouseDown(e: MouseEvent) {
  dragState = { type: 'lo', startX: e.clientX, startFreq: 0, startLo: props.lowCutHz, startHi: 0 }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onHiMouseDown(e: MouseEvent) {
  dragState = { type: 'hi', startX: e.clientX, startFreq: 0, startLo: 0, startHi: props.highCutHz }
  window.addEventListener('mousemove', onDragMove)
  window.addEventListener('mouseup', onDragUp)
}

function onDragMove(e: MouseEvent) {
  if (!dragState) return
  const span = props.viewHighKhz - props.viewLowKhz
  const el = scaleEl.value
  if (!el || span <= 0) return
  const rect = el.getBoundingClientRect()
  const deltaFrac = (e.clientX - dragState.startX) / rect.width
  const deltaKhz = deltaFrac * span

  if (dragState.type === 'cursor') {
    const newFreq = Math.max(props.viewLowKhz, Math.min(props.viewHighKhz, dragState.startFreq + deltaKhz))
    emit('tune', newFreq)
  } else if (dragState.type === 'lo') {
    const newLo = Math.min(props.highCutHz - 100, dragState.startLo + deltaKhz * 1000)
    emit('low-cut', newLo)
  } else if (dragState.type === 'hi') {
    const newHi = Math.max(props.lowCutHz + 100, dragState.startHi + deltaKhz * 1000)
    emit('high-cut', newHi)
  }
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

/* Cursor overlay */
.freq-ruler__cursor {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 0;
  z-index: 2;
  pointer-events: none;
  transform: translateX(-50%);
  cursor: ew-resize;
}

.freq-ruler__cursor > * {
  pointer-events: auto;
}

.freq-ruler__cursor-arrow {
  position: absolute;
  top: 0;
  left: 50%;
  transform: translateX(-50%);
  width: 0;
  height: 0;
  border-left: 6px solid transparent;
  border-right: 6px solid transparent;
  border-top: 8px solid #FFD700;
  filter: drop-shadow(0 0 2px rgba(255,215,0,0.5));
}

.freq-ruler__cursor::after {
  content: '';
  position: absolute;
  top: 8px;
  bottom: 0;
  left: 50%;
  width: 2px;
  background: #FFD700;
  transform: translateX(-50%);
  box-shadow: 0 0 3px rgba(255,215,0,0.5);
}

/* Bracket cursor (green, high zoom) */
.freq-ruler__cursor-bracket {
  position: absolute;
  top: 2px;
  height: 22px;
  z-index: 2;
  pointer-events: none;
  display: flex;
  align-items: stretch;
  cursor: ew-resize;
}

.freq-ruler__cursor-bracket > * {
  pointer-events: auto;
}

.freq-ruler__bracket-body {
  flex: 1;
  background: rgba(0, 255, 0, 0.08);
  border-top: 2px solid #00FF00;
  border-bottom: 2px solid #00FF00;
  position: relative;
}

.freq-ruler__bracket-handle {
  width: 4px;
  background: #00FF00;
  cursor: ew-resize;
  border-radius: 2px;
}

.freq-ruler__bracket-handle:hover {
  background: #66FF66;
  width: 6px;
}
</style>
