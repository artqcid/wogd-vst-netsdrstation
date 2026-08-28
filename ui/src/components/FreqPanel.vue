<template>
  <KPanel title="Frequency" class="kiwi-panel" data-testid="freq-panel">
    <!-- Step-tuning buttons -->
    <div class="freq-panel__steps">
      <KButton label="&larr;10" @click="stepBy(-10)" />
      <KButton label="&larr;1" @click="stepBy(-1)" />
      <KButton label="&larr;0.1" @click="stepBy(-0.1)" />
      <KButton label="+0.1" @click="stepBy(0.1)" />
      <KButton label="+1" @click="stepBy(1)" />
      <KButton label="+10" @click="stepBy(10)" />
    </div>

    <!-- Direct frequency entry (kHz) -->
    <KNumberInput
      :model-value="store.freqKhz"
      :min="kMinKhz"
      :max="kMaxKhz"
      :step="0.001"
      unit="kHz"
      label="Frequency"
      @update:model-value="onInput"
    />

    <!-- Large digital readout (KiwiSDR 7-digit format) -->
    <KReadout class="freq-panel__readout" :value="store.freqKhz" unit="kHz" :digits="3" />
  </KPanel>
</template>

<script setup lang="ts">
import KPanel from '@/components/KPanel.vue'
import KButton from '@/components/KButton.vue'
import KNumberInput from '@/components/KNumberInput.vue'
import KReadout from '@/components/KReadout.vue'
import { useKiwiStore } from '@/store/kiwiStore'

const store = useKiwiStore()

const kMinKhz = 0.001
const kMaxKhz = 30000

/** Clamps to the KiwiSDR frequency range and forwards to the bridge. */
function setFreq(value: number) {
  const clamped = Math.min(kMaxKhz, Math.max(kMinKhz, value))
  store.setParam('freqKhz', clamped)
}

/** Step buttons: delta in kHz, applied to the current store value. */
function stepBy(delta: number) {
  setFreq(store.freqKhz + delta)
}

/** Manual text entry (KNumberInput emits parsed numbers). */
function onInput(value: number) {
  setFreq(value)
}
</script>

<style scoped>
.freq-panel__steps {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.freq-panel__readout {
  font-size: 20px;
  min-width: 140px;
}
</style>