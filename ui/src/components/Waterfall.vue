<template>
  <div ref="rootEl" class="waterfall" data-testid="waterfall">
    <canvas
      ref="canvasEl"
      class="waterfall__canvas"
      :width="canvasW"
      :height="canvasH"
      aria-label="Waterfall spectrum display"
    ></canvas>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onBeforeUnmount } from 'vue'
import { colorFor, type ColorMapName } from '@/components/waterfall/colorMap'

const props = withDefaults(
  defineProps<{
    bins?: number[]
    colorMap?: ColorMapName
    /** Frequency cursor position in kHz (from store.freqKhz). */
    cursorKhz?: number
    /** Passband edges in Hz relative to the centre frequency. */
    lowCutHz?: number
    highCutHz?: number
    /** Centre frequency of the display in kHz (from store.freqKhz). */
    centreKhz?: number
    /** Span shown on screen in kHz (defaults to 24 kHz window). */
    spanKhz?: number
    /** Fallback size (used in tests / before first layout). When the parent
        has a real size, the canvas fills the parent (ResizeObserver). */
    width?: number
    height?: number
  }>(),
  {
    bins: () => [],
    colorMap: 'default',
    cursorKhz: 0,
    lowCutHz: -4900,
    highCutHz: 4900,
    centreKhz: 14100,
    spanKhz: 24,
    width: 480,
    height: 160,
  }
)

const rootEl = ref<HTMLDivElement | null>(null)
const canvasEl = ref<HTMLCanvasElement | null>(null)
const canvasW = ref(props.width)
const canvasH = ref(props.height)

let observer: ResizeObserver | null = null

function measure() {
  const el = rootEl.value
  if (el && el.clientWidth > 0 && el.clientHeight > 0) {
    canvasW.value = el.clientWidth
    canvasH.value = el.clientHeight
  }
}

function ctx2d() {
  const canvas = canvasEl.value
  if (!canvas) return null
  return canvas.getContext('2d')
}

/**
 * Scrolls the existing image down by one line and draws the new spectrum
 * frame as a coloured horizontal line at the top (KiwiSDR waterfall).
 */
function pushFrame(bins: number[]) {
  const canvas = canvasEl.value
  const w = canvasW.value
  const h = canvasH.value
  if (!canvas || bins.length < 2 || w < 2 || h < 2) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  // Scroll existing content down one pixel row.
  ctx.drawImage(canvas, 0, 0, w, h - 1, 0, 1, w, h - 1)

  const img = ctx.createImageData(w, 1)
  const px = img.data
  const binCount = bins.length
  for (let x = 0; x < w; ++x) {
    // Map screen column to a continuous bin position and linearly interpolate
    // between adjacent bins, so the waterfall is dense and continuous (no gaps)
    // even when the window is wider than the bin count.
    const binPos = (x / w) * (binCount - 1)
    const b0 = Math.floor(binPos)
    const b1 = Math.min(binCount - 1, b0 + 1)
    const frac = binPos - b0
    const v0 = bins[b0] ?? -160
    const v1 = bins[b1] ?? -160
    const binValue = v0 * (1 - frac) + v1 * frac
    const [r, g, b] = colorFor(binValue, props.colorMap)
    const o = x * 4
    px[o] = r
    px[o + 1] = g
    px[o + 2] = b
    px[o + 3] = 255
  }
  ctx.putImageData(img, 0, 0)

  drawOverlay(ctx)
}

/** Frequency cursor + passband shading on the top row. */
function drawOverlay(ctx: CanvasRenderingContext2D) {
  const w = canvasW.value
  const h = canvasH.value
  const rel = props.cursorKhz - props.centreKhz
  const half = props.spanKhz / 2
  const x = ((rel + half) / props.spanKhz) * w
  if (x >= 0 && x <= w) {
    ctx.fillStyle = 'rgba(255,255,0,0.9)'
    ctx.fillRect(Math.round(x), 0, 2, h)
  }
  // Passband shading: semi-transparent green box over lowCut..highCut.
  const lo = ((props.lowCutHz / 1000 + half) / props.spanKhz) * w
  const hi = ((props.highCutHz / 1000 + half) / props.spanKhz) * w
  if (hi > lo) {
    ctx.fillStyle = 'rgba(0,255,0,0.08)'
    ctx.fillRect(Math.round(lo), 0, Math.round(hi - lo), h)
  }
}

watch(() => props.bins, frames => {
  if (frames && frames.length >= 2) pushFrame(frames)
})

onMounted(() => {
  measure()
  if (typeof ResizeObserver !== 'undefined') {
    observer = new ResizeObserver(measure)
    if (rootEl.value) observer.observe(rootEl.value)
  }

  const ctx = ctx2d()
  if (ctx) {
    ctx.fillStyle = 'var(--kiwi-waterfall-bg, #1e5f7f)'
    ctx.fillRect(0, 0, canvasW.value, canvasH.value)
  }
  if (props.bins.length >= 2) pushFrame(props.bins)
})

onBeforeUnmount(() => {
  observer?.disconnect()
})
</script>

<style scoped>
.waterfall {
  width: 100%;
  height: 100%;
  background: var(--kiwi-waterfall-bg, #1e5f7f);
  cursor: crosshair;
  overflow: hidden;
}

.waterfall__canvas {
  display: block;
}
</style>