<template>
  <div
    class="passband-overlay"
    :style="{ cursor: isDragging ? 'grabbing' : 'grab' }"
    @mousedown.prevent="onMouseDown"
    aria-label="Frequency cursor — drag to tune"
  >
    <!-- Yellow center-frequency caret -->
    <div
      class="passband-overlay__caret"
      :style="{ left: cursorPct + '%' }"
    >▲</div>

    <!-- Passband shading -->
    <div
      class="passband-overlay__shade"
      :style="{
        left: passLowPct + '%',
        width: Math.max(0, passHighPct - passLowPct) + '%',
      }"
    ></div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

const props = withDefaults(defineProps<{
  /** Current centre frequency in kHz */
  centreKhz?: number
  /** Low edge of visible waterfall window in kHz */
  viewLowKhz?: number
  /** High edge of visible waterfall window in kHz */
  viewHighKhz?: number
  /** Passband low cut in Hz relative to centre */
  lowCutHz?: number
  /** Passband high cut in Hz relative to centre */
  highCutHz?: number
}>(), {
  centreKhz: 14100,
  viewLowKhz: 14088,
  viewHighKhz: 14112,
  lowCutHz: -4900,
  highCutHz: 4900,
})

const emit = defineEmits<{
  (e: 'tune', freqKhz: number): void
}>()

const isDragging = ref(false)
let dragStartX = 0
let dragStartFreq = 0

const spanKhz = computed(() => props.viewHighKhz - props.viewLowKhz)

function freqToPercent(freqKhz: number): number {
  if (spanKhz.value <= 0) return 50
  return ((freqKhz - props.viewLowKhz) / spanKhz.value) * 100
}

const cursorPct = computed(() => freqToPercent(props.centreKhz))

const passLowPct = computed(() =>
  freqToPercent(props.centreKhz + props.lowCutHz / 1000)
)
const passHighPct = computed(() =>
  freqToPercent(props.centreKhz + props.highCutHz / 1000)
)

function onMouseDown(e: MouseEvent) {
  isDragging.value = true
  dragStartX = e.clientX
  dragStartFreq = props.centreKhz

  const onMouseMove = (ev: MouseEvent) => {
    if (!isDragging.value) return
    const el = (e.target as HTMLElement).closest('.passband-overlay') as HTMLElement
    if (!el) return
    const width = el.clientWidth
    if (width <= 0) return
    const deltaX = ev.clientX - dragStartX
    const deltaKhz = (deltaX / width) * spanKhz.value
    const newFreq = Math.max(0.001, Math.min(30000, dragStartFreq + deltaKhz))
    emit('tune', newFreq)
  }

  const onMouseUp = () => {
    isDragging.value = false
    document.removeEventListener('mousemove', onMouseMove)
    document.removeEventListener('mouseup', onMouseUp)
  }

  document.addEventListener('mousemove', onMouseMove)
  document.addEventListener('mouseup', onMouseUp)
}
</script>

<style scoped>
.passband-overlay {
  position: absolute;
  inset: 0;
  pointer-events: auto;
  z-index: 10;
  overflow: hidden;
}

.passband-overlay__caret {
  position: absolute;
  top: 0;
  font-size: 14px;
  color: #FFD700;
  transform: translateX(-50%);
  line-height: 1;
  pointer-events: none;
  z-index: 2;
  text-shadow: 0 1px 2px rgba(0,0,0,0.8);
}

.passband-overlay__shade {
  position: absolute;
  top: 0;
  bottom: 0;
  background: rgba(255, 230, 0, 0.12);
  border-left: 1px solid rgba(255, 230, 0, 0.4);
  border-right: 1px solid rgba(255, 230, 0, 0.4);
  pointer-events: none;
  z-index: 1;
}
</style>
