<template>
  <div class="freq-ruler" aria-label="Frequency ruler" @wheel.prevent="onWheel">
    <span class="freq-ruler__db-text">▲ database: stored</span>
    <div
      class="freq-ruler__scale"
      ref="scaleEl"
    ></div>

    <!--
      SVG frequency-scale ticks (Bug 8).
      Minor ticks: 8 px tall, short dashed look.
      Major ticks: 11 px tall, white, 2 px.
      Labels: 9 px white, anchored middle, 20 px baseline.
    -->
    <svg
      class="freq-ruler__scale-svg"
      :style="{
        position: 'absolute',
        top: 0, left: 0,
        width: '100%',
        height: '26px',
        pointerEvents: 'none',
      }"
      :viewBox="`0 0 ${rulerWidthPx} 26`"
      preserveAspectRatio="none"
    >
      <!-- Minor ticks -->
      <line
        v-for="(t, i) in minorTicks"
        :key="'min-' + i"
        :x1="t.x"
        y1="0"
        :x2="t.x"
        :y2="8"
        stroke="rgba(255,255,255,0.35)"
        stroke-width="1.5"
        class="freq-ruler__tick--minor"
      />
      <!-- Major ticks -->
      <line
        v-for="(t, i) in majorTicks"
        :key="'maj-' + i"
        :x1="t.x"
        y1="0"
        :x2="t.x"
        :y2="11"
        stroke="white"
        stroke-width="2"
        class="freq-ruler__tick--major"
      />
      <!-- Labels -->
      <text
        v-for="(t, i) in majorTicks"
        :key="'lbl-' + i"
        :x="t.x"
        y="20"
        fill="white"
        font-size="9"
        text-anchor="middle"
        class="freq-ruler__label"
      >{{ t.label }}</text>
    </svg>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onMounted } from 'vue'
import { freqToPx } from './frequencyRulerLogic'

const props = withDefaults(defineProps<{
  /** Low frequency of visible window in kHz */
  viewLowKhz?: number
  /** High frequency of visible window in kHz */
  viewHighKhz?: number
  /** 0..14 (store.wfZoom) — kept for backward compat, unused by cursor */
  zoomLevel?: number
}>(), {
  viewLowKhz: 0,
  viewHighKhz: 30000,
  zoomLevel: 0,
})

const emit = defineEmits<{
  (e: 'zoom', delta: number, anchorFrac: number): void
}>()

const scaleEl = ref<HTMLDivElement | null>(null)

// ---- Ruler pixel width (needed for the SVG viewBox) ----

/**
 * Pixel width of the scale (and the ruler SVG).  Read directly from the
 * DOM after mount; updated lazily on every pointer event via
 * `currentRulerWidthPx`.
 */
const rulerWidthPx = ref(0)

// ---- Tick marks (Bug 8: SVG ticks — major + minor) ----

const majorTicks = computed(() => {
  const low  = props.viewLowKhz
  const high = props.viewHighKhz
  const span = high - low
  if (span <= 0 || rulerWidthPx.value <= 0) return []

  let stepKhz: number
  if (span >= 20000)      stepKhz = 5000
  else if (span >= 10000) stepKhz = 2000
  else if (span >= 5000)  stepKhz = 1000
  else if (span >= 1000)  stepKhz = 200
  else if (span >= 200)   stepKhz = 50
  else if (span >= 50)    stepKhz = 10
  else if (span >= 10)    stepKhz = 2
  else if (span >= 2)     stepKhz = 0.5
  else                    stepKhz = 0.1

  const result: { x: number; label: string }[] = []
  const start = Math.ceil(low / stepKhz) * stepKhz
  for (let f = start; f <= high; f += stepKhz) {
    const x = ((f - low) / span) * rulerWidthPx.value
    result.push({ x, label: formatFreq(f) })
  }
  return result
})

const minorTicks = computed(() => {
  if (majorTicks.value.length < 2) return []
  const result: { x: number }[] = []
  for (let i = 0; i < majorTicks.value.length - 1; i++) {
    const a = majorTicks.value[i]
    const b = majorTicks.value[i + 1]
    if (!a || !b) continue
    const x1 = a.x
    const x2 = b.x
    const step = (x2 - x1) / 5
    // Add 4 minor ticks (the 5th point IS the next major tick)
    for (let j = 1; j <= 4; j++) {
      result.push({ x: x1 + step * j })
    }
  }
  return result
})

function formatFreq(khz: number): string {
  if (khz === 0) return '0'
  if (khz >= 1000) return `${(khz / 1000).toFixed(khz % 1000 === 0 ? 0 : 1)} MHz`
  if (khz >= 1)    return `${khz % 1 === 0 ? khz : khz.toFixed(1)} kHz`
  return `${Math.round(khz * 1000)} Hz`
}

// ---- Wheel zoom handler (unchanged) ----

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

// ---- Keep rulerWidthPx current ----

/**
 * Read the live pixel width of the scale element.  Called at mount and on
 * every pointer event before hit-testing / delta calculation.  Simpler than
 * a ResizeObserver and sufficient because deltas are computed from the
 * current width at event time.
 */
function refreshRulerWidth() {
  const el = scaleEl.value
  if (el) {
    rulerWidthPx.value = el.getBoundingClientRect().width
  }
}

onMounted(() => {
  refreshRulerWidth()
})
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
  user-select: none;
  -webkit-user-select: none;
}

.freq-ruler__db-text {
  font-size: 9px;
  color: #FFD700;
  padding: 2px 6px 0;
  flex-shrink: 0;
  white-space: nowrap;
  align-self: center;
  pointer-events: none;
}

.freq-ruler__scale {
  flex: 1;
  position: relative;
  overflow: hidden;
  z-index: 1;
  cursor: ew-resize;
}

.freq-ruler__tick {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 1px;
  background: rgba(255, 255, 255, 0.35);
  pointer-events: none;
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

/* Frequency-scale tick SVG (Bug 8) */
.freq-ruler__scale-svg {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 26px;
  z-index: 1;
  pointer-events: none;
}
</style>
