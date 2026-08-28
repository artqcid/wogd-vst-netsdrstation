<template>
  <div class="container kiwi-layout">
    <!-- Header row (M4.1 layout): title + station/status -->
    <header class="kiwi-header">
      <div class="kiwi-title">
        <h1>NetSDRStation</h1>
        <p class="subtitle">KiwiSDR Receiver - Milestone M4</p>
      </div>
      <StationInput
        :station="store.station"
        :status="store.status"
        @connect="onStation"
        @disconnect="store.disconnect()"
      />
      <KStatusBadge class="kiwi-status-badge" :state="statusState" :label="statusText" />
    </header>

    <!-- Main controls row (M4.1/M4.2 layout): panels wrap at narrow widths -->
    <main class="kiwi-controls-row">
      <KPanel title="Receiver" class="kiwi-panel">
        <KSelect
          :model-value="store.mode"
          :options="modeOptions"
          label="Mode"
          @update:model-value="onModeChange"
        />
        <KNumberInput
          :model-value="store.freqKhz"
          :min="0.001"
          :max="30000"
          :step="0.001"
          unit="kHz"
          label="Frequency"
          @update:model-value="onParam('freqKhz', $event)"
        />
      </KPanel>

      <KPanel title="Passband" class="kiwi-panel">
        <KNumberInput
          :model-value="store.lowCut"
          :min="-8000"
          :max="0"
          :step="100"
          unit="Hz"
          label="Low Cut"
          @update:model-value="onParam('lowCut', $event)"
        />
        <KNumberInput
          :model-value="store.highCut"
          :min="0"
          :max="8000"
          :step="100"
          unit="Hz"
          label="High Cut"
          @update:model-value="onParam('highCut', $event)"
        />
      </KPanel>

      <KPanel title="Audio" class="kiwi-panel">
        <KToggle :model-value="store.agcOn" label="AGC" @update:model-value="onParamBool('agcOn', $event)" />
        <KSlider
          :model-value="store.volume"
          :min="0"
          :max="1"
          :step="0.01"
          label="Volume"
          @update:model-value="onParam('volume', $event)"
        />
        <KToggle :model-value="store.mute" label="Mute" @update:model-value="onParamBool('mute', $event)" />
      </KPanel>

      <KPanel title="Display" class="kiwi-panel">
        <KToggle :model-value="store.wfOn" label="Waterfall" @update:model-value="onParamBool('wfOn', $event)" />
      </KPanel>
    </main>

    <!-- Status bar row (M4.1 layout) -->
    <footer class="kiwi-statusbar">
      <KReadout :value="store.signalLevel" unit="dBm" :digits="1" />
      <span class="kiwi-statusbar__users">users: {{ store.userCount }}</span>
    </footer>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { storeToRefs } from 'pinia'
import StationInput from '@/components/StationInput.vue'
import KPanel from '@/components/KPanel.vue'
import KSelect from '@/components/KSelect.vue'
import KNumberInput from '@/components/KNumberInput.vue'
import KToggle from '@/components/KToggle.vue'
import KSlider from '@/components/KSlider.vue'
import KReadout from '@/components/KReadout.vue'
import KStatusBadge from '@/components/KStatusBadge.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'
import type { ParamId } from '@/generated/bridge-validators'

const store = useKiwiStore()
const { statusText, statusState } = storeToRefs(store)

// 18 KiwiSDR modes (index 0..17)
const modeOptions = [
  { value: 0, label: 'AM' },
  { value: 1, label: 'AMN' },
  { value: 2, label: 'AMW' },
  { value: 3, label: 'USB' },
  { value: 4, label: 'USN' },
  { value: 5, label: 'LSB' },
  { value: 6, label: 'LSN' },
  { value: 7, label: 'CW' },
  { value: 8, label: 'CWN' },
  { value: 9, label: 'NBFM' },
  { value: 10, label: 'NNFM' },
  { value: 11, label: 'IQ' },
  { value: 12, label: 'DRM' },
  { value: 13, label: 'SAM' },
  { value: 14, label: 'SAU' },
  { value: 15, label: 'SAL' },
  { value: 16, label: 'SAS' },
  { value: 17, label: 'QAM' },
]

function onParam(id: ParamId, value: number) {
  store.setParam(id, value)
}

function onParamBool(id: ParamId, value: boolean) {
  store.setParam(id, value ? 1 : 0)
}

function onModeChange(value: string | number) {
  store.setParam('mode', Number(value))
}

function onStation(hostPort: string) {
  store.setStation(hostPort)
  store.setStatus('Connecting...')
  if (!pluginService.isInNative()) {
    store.setStatus('Connected (dev)')
  }
}

onMounted(() => {
  pluginService.onMessage(message => {
    if (message.type === 'param') {
      store.applyParam(message.data.id, message.data.value)
    }
    if (message.type === 'status') {
      store.setStatus(message.data)
    }
  })
  pluginService.getParameters()
})
</script>

<style scoped>
/* M4.1/M4.2 layout: full editor surface, reflows at any size. */
.kiwi-layout {
  display: grid;
  grid-template-rows: auto 1fr auto;
  grid-template-columns: 1fr;
  height: 100%;
  min-height: 0;
  overflow: auto;
  background: var(--kiwi-bg, #222);
  color: var(--kiwi-text, #eee);
  font-family: 'Segoe UI', Arial, sans-serif;
  padding: 12px;
  gap: 10px;
}

.kiwi-header {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 0.5rem 1rem;
  padding-bottom: 8px;
  border-bottom: 1px solid var(--kiwi-border, #444);
}

.kiwi-title {
  flex: 1 1 200px;
  min-width: 0;
}

h1 {
  color: var(--kiwi-accent, #4CAF50);
  font-size: 20px;
  margin: 0;
}

.subtitle {
  color: #888;
  font-size: 12px;
  margin: 2px 0 0 0;
}

.kiwi-controls-row {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: flex-start;
  align-content: flex-start;
  min-height: 0;
}

.kiwi-panel {
  flex: 1 1 220px;
  min-width: 0;
}

.kiwi-statusbar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding-top: 8px;
  border-top: 1px solid var(--kiwi-border, #444);
  font-size: var(--kiwi-font-sm, 12px);
  color: #999;
}

.kiwi-status-badge {
  margin-left: auto;
}
</style>