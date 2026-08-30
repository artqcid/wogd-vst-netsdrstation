<template>
  <div class="spectrum-af" ref="containerRef" data-testid="spectrum-af">
    <canvas ref="canvasRef" class="spectrum-af__canvas" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch } from 'vue'
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

const HEIGHT = 200
const MARGIN_LEFT = 50
const MARGIN_RIGHT = 50
const MARGINS = MARGIN_LEFT + MARGIN_RIGHT
const AF_LEFT = MARGIN_LEFT
const DBTEXT_W = 25
const MAXDB = -10
const MINDB = -110
const FULL_SCALE = MAXDB - MINDB
const COLOR_SHIFT_DB = -12
const AUDIO_SAMPLE_RATE = 48000 // ext_nom_sample_rate() default

// Default colormap: 256 RGB entries (same as SpectrumRf)
const COLORMAP: string[] = []
function initColormap() {
  for (let i = 0; i < 256; i++) {
    const t = i / 255
    let r: number, g: number, b: number
    if (t < 0.25) {
      const lt = t / 0.25
      r = 0; g = Math.round(lt * 40); b = Math.round(60 + lt * 100)
    } else if (t < 0.5) {
      const lt = (t - 0.5) / 0.25
      r = 0; g = Math.round(40 + lt * 120); b = Math.round(160 - lt * 60)
    } else if (t < 0.75) {
      const lt = (t - 0.75) / 0.25
      r = Math.round(lt * 180); g = Math.round(160); b = Math.round(100 - lt * 80)
    } else {
      const lt = (t - 0.75) / 0.25
      r = Math.round(180 + lt * 75); g = Math.round(160 - lt * 140); b = Math.round(20 - lt * 20)
    }
    COLORMAP.push(`rgb(${r},${g},${b})`)
  }
}
initColormap()

interface DBBand { dB: number; y1: number; y2: number; norm: number; color: string }
let dbBands: DBBand[] = []
let animationId = 0
let lastData: number[] = []

function cmap(idx: number): string {
  return COLORMAP[Math.max(0, Math.min(255, idx))] ?? 'rgb(0,0,0)'
}

function dBWireToDbm(dbValue: number): number {
  if (dbValue < 0) dbValue = 0
  if (dbValue > 255) dbValue = 255
  return -(255 - dbValue) + (-13)
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
    const ci = Math.max(0, Math.min(255, Math.round((dB - barMin) / rng * 255)))
    const yp1 = Math.round(lastNorm * HEIGHT)
    const yp2 = Math.round(norm * HEIGHT)
    if (yp1 === 0 && yp2 === 0) continue
    dbBands.push({ dB, y1: yp1, y2: yp2, norm, color: cmap(ci) })
    lastNorm = norm
  }
}

function draw() {
  const canvas = canvasRef.value
  if (!canvas) { animationId = requestAnimationFrame(draw); return }

  const ctx = canvas.getContext('2d')
  if (!ctx) { animationId = requestAnimationFrame(draw); return }

  const parent = containerRef.value
  const w = parent?.clientWidth ?? 800
  if (w <= 0) { animationId = requestAnimationFrame(draw); return }

  canvas.width = w - MARGINS
  canvas.height = HEIGHT
  canvas.style.left = `${AF_LEFT}px`

  const sw = canvas.width - DBTEXT_W
  const sh = HEIGHT

  // Clear
  ctx.fillStyle = 'black'
  ctx.fillRect(0, 0, canvas.width, sh)

  // Draw colormap background per dB band
  for (const band of dbBands) {
    ctx.fillStyle = band.color
    ctx.fillRect(0, band.y2, sw, (band.y1 || HEIGHT) - band.y2)
  }

  // Horizontal grid lines at 10 dB boundaries
  ctx.fillStyle = 'rgba(200,200,200,0.4)'
  for (const band of dbBands) {
    ctx.fillRect(0, Math.round(band.norm * sh), sw, 1)
  }

  // Draw AF spectrum data
  const data_ = props.bins.length > 0 ? props.bins : lastData
  if (data_.length > 0) {
    lastData = data_
    const dataLen = Math.min(sw, 512, data_.length)
    for (let x = 0; x < dataLen; x++) {
      const ci = colorIndex(data_[x] ?? 0)
      const y = Math.round((1 - ci / 255) * sh)
      if (y < sh) {
        ctx.fillStyle = cmap(ci)
        ctx.fillRect(x, y, 1, sh - y)
      }
    }
  }

  // dB labels on right side
  ctx.font = '10px sans-serif'
  ctx.textBaseline = 'middle'
  ctx.textAlign = 'left'
  for (const band of dbBands) {
    ctx.fillStyle = 'white'
    ctx.fillText(String(band.dB), sw + 3, Math.round(band.norm * sh) - 4)
  }

  // --- AF-specific markers ---
  // Vertical 1kHz grid lines
  const sr = AUDIO_SAMPLE_RATE
  const frac = sr % 1000
  const sp = (sr - frac) - 1000
  ctx.fillStyle = 'rgba(200,200,200,0.3)'
  for (let i = 1000 + frac / 2; i <= sp + frac / 2; i += 1000) {
    const x = Math.round(canvas.width * i / sr)
    ctx.fillRect(x, 0, 1, sh)
  }

  // Green center line (DC / center of audio band)
  ctx.fillStyle = 'lime'
  ctx.fillRect(Math.round(canvas.width / 2) - 1, 0, 3, sh)

  // Red edge markers (Nyquist edges)
  ctx.fillStyle = 'red'
  ctx.fillRect(0, 0, 3, sh)
  ctx.fillRect(canvas.width - 3, 0, 3, sh)

  // --- Passband markers (if passbandLowHz/passbandHighHz provided) ---
  if (props.passbandLowHz !== undefined && props.passbandHighHz !== undefined) {
    const centreHz = (props.centreKhz ?? 0) * 1000
    const halfSpanHz = AUDIO_SAMPLE_RATE / 2
    const pxPerHz = canvas.width / (2 * halfSpanHz)
    // X relative to center
    const x1 = Math.round(canvas.width / 2 + props.passbandLowHz * pxPerHz)
    const x2 = Math.round(canvas.width / 2 + props.passbandHighHz * pxPerHz)
    // Draw semi-transparent region
    ctx.fillStyle = 'rgba(150, 150, 150, 0.25)'
    ctx.fillRect(x1, 0, x2 - x1, sh)
  }

  animationId = requestAnimationFrame(draw)
}

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
.spectrum-af {
  position: relative;
  width: 100%;
  height: 200px;
  background: black;
  cursor: default;
  flex-shrink: 0;
}

.spectrum-af__canvas {
  position: absolute;
  top: 0;
  height: 200px;
}
</style>