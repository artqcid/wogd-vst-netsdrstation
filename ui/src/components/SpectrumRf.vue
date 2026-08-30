<template>
  <div class="spectrum-rf" ref="containerRef" data-testid="spectrum-rf">
    <canvas ref="canvasRef" class="spectrum-rf__canvas" />
    <canvas ref="pbCanvasRef" class="spectrum-rf__pb-canvas" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch, computed } from 'vue'
import { useKiwiStore } from '@/store/kiwiStore'

const props = defineProps<{
  bins: number[]
  passbandLowHz?: number
  passbandHighHz?: number
  spanKhz?: number
  centreKhz?: number
}>()

const store = useKiwiStore()
const containerRef = ref<HTMLElement | null>(null)
const canvasRef = ref<HTMLCanvasElement | null>(null)
const pbCanvasRef = ref<HTMLCanvasElement | null>(null)

const HEIGHT = 200
const DBTEXT_W = 30
const MAXDB = -10
const MINDB = -110
const FULL_SCALE = MAXDB - MINDB
const COLOR_SHIFT_DB = -12

// Default colormap: 256 RGB entries (KiwiSDR-compatible grayscale-ish)
// We use a simple grayscale-to-color mapping
const COLORMAP = createColormap()

interface DBBand {
  dB: number
  y1: number
  y2: number
  norm: number
  color: string
}

let dbBands: DBBand[] = []
let animationId = 0
let lastData: number[] = []

/** Safe colormap lookup — never returns undefined. */
function cmap(idx: number): string {
  return COLORMAP[Math.max(0, Math.min(255, idx))] ?? 'rgb(0,0,0)'
}

function createColormap(): string[] {
  // Create 256 colors: dark blue -> cyan -> green -> yellow -> red
  // Create 256 colors: dark blue -> cyan -> green -> yellow -> red
  const colors: string[] = []
  for (let i = 0; i < 256; i++) {
    const t = i / 255
    let r: number, g: number, b: number
    if (t < 0.25) {
      // dark blue -> blue
      const lt = t / 0.25
      r = 0; g = Math.round(lt * 40); b = Math.round(60 + lt * 100)
    } else if (t < 0.5) {
      // blue -> cyan
      const lt = (t - 0.25) / 0.25
      r = 0; g = Math.round(40 + lt * 120); b = Math.round(160 - lt * 60)
    } else if (t < 0.75) {
      // cyan -> green-yellow
      const lt = (t - 0.5) / 0.25
      r = Math.round(lt * 180); g = Math.round(160); b = Math.round(100 - lt * 80)
    } else {
      // green-yellow -> red
      const lt = (t - 0.75) / 0.25
      r = Math.round(180 + lt * 75); g = Math.round(160 - lt * 140); b = Math.round(20 - lt * 20)
    }
    colors.push(`rgb(${r},${g},${b})`)
  }
  return colors
}

function dBWireToDbm(dbValue: number): number {
  if (dbValue < 0) dbValue = 0
  if (dbValue > 255) dbValue = 255
  const rawDbm = -(255 - dbValue)
  return rawDbm + (-13) // wf.cal = -13
}

function colorIndex(dbValue: number): number {
  const dBm = dBWireToDbm(dbValue)
  const clamped = Math.max(MINDB, Math.min(MAXDB, dBm))
  const relative = clamped - MINDB
  const percent = relative / FULL_SCALE
  return Math.round(Math.max(0, Math.min(255, percent * 255)))
}

function computeDbBands() {
  dbBands = []
  const sFullScale = MAXDB - MINDB
  const barMax = MAXDB
  const barMin = MINDB + COLOR_SHIFT_DB
  const rng = barMax - barMin
  let lastNorm = 0

  for (let dB = Math.floor(MAXDB / 10) * 10; (MINDB - dB) < 10; dB -= 10) {
    const norm = 1 - ((dB - MINDB) / sFullScale)
    if (norm > 1) continue
    const cmi = Math.round((dB - barMin) / rng * 255)
    const ci = Math.max(0, Math.min(255, cmi))
    const yp1 = Math.round(lastNorm * HEIGHT)
    const yp2 = Math.round(norm * HEIGHT)
    if (yp1 === 0 && yp2 === 0) continue
    dbBands.push({
      dB,
      y1: yp1,
      y2: yp2,
      norm,
      color: cmap(ci),
    })
    lastNorm = norm
  }
}

