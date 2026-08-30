<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { ENV_BL, ENV_ATT, ENV_H1, ENV_H2, ENV_SLOPE, ENV_ADJ, freqToPx } from '@/components/frequencyRulerLogic'

const props = defineProps<{
  viewLowKhz: number
  viewHighKhz: number
  cursorKhz: number
  lowCutHz: number
  highCutHz: number
}>()

const emit = defineEmits<{
  (e: 'tune', freqKhz: number): void
  (e: 'update:lowCut', hz: number): void
  (e: 'update:highCut', hz: number): void
}>()

const rulerWidthPx = ref(0)
const rootEl = ref<HTMLDivElement | null>(null)

const spanKhz = computed(() => props.viewHighKhz - props.viewLowKhz)

const drawFrom = computed(() => {
  return freqToPx(props.cursorKhz + props.lowCutHz / 1000, props.viewLowKhz, props.viewHighKhz, rulerWidthPx.value) - ENV_SLOPE
})

const drawTo = computed(() => {
  return freqToPx(props.cursorKhz + props.highCutHz / 1000, props.viewLowKhz, props.viewHighKhz, rulerWidthPx.value) + ENV_SLOPE
})

const allowResize = computed(() => (drawTo.value - drawFrom.value) >= 50)

const cursorColor = computed(() => (allowResize.value ? 'lime' : 'yellow'))

const trapezoidPoints = computed(() => {
  const w = rulerWidthPx.value
  if (w === 0) return '0,0 0,0 0,0 0,0'
  return [
    `${ENV_BL},${ENV_H2}`,
    `${w - ENV_BL},${ENV_H2}`,
    `${w - ENV_BL + ENV_ATT},${ENV_H1}`,
    `${ENV_BL - ENV_ATT},${ENV_H1}`,
  ].join(' ')
})

const carrierX = computed(() => {
  if (rulerWidthPx.value === 0) return 0
  return freqToPx(props.cursorKhz, props.viewLowKhz, props.viewHighKhz, rulerWidthPx.value)
})

function updateWidth() {
  if (rootEl.value) {
    rulerWidthPx.value = rootEl.value.clientWidth
  }
}

type DragZone = 'center' | 'lo' | 'hi'

interface DragState {
  zone: DragZone
  startX: number
  startFreqKhz: number
  startLoHz: number
  startHiHz: number
  freqPerPx: number
}

let dragState: DragState | null = null

function startDrag(e: PointerEvent, zone: DragZone, startFreqKhz: number, startLoHz: number, startHiHz: number) {
  const freqPerPx = spanKhz.value / rulerWidthPx.value
  dragState = { zone, startX: e.clientX, startFreqKhz, startLoHz, startHiHz, freqPerPx }
  document.addEventListener('pointermove', onPointerMove)
  document.addEventListener('pointerup', onPointerUp)
}

function onPointerMove(e: PointerEvent) {
  if (!dragState) return
  const dx = e.clientX - dragState.startX
  const { zone, startFreqKhz, startLoHz, startHiHz, freqPerPx } = dragState
  if (zone === 'center') {
    emit('tune', startFreqKhz + dx * freqPerPx)
  } else if (zone === 'lo') {
    const val = Math.min(0, startLoHz + dx * freqPerPx * 1000)
    emit('update:lowCut', val)
  } else if (zone === 'hi') {
    const val = Math.max(0, startHiHz + dx * freqPerPx * 1000)
    emit('update:highCut', val)
  }
}

function onPointerUp() {
  dragState = null
  document.removeEventListener('pointermove', onPointerMove)
  document.removeEventListener('pointerup', onPointerUp)
}

function classifyZone(x: number): DragZone {
  if (rulerWidthPx.value === 0) return 'center'
  const loStart = drawFrom.value + ENV_ADJ
  const loEnd = drawFrom.value + ENV_SLOPE + ENV_ADJ
  const hiStart = drawTo.value - ENV_SLOPE - ENV_ADJ
  const hiEnd = drawTo.value + ENV_ADJ
  if (allowResize.value && x >= loStart && x <= loEnd) return 'lo'
  if (allowResize.value && x >= hiStart && x <= hiEnd) return 'hi'
  return 'center'
}

