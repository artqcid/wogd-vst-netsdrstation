<template>
  <div ref="rootEl" class="waterfall" data-testid="waterfall" @wheel.prevent="onWheel">
    <canvas
      ref="canvasEl"
      class="waterfall__canvas"
      :width="canvasW"
      :height="canvasH"
      aria-label="Waterfall spectrum display"
    ></canvas>
    <div v-if="!hasBins" class="waterfall__no-signal">
      No signal — connect to a KiwiSDR station
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onBeforeUnmount, computed } from 'vue'
import { colorFor, type ColorMapName } from '@/components/waterfall/colorMap'

const props = withDefaults(
  defineProps<{
    bins?: number[]
    colorMap?: ColorMapName
    cursorKhz?: number
    lowCutHz?: number
    highCutHz?: number
    centreKhz?: number
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

const hasBins = computed(() => props.bins.length > 0)

const emit = defineEmits<{
  (e: 'zoom', delta: number, anchorFrac: number): void
  (e: 'pan', deltaKhz: number): void
}>()

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

function pushFrame(bins: number[]) {
  const canvas = canvasEl.value
  const w = canvasW.value
  const h = canvasH.value
  if (!canvas || bins.length < 2 || w < 2 || h < 2) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return
  ctx.drawImage(canvas, 0, 0, w, h - 1, 0, 1, w, h - 1)
  const img = ctx.createImageData(w, 1)
  const px = img.data
  const binCount = bins.length
  for (let x = 0; x < w; ++x) {
    const binPos = (x / w) * (binCount - 1)
    const b0 = Math.floor(binPos)
    const b1 = Math.min(binCount - 1, b0 + 1)
    const frac = binPos - b0
    const v0 = bins[b0] ?? -160
    const v1 = bins[b1] ?? -160
    const binValue = v0 * (1 - frac) + v1 * frac
    const [r, g, b] = colorFor(binValue, props.colorMap)
    const o = x * 4
    px[o] = r; px[o + 1] = g; px[o + 2] = b; px[o + 3] = 255
  }
  ctx.putImageData(img, 0, 0)
  drawOverlay(ctx)
}

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
  const lo = ((props.lowCutHz / 1000 + half) / props.spanKhz) * w
  const hi = ((props.highCutHz / 1000 + half) / props.spanKhz) * w
  if (hi > lo) {
    ctx.fillStyle = 'rgba(0,255,0,0.08)'
    ctx.fillRect(Math.round(lo), 0, Math.round(hi - lo), h)
  }
}

function onWheel(e: WheelEvent) {
  e.preventDefault()
  const el = rootEl.value
  if (!el) return
  const rect = el.getBoundingClientRect()
  const offsetX = e.clientX - rect.left
  const anchorFrac = Math.max(0, Math.min(1, offsetX / (rect.width || 1)))
  const delta = e.deltaY > 0 ? -1 : 1
  emit('zoom', delta, anchorFrac)
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
  position: relative;
  height: 100%;
  background: var(--kiwi-waterfall-bg, #1e5f7f);
  cursor: crosshair;
  overflow: hidden;
}
.waterfall__canvas { display: block; }
.waterfall__no-signal {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  color: rgba(255, 255, 255, 0.5);
  font-size: 14px;
  font-weight: bold;
  text-align: center;
  pointer-events: none;
  z-index: 5;
}
</style>
