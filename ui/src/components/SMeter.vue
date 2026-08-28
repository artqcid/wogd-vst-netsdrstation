<template>
  <div class="s-meter" data-testid="s-meter">
    <canvas ref="canvasEl" class="s-meter__canvas" :width="width" :height="height" aria-label="Signal level S-meter"></canvas>
    <span class="s-meter__readout">{{ dbm.toFixed(1) }} dBm</span>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted } from 'vue'

const props = withDefaults(
  defineProps<{
    dbm?: number
    width?: number
    height?: number
  }>(),
  { dbm: -140, width: 260, height: 26 }
)

const canvasEl = ref<HTMLCanvasElement | null>(null)

// S1..S9 (grey) then +20/+40/+60 dB (yellow/red) per KiwiSDR S-meter scale.
// 1 dBm per 4 px across the scale; -127..+20 dBm displayed.
const kMinDbm = -127
const kMaxDbm = 20
const kRange = kMaxDbm - kMinDbm

function pixelFor(dbm: number): number {
  const t = Math.min(1, Math.max(0, (dbm - kMinDbm) / kRange))
  return t * props.width
}

function draw() {
  const canvas = canvasEl.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  ctx.clearRect(0, 0, props.width, props.height)

  // Background track (dark).
  ctx.fillStyle = '#444'
  ctx.fillRect(0, 0, props.width, props.height)

  // Level bar: grey up to S9 (-73 dBm), yellow to +10, red beyond.
  const px = pixelFor(props.dbm)
  const s9Px = pixelFor(-73)
  const p10Px = pixelFor(10)
  if (px > 0) {
    ctx.fillStyle = '#4CAF50' // S1..S9 green
    ctx.fillRect(0, 0, Math.min(px, s9Px), props.height)
    if (px > s9Px) {
      ctx.fillStyle = '#f0c040' // +10..+20 yellow
      ctx.fillRect(s9Px, 0, Math.min(px, p10Px) - s9Px, props.height)
    }
    if (px > p10Px) {
      ctx.fillStyle = '#f44336' // beyond red
      ctx.fillRect(p10Px, 0, px - p10Px, props.height)
    }
  }

  // Scale ticks every 10 dBm.
  ctx.fillStyle = '#888'
  for (let db = kMinDbm; db <= kMaxDbm; db += 10) {
    const x = pixelFor(db)
    ctx.fillRect(x, 0, 1, props.height)
  }
}

function redraw() {
  requestAnimationFrame(draw)
}

watch(() => props.dbm, redraw)
onMounted(draw)
</script>

<style scoped>
.s-meter {
  display: flex;
  align-items: center;
  gap: 8px;
}

.s-meter__canvas {
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
}

.s-meter__readout {
  font-family: Consolas, 'Courier New', monospace;
  font-size: var(--kiwi-font-sm, 11px);
  color: var(--kiwi-text, #ddd);
  min-width: 60px;
  text-align: right;
}
</style>