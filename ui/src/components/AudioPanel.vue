<template>
  <KPanel title="Audio" class="kiwi-panel" data-testid="audio-panel">
    <!-- Volume + mute -->
    <div class="audio-panel__row">
      <KSlider
        :model-value="store.volume"
        :min="0"
        :max="1"
        :step="0.01"
        label="Volume"
        @update:model-value="onParam('volume', $event)"
      />
      <KToggle :model-value="store.mute" label="Mute" @update:model-value="onParamBool('mute', $event)" />
    </div>

    <!-- AGC -->
    <div class="audio-panel__section">
      <span class="audio-panel__section-title">AGC</span>
      <KToggle :model-value="store.agcOn" label="On" @update:model-value="onParamBool('agcOn', $event)" />
      <KToggle :model-value="store.agcHang" label="Hang" @update:model-value="onParamBool('agcHang', $event)" />
      <KNumberInput :model-value="store.agcThresh" :min="-150" :max="0" :step="1" unit="dB" label="Thresh" @update:model-value="onParam('agcThresh', $event)" />
      <KNumberInput :model-value="store.agcDecay" :min="100" :max="5000" :step="100" unit="ms" label="Decay" @update:model-value="onParam('agcDecay', $event)" />
      <KNumberInput :model-value="store.agcSlope" :min="-20" :max="20" :step="1" unit="dB" label="Slope" @update:model-value="onParam('agcSlope', $event)" />
      <KNumberInput :model-value="store.agcManGain" :min="-50" :max="50" :step="1" unit="dB" label="ManGain" @update:model-value="onParam('agcManGain', $event)" />
    </div>

    <!-- Squelch -->
    <div class="audio-panel__section">
      <span class="audio-panel__section-title">Squelch</span>
      <KToggle :model-value="store.squelchOn" label="On" @update:model-value="onParamBool('squelchOn', $event)" />
      <KSlider :model-value="store.squelchThr" :min="0" :max="1" :step="0.01" label="Threshold" @update:model-value="onParam('squelchThr', $event)" />
    </div>

    <!-- Noise blanker / reduction -->
    <div class="audio-panel__section">
      <span class="audio-panel__section-title">Processing</span>
      <KToggle :model-value="store.nbOn" label="NB" @update:model-value="onParamBool('nbOn', $event)" />
      <KSlider :model-value="store.nbThresh" :min="0" :max="1" :step="0.01" label="NB Thresh" @update:model-value="onParam('nbThresh', $event)" />
      <KToggle :model-value="store.nrOn" label="NR" @update:model-value="onParamBool('nrOn', $event)" />
    </div>
  </KPanel>
</template>

<script setup lang="ts">
import KPanel from '@/components/KPanel.vue'
import KSlider from '@/components/KSlider.vue'
import KToggle from '@/components/KToggle.vue'
import KNumberInput from '@/components/KNumberInput.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import type { ParamId } from '@/generated/bridge-validators'

const store = useKiwiStore()

function onParam(id: ParamId, value: number) {
  store.setParam(id, value)
}

function onParamBool(id: ParamId, value: boolean) {
  store.setParam(id, value ? 1 : 0)
}
</script>

<style scoped>
.audio-panel__row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
}

.audio-panel__section {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  padding-top: 6px;
  border-top: 1px solid var(--kiwi-border, #444);
}

.audio-panel__section-title {
  font-size: var(--kiwi-font-sm, 11px);
  color: var(--kiwi-accent, #4CAF50);
  text-transform: uppercase;
  letter-spacing: 1px;
  min-width: 70px;
}
</style>