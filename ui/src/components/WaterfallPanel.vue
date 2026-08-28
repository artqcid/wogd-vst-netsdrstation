<template>
  <KPanel title="Waterfall" class="kiwi-panel" data-testid="waterfall-panel">
    <Waterfall
      :bins="store.waterfallBins"
      :color-map="store.colorMap"
      :cursor-khz="store.freqKhz"
      :centre-khz="store.freqKhz"
      :low-cut-hz="store.lowCut"
      :high-cut-hz="store.highCut"
    />

    <div class="waterfall-panel__controls">
      <div class="waterfall-panel__row">
        <span class="waterfall-panel__label">Zoom</span>
        <KButton label="−" @click="onZoom(-1)" />
        <KButton label="+" @click="onZoom(1)" />
        <KButton label="Max In" @click="store.setParam('wfZoom', 3)" />
        <KButton label="Max Out" @click="store.setParam('wfZoom', -1)" />
      </div>

      <div class="waterfall-panel__row">
        <KSlider :model-value="store.wfMaxDb" :min="-10" :max="0" :step="1" label="WF Max dB" @update:model-value="onParam('wfMaxDb', $event)" />
        <KSlider :model-value="store.wfMinDb" :min="-160" :max="-60" :step="1" label="WF Min dB" @update:model-value="onParam('wfMinDb', $event)" />
      </div>

      <div class="waterfall-panel__row">
        <KSelect :model-value="store.wfSpeed" :options="speedOptions" label="Speed" @update:model-value="onSpeed" />
        <KSelect :model-value="store.colorMap" :options="colorMapOptions" label="Color" @update:model-value="onColorMap" />
        <KSelect :model-value="store.displayMode" :options="displayModeOptions" label="Mode" @update:model-value="onDisplayMode" />
        <KToggle :model-value="store.wfComp" label="CIC" @update:model-value="onParamBool('wfComp', $event)" />
      </div>
    </div>
  </KPanel>
</template>

<script setup lang="ts">
import KPanel from '@/components/KPanel.vue'
import KButton from '@/components/KButton.vue'
import KSlider from '@/components/KSlider.vue'
import KSelect from '@/components/KSelect.vue'
import KToggle from '@/components/KToggle.vue'
import Waterfall from '@/components/Waterfall.vue'
import type { ColorMapName } from '@/components/waterfall/colorMap'
import { useKiwiStore } from '@/store/kiwiStore'
import type { ParamId } from '@/generated/bridge-validators'

const store = useKiwiStore()

const speedOptions = [
  { value: 0, label: 'Pause' },
  { value: 1, label: 'Slow' },
  { value: 2, label: 'Med' },
  { value: 3, label: 'Fast' },
]

const colorMapOptions = [
  { value: 'default', label: 'Default' },
  { value: 'rain', label: 'Rain' },
  { value: 'grayscale', label: 'Grayscale' },
]

const displayModeOptions = [
  { value: 'waterfall', label: 'WF' },
  { value: 'spectrum', label: 'Spec' },
  { value: 'both', label: 'Both' },
]

function onParam(id: ParamId, value: number) {
  store.setParam(id, value)
}

function onParamBool(id: ParamId, value: boolean) {
  store.setParam(id, value ? 1 : 0)
}

/** Zoom step buttons: -1 / +1 adjust the existing wfZoom value. */
function onZoom(delta: number) {
  const base = typeof store.wfZoom === 'number' ? store.wfZoom : 0
  store.setParam('wfZoom', base + delta)
}

function onSpeed(value: string | number) {
  store.setParam('wfSpeed', Number(value))
}

function onColorMap(value: string | number) {
  store.colorMap = String(value) as ColorMapName
}

function onDisplayMode(value: string | number) {
  const mode = String(value)
  store.displayMode = mode === 'spectrum' || mode === 'both' ? mode : 'waterfall'
}
</script>

<style scoped>
.waterfall-panel__controls {
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-top: 8px;
}

.waterfall-panel__row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
}

.waterfall-panel__label {
  font-size: var(--kiwi-font-sm, 11px);
  color: var(--kiwi-text, #ddd);
  min-width: 40px;
}
</style>