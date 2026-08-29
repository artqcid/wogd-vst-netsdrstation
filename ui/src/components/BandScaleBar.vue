<template>
  <div class="band-scale" aria-label="Band frequency scale">
    <button class="band-scale__arrow" @click="$emit('pan', -1)" aria-label="Pan left">◄</button>
    <div class="band-scale__inner" ref="innerEl">
      <span
        v-for="band in visibleBands"
        :key="band.label + band.startFreq"
        class="band-scale__block"
        :style="{
          left: freqToPercent(band.startFreq) + '%',
          width: freqWidthPercent(band.startFreq, band.endFreq) + '%',
          background: band.color,
          color: band.textColor ?? 'black',
        }"
        :title="`${band.label} (${band.startFreq}-${band.endFreq} MHz)`"
        @click="$emit('tune', (band.startFreq + band.endFreq) / 2 * 1000)"
      >{{ band.label }}</span>
    </div>
    <button class="band-scale__arrow" @click="$emit('pan', 1)" aria-label="Pan right">►</button>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

const props = withDefaults(defineProps<{
  /** Total spectrum span in MHz (default: 30 = full KiwiSDR range 0–30 MHz) */
  totalMhz?: number
  /** Low edge of visible window in MHz (for future zoom support) */
  viewLowMhz?: number
  /** High edge of visible window in MHz */
  viewHighMhz?: number
}>(), {
  totalMhz: 30,
  viewLowMhz: 0,
  viewHighMhz: 30,
})

defineEmits<{
  (e: 'pan', direction: -1 | 1): void
  (e: 'tune', freqKhz: number): void
}>()

const innerEl = ref<HTMLDivElement | null>(null)

interface BandDef {
  label: string
  startFreq: number  // MHz — band start
  endFreq: number    // MHz — band end
  color: string
  textColor?: string
}

/** SW Broadcast bands (light blue) */
const BROADCAST_BANDS: BandDef[] = [
  { label: 'LW',  startFreq: 0.1485, endFreq: 0.2835, color: '#FF9800' },
  { label: 'MW',  startFreq: 0.5265, endFreq: 1.6065, color: '#FF9800' },
  { label: '120m',startFreq: 2.3,    endFreq: 2.495,  color: '#4fc3f7' },
  { label: '90m', startFreq: 3.2,    endFreq: 3.4,    color: '#4fc3f7' },
  { label: '75m', startFreq: 3.9,    endFreq: 4.0,    color: '#4fc3f7' },
  { label: '60m', startFreq: 4.75,   endFreq: 5.06,   color: '#4fc3f7' },
  { label: '49m', startFreq: 5.9,    endFreq: 6.2,    color: '#4fc3f7' },
  { label: '41m', startFreq: 7.2,    endFreq: 7.45,   color: '#4fc3f7' },
  { label: '31m', startFreq: 9.4,    endFreq: 9.9,    color: '#4fc3f7' },
  { label: '25m', startFreq: 11.6,   endFreq: 12.1,   color: '#4fc3f7' },
  { label: '22m', startFreq: 13.57,  endFreq: 13.87,  color: '#4fc3f7' },
  { label: '19m', startFreq: 15.1,   endFreq: 15.8,   color: '#4fc3f7' },
  { label: '16m', startFreq: 17.48,  endFreq: 17.9,   color: '#4fc3f7' },
  { label: '15m', startFreq: 18.9,   endFreq: 19.02,  color: '#4fc3f7' },
  { label: '13m', startFreq: 21.45,  endFreq: 21.85,  color: '#4fc3f7' },
  { label: '11m', startFreq: 25.6,   endFreq: 26.1,   color: '#4fc3f7' },
]

/** Amateur bands (red) */
const AMATEUR_BANDS: BandDef[] = [
  { label: '160m', startFreq: 1.8,     endFreq: 2.0,    color: '#ef5350', textColor: 'white' },
  { label: '80m',  startFreq: 3.5,     endFreq: 3.8,    color: '#ef5350', textColor: 'white' },
  { label: '60m',  startFreq: 5.3515,  endFreq: 5.3665, color: '#ef5350', textColor: 'white' },
  { label: '40m',  startFreq: 7.0,     endFreq: 7.2,    color: '#ef5350', textColor: 'white' },
  { label: '30m',  startFreq: 10.1,    endFreq: 10.15,  color: '#ef5350', textColor: 'white' },
  { label: '20m',  startFreq: 14.0,    endFreq: 14.35,  color: '#ef5350', textColor: 'white' },
  { label: '17m',  startFreq: 18.068,  endFreq: 18.168, color: '#ef5350', textColor: 'white' },
  { label: '15m',  startFreq: 21.0,    endFreq: 21.45,  color: '#ef5350', textColor: 'white' },
  { label: '12m',  startFreq: 24.89,   endFreq: 24.99,  color: '#ef5350', textColor: 'white' },
  { label: '10m',  startFreq: 28.0,    endFreq: 29.7,   color: '#ef5350', textColor: 'white' },
]

const allBands = computed<BandDef[]>(() => [...BROADCAST_BANDS, ...AMATEUR_BANDS])

const visibleBands = computed(() =>
  allBands.value.filter(b => b.endFreq >= props.viewLowMhz && b.startFreq <= props.viewHighMhz)
)

/** Convert MHz to a percentage position within the visible window */
function freqToPercent(freqMhz: number): number {
  const span = props.viewHighMhz - props.viewLowMhz
  if (span <= 0) return 0
  return ((freqMhz - props.viewLowMhz) / span) * 100
}

function freqWidthPercent(startMhz: number, endMhz: number): number {
  const span = props.viewHighMhz - props.viewLowMhz
  if (span <= 0) return 0
  return Math.max(0.1, ((endMhz - startMhz) / span) * 100)  // min 0.1% damit Band sichtbar bleibt
}
</script>

<style scoped>
.band-scale {
  display: flex;
  align-items: center;
  height: 20px;
  background: white;
  flex-shrink: 0;
  border-bottom: 1px solid #ccc;
  overflow: hidden;
}

.band-scale__arrow {
  font-size: 10px;
  padding: 0 5px;
  cursor: pointer;
  color: #555;
  flex-shrink: 0;
  background: transparent;
  border: none;
  height: 100%;
  line-height: 20px;
  user-select: none;
}

.band-scale__arrow:hover { background: #eee; }

.band-scale__inner {
  flex: 1;
  position: relative;
  height: 100%;
  overflow: hidden;
}

.band-scale__block {
  position: absolute;
  height: 16px;
  top: 2px;
  border-radius: 3px;
  font-size: 8px;
  font-weight: bold;
  padding: 1px 3px;
  white-space: nowrap;
  cursor: pointer;
  border: 1px solid rgba(0,0,0,0.15);
  line-height: 14px;
  user-select: none;
}

.band-scale__block:hover { filter: brightness(1.15); }
</style>