function onRootPointerDown(e: PointerEvent) {
  const el = e.currentTarget as HTMLElement | null
  if (!el) return
  const rect = el.getBoundingClientRect()
  const x = e.clientX - rect.left
  const zone = classifyZone(x)
  if (zone === 'center') {
    startDrag(e, 'center', props.cursorKhz, props.lowCutHz, props.highCutHz)
  } else if (zone === 'lo') {
    const startFreqKhz = props.cursorKhz + props.lowCutHz / 1000
    startDrag(e, 'lo', startFreqKhz, props.lowCutHz, props.highCutHz)
  } else if (zone === 'hi') {
    const startFreqKhz = props.cursorKhz + props.highCutHz / 1000
    startDrag(e, 'hi', startFreqKhz, props.lowCutHz, props.highCutHz)
  }
}

onMounted(() => {
  updateWidth()
  window.addEventListener('resize', updateWidth)
})
onUnmounted(() => {
  window.removeEventListener('resize', updateWidth)
  if (dragState) {
    document.removeEventListener('pointermove', onPointerMove)
    document.removeEventListener('pointerup', onPointerUp)
  }
})
</script>

<template>
  <div
    ref="rootEl"
    class="cursor-bar"
    data-testid="cursor-bar"
    :data-cursor-color="cursorColor"
    @pointerdown.prevent="onRootPointerDown"
    @pointermove.prevent="updateWidth"
    @pointerup.prevent="updateWidth"
  >
    <svg
      class="cursor-bar__trapezoid"
      :viewBox="`0 0 ${rulerWidthPx} 20`"
      :points="trapezoidPoints"
      :fill="cursorColor"
      :stroke="cursorColor"
    >
      <polygon :points="trapezoidPoints" />
      <line
        :x1="carrierX"
        :y1="0"
        :x2="carrierX"
        :y2="20"
        data-testid="cursor-carrier"
        :stroke="cursorColor"
        stroke-width="2"
      />
    </svg>

    <div
      v-if="allowResize"
      class="cursor-bar__zone cursor-bar__zone--lo"
      data-testid="cursor-zone-lo"
      :style="{
        left: `${drawFrom + ENV_ADJ}px`,
        width: `${ENV_SLOPE}px`,
      }"
      @pointerdown.prevent.stop="(e: PointerEvent) => startDrag(e, 'lo', props.cursorKhz + props.lowCutHz / 1000, props.lowCutHz, props.highCutHz)"
    />

    <div
      v-if="allowResize"
      class="cursor-bar__zone cursor-bar__zone--hi"
      data-testid="cursor-zone-hi"
      :style="{
        left: `${drawTo - ENV_SLOPE - ENV_ADJ}px`,
        width: `${ENV_SLOPE}px`,
      }"
      @pointerdown.prevent.stop="(e: PointerEvent) => startDrag(e, 'hi', props.cursorKhz + props.highCutHz / 1000, props.lowCutHz, props.highCutHz)"
    />

    <div
      class="cursor-bar__zone cursor-bar__zone--center"
      data-testid="cursor-zone-center"
      :style="{
        left: allowResize ? `${drawFrom + ENV_SLOPE + ENV_ADJ}px` : `${ENV_BL - ENV_ATT}px`,
        width: allowResize
          ? `${drawTo - ENV_SLOPE - ENV_ADJ - (drawFrom + ENV_SLOPE + ENV_ADJ)}px`
          : `${rulerWidthPx - 2 * (ENV_BL - ENV_ATT)}px`,
      }"
      @pointerdown.prevent.stop="(e: PointerEvent) => startDrag(e, 'center', props.cursorKhz, props.lowCutHz, props.highCutHz)"
    />
  </div>
</template>

<style scoped>
.cursor-bar {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 20px;
  z-index: 10;
  pointer-events: all;
  cursor: default;
  overflow: visible;
}

.cursor-bar svg {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 20px;
  pointer-events: none;
  overflow: visible;
}

.cursor-bar__zone {
  position: absolute;
  top: 0;
  height: 20px;
  pointer-events: all;
}

.cursor-bar__zone--lo,
.cursor-bar__zone--hi {
  cursor: ew-resize;
}

.cursor-bar__zone--center {
  cursor: grab;
}

.cursor-bar__zone--center:active {
  cursor: grabbing;
}
</style>