function draw() {
  const canvas = canvasRef.value
  const pbCanvas = pbCanvasRef.value
  if (!canvas || !pbCanvas) return

  const ctx = canvas.getContext('2d')
  const pbCtx = pbCanvas.getContext('2d')
  if (!ctx || !pbCtx) return

  const parent = containerRef.value
  const w = parent?.clientWidth ?? canvas.width
  if (w <= 0) return

  // Resize canvases
  canvas.width = w
  canvas.height = HEIGHT
  pbCanvas.width = w
  pbCanvas.height = HEIGHT

  const sw1 = w - DBTEXT_W
  const sh = HEIGHT

  // Clear
  ctx.fillStyle = 'black'
  ctx.fillRect(0, 0, w, sh)

  // Draw dB band background
  const data = ctx.createImageData(1, sh)
  if (dbBands.length === 0) computeDbBands()

  for (let x = 0; x < sw1; x++) {
    // This column approach is optimized by drawing vertical strips instead
  }

  // Draw colormap background as vertical strips per dB band
  for (const band of dbBands) {
    ctx.fillStyle = band.color
    ctx.fillRect(0, band.y2, sw1, (band.y1 || HEIGHT) - band.y2)
  }

  // Draw grid lines at 10 dB boundaries
  ctx.fillStyle = 'rgba(200,200,200,0.4)'
  for (const band of dbBands) {
    const y = Math.round(band.norm * sh)
    ctx.fillRect(0, y, sw1, 1)
  }

  // Draw spectrum data
  const data_ = props.bins.length > 0 ? props.bins : lastData
  if (data_.length > 0) {
    lastData = data_
    // Draw each column
    for (let x = 0; x < sw1 && x < data_.length; x++) {
      const ci = colorIndex(data_[x] ?? 0)
      const y = Math.round((1 - ci / 255) * sh)
      const fillY = Math.max(0, y)
      ctx.fillStyle = cmap(ci)
      ctx.fillRect(x, fillY, 1, sh - fillY)
    }
  }

  // Draw dB labels on right side
  ctx.font = '10px sans-serif'
  ctx.textBaseline = 'middle'
  ctx.textAlign = 'left'
  for (const band of dbBands) {
    const y = Math.round(band.norm * sh)
    ctx.fillStyle = 'white'
    ctx.fillText(String(band.dB), sw1 + 3, y - 4)
  }

  // Draw passband overlay
  pbCtx.clearRect(0, 0, pbCanvas.width, pbCanvas.height)
  if (
    props.passbandLowHz !== undefined &&
    props.passbandHighHz !== undefined &&
    props.spanKhz !== undefined &&
    props.centreKhz !== undefined &&
    props.spanKhz > 0
  ) {
    const khzPerPx = (props.spanKhz * 1000) / sw1
    const centreHz = props.centreKhz * 1000
    const x1 = Math.max(0, Math.round((centreHz + props.passbandLowHz - (centreHz - (props.spanKhz * 1000) / 2)) / khzPerPx))
    const x2 = Math.min(sw1, Math.round((centreHz + props.passbandHighHz - (centreHz - (props.spanKhz * 1000) / 2)) / khzPerPx))
    if (x2 > x1) {
      pbCtx.fillStyle = 'rgba(150, 150, 150, 0.25)'
      pbCtx.fillRect(x1, 0, x2 - x1, sh)
    }
  }

  animationId = requestAnimationFrame(draw)
}

const spanHz = computed(() => {
  return (props.spanKhz ?? 30000) * 1000
})

onMounted(() => {
  computeDbBands()
  animationId = requestAnimationFrame(draw)
})

onBeforeUnmount(() => {
  if (animationId) cancelAnimationFrame(animationId)
})

watch(() => store.wfMaxDb, computeDbBands)
watch(() => store.wfMinDb, computeDbBands)
</script>

<style scoped>
.spectrum-rf {
  position: relative;
  width: 100%;
  height: 200px;
  background: black;
  cursor: default;
  flex-shrink: 0;
}

.spectrum-rf__canvas,
.spectrum-rf__pb-canvas {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 200px;
}

.spectrum-rf__pb-canvas {
  pointer-events: none;
}
</style>