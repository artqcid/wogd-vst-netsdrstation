<template>
  <KPanel title="Mode &amp; Passband" class="kiwi-panel" data-testid="mode-panel">
    <!-- 18 mode buttons in two rows; active = green -->
    <div class="mode-panel__modes">
      <KButton
        v-for="m in modes"
        :key="m.index"
        :label="m.label"
        :active="store.mode === m.index"
        @click="onMode(m.index)"
      />
    </div>

    <div class="mode-panel__passband">
      <KNumberInput
        :model-value="store.lowCut"
        :min="-8000"
        :max="0"
        :step="100"
        unit="Hz"
        label="Low"
        @update:model-value="onLowCut"
      />
      <KNumberInput
        :model-value="store.highCut"
        :min="0"
        :max="8000"
        :step="100"
        unit="Hz"
        label="High"
        @update:model-value="onHighCut"
      />
      <KReadout :value="bandwidth" unit="Hz" label="BW" />
      <KButton label="Reset" @click="resetDefaults" />
    </div>
  </KPanel>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import KPanel from '@/components/KPanel.vue'
import KButton from '@/components/KButton.vue'
import KNumberInput from '@/components/KNumberInput.vue'
import KReadout from '@/components/KReadout.vue'
import { useKiwiStore } from '@/store/kiwiStore'

const store = useKiwiStore()

interface ModeDef {
  index: number
  label: string
  lowCut: number
  highCut: number
}

/** KiwiSDR default passbands per mode (server rx_cmd.cpp / UI mode defaults). */
const MODE_DEFAULTS: ModeDef[] = [
  { index: 0, label: 'AM', lowCut: -4900, highCut: 4900 },
  { index: 1, label: 'AMN', lowCut: -2500, highCut: 2500 },
  { index: 2, label: 'AMW', lowCut: -5900, highCut: 5900 },
  { index: 3, label: 'USB', lowCut: 300, highCut: 2700 },
  { index: 4, label: 'USN', lowCut: 400, highCut: 1500 },
  { index: 5, label: 'LSB', lowCut: -2700, highCut: -300 },
  { index: 6, label: 'LSN', lowCut: -1500, highCut: -400 },
  { index: 7, label: 'CW', lowCut: 300, highCut: 800 },
  { index: 8, label: 'CWN', lowCut: 400, highCut: 600 },
  { index: 9, label: 'NBFM', lowCut: -6000, highCut: 6000 },
  { index: 10, label: 'NNFM', lowCut: -4000, highCut: 4000 },
  { index: 11, label: 'IQ', lowCut: -5000, highCut: 5000 },
  { index: 12, label: 'DRM', lowCut: -4800, highCut: 4800 },
  { index: 13, label: 'SAM', lowCut: -4900, highCut: 4900 },
  { index: 14, label: 'SAU', lowCut: -4900, highCut: 4900 },
  { index: 15, label: 'SAL', lowCut: -4900, highCut: 4900 },
  { index: 16, label: 'SAS', lowCut: -4900, highCut: 4900 },
  { index: 17, label: 'QAM', lowCut: -6000, highCut: 6000 },
]

const modes = MODE_DEFAULTS

/** Derived bandwidth = highCut - lowCut (read-only readout). */
const bandwidth = computed(() => store.highCut - store.lowCut)

/** Selects a mode and applies its default passband. */
function onMode(index: number) {
  store.setParam('mode', index)
  const def = MODE_DEFAULTS[index]
  if (def) {
    store.setParam('lowCut', def.lowCut)
    store.setParam('highCut', def.highCut)
  }
}

function onLowCut(value: number) {
  store.setParam('lowCut', Math.min(0, value))
}

function onHighCut(value: number) {
  store.setParam('highCut', Math.max(0, value))
}

/** Reset restores the current mode's defaults. */
function resetDefaults() {
  const def = MODE_DEFAULTS[store.mode] ?? MODE_DEFAULTS[0]
  if (!def) return
  store.setParam('lowCut', def.lowCut)
  store.setParam('highCut', def.highCut)
}
</script>

<style scoped>
.mode-panel__modes {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.mode-panel__passband {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
}
</style>