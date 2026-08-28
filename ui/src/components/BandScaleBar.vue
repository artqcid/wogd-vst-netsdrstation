<template>
  <div class="band-scale" aria-label="Band frequency scale">
    <button class="band-scale__arrow" @click="$emit('pan', -1)" aria-label="Pan left">◄</button>
    <div class="band-scale__inner" ref="innerEl">
      <span
        v-for="band in allBands"
        :key="band.label + band.freq"
        class="band-scale__block"
        :style="{
          left: freqToPercent(band.freq) + '%',
          background: band.color,
          color: band.textColor ?? 'black',
        }"
        :title="`${band.label} (${band.freq} MHz)`"
        @click="$emit('tune', band.freq * 1000)"
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
  freq: number  // MHz centre frequency
  color: string
  textColor?: string
}

/** SW Broadcast bands (light blue) */
const BROADCAST_BANDS: BandDef[] = [
  { label: 'LW',  freq: 0.2,   color: '#FF9800' },
  { label: 'MW',  freq: 0.72,  color: '#FF9800' },
  { label: '49m', freq: 6.1,   color: '#4fc3f7' },
  { label: '41m', freq: 7.3,   color: '#4fc3f7' },
  { label: '31m', freq: 9.7,   color: '#4fc3f7' },
  { label: '25m', freq: 11.75, color: '#4fc3f7' },
  { label: '22m', freq: 13.75, color: '#4fc3f7' },
  { label: '19m', freq: 15.3,  color: '#4fc3f7' },
  { label: '16m', freq: 17.7,  color: '#4fc3f7' },
  { label: '13m', freq: 21.75, color: '#4fc3f7' },
  { label: '11m', freq: 25.8,  color: '#4fc3f7' },
]

/** Amateur bands (red) */
const AMATEUR_BANDS: BandDef[] = [
  { label: '160m', freq: 1.85,  color: '#ef5350', textColor: 'white' },
  { label: '80m',  freq: 3.7,   color: '#ef5350', textColor: 'white' },
  { label: '60m',  freq: 5.35,  color: '#ef5350', textColor: 'white' },
  { label: '40m',  freq: 7.1,   color: '#ef5350', textColor: 'white' },
  { label: '30m',  freq: 10.125,color: '#ef5350', textColor: 'white' },
  { label: '20m',  freq: 14.175,color: '#ef5350', textColor: 'white' },
  { label: '17m',  freq: 18.1,  color: '#ef5350', textColor: 'white' },
  { label: '15m',  freq: 21.2,  color: '#ef5350', textColor: 'white' },
  { label: '12m',  freq: 24.9,  color: '#ef5350', textColor: 'white' },
  { label: '10m',  freq: 28.5,  color: '#ef5350', textColor: 'white' },
]

const allBands = computed<BandDef[]>(() => [...BROADCAST_BANDS, ...AMATEUR_BANDS])

/** Convert MHz to a percentage position within the visible window */
function freqToPercent(freqMhz: number): number {
  const span = props.viewHighMhz - props.viewLowMhz
  if (span <= 0) return 0
  return ((freqMhz - props.viewLowMhz) / span) * 100
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
  transform: translateX(-50%);
  user-select: none;
}

.band-scale__block:hover { filter: brightness(1.15); }
</style>
