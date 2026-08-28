<template>
  <div class="waterfall" data-testid="waterfall">
    <canvas ref="canvasEl" class="waterfall__canvas" :width="width" :height="height" aria-label="Waterfall spectrum display"></canvas>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted } from 'vue'
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

const canvasEl = ref<HTMLCanvasElement | null>(null)

/**
 * Scrolls the existing image down by one line and draws the new spectrum
 * frame as a coloured horizontal line at the top (KiwiSDR waterfall).
 */
function pushFrame(bins: number[]) {
  const canvas = canvasEl.value
  if (!canvas || bins.length < 2) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  // Scroll existing content down one pixel row.
  ctx.drawImage(canvas, 0, 0, props.width, props.height - 1, 0, 1, props.width, props.height - 1)

  const img = ctx.createImageData(props.width, 1)
  const px = img.data
  const binCount = bins.length
  for (let x = 0; x < props.width; ++x) {
    // Map screen column to a bin index (bins span 0..Nyquist = half the
    // display window by default; KiwiSDR shows 0..+24 kHz).
    const binIndex = Math.min(binCount - 1, Math.floor((x / props.width) * binCount))
    const binValue = bins[binIndex] ?? -160
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
  const rel = props.cursorKhz - props.centreKhz
  const half = props.spanKhz / 2
  const x = ((rel + half) / props.spanKhz) * props.width
  if (x >= 0 && x <= props.width) {
    ctx.fillStyle = 'rgba(255,255,0,0.9)'
    ctx.fillRect(Math.round(x), 0, 2, props.height)
  }
  // Passband shading: semi-transparent green box over lowCut..highCut.
  const lo = ((props.lowCutHz / 1000 + half) / props.spanKhz) * props.width
  const hi = ((props.highCutHz / 1000 + half) / props.spanKhz) * props.width
  if (hi > lo) {
    ctx.fillStyle = 'rgba(0,255,0,0.08)'
    ctx.fillRect(Math.round(lo), 0, Math.round(hi - lo), props.height)
  }
}

watch(() => props.bins, frames => {
  if (frames && frames.length >= 2) pushFrame(frames)
})

onMounted(() => {
  // Initial dark fill.
  const canvas = canvasEl.value
  if (canvas) {
    const ctx = canvas.getContext('2d')
    if (ctx) {
      ctx.fillStyle = '#0a0a0a'
      ctx.fillRect(0, 0, props.width, props.height)
    }
  }
  if (props.bins.length >= 2) pushFrame(props.bins)
})
</script>

<style scoped>
.waterfall__canvas {
  display: block;
  width: 100%;
  height: auto;
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  background: #0a0a0a;
}
</style>