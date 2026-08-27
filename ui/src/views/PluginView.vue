<template>
  <div class="container kiwi-layout">
    <!-- Header row (M4.1 layout): title + station/status -->
    <header class="kiwi-header">
      <div class="kiwi-title">
        <h1>NetSDRStation</h1>
        <p class="subtitle">KiwiSDR Receiver - Milestone M4</p>
      </div>
      <div class="kiwi-panel station-panel">
        <StationInput :station="station" :status="status" @connect="onStation" @disconnect="onDisconnect" />
      </div>
      <StatusBadge class="kiwi-status-badge" :status="status" />
    </header>

    <!-- Main controls row (M4.1 layout): panels wrap at narrow widths -->
    <main class="kiwi-controls-row">
      <section class="kiwi-panel">
        <label class="section-label">Receiver</label>
        <select v-model.number="mode" @change="onModeChange" data-testid="mode-select">
          <option value="0">AM</option>
          <option value="1">AMN</option>
          <option value="2">AMW</option>
          <option value="3">USB</option>
          <option value="4">USN</option>
          <option value="5">LSB</option>
          <option value="6">LSN</option>
          <option value="7">CW</option>
          <option value="8">CWN</option>
          <option value="9">NBFM</option>
          <option value="10">NNFM</option>
          <option value="11">IQ</option>
          <option value="12">DRM</option>
          <option value="13">SAM</option>
          <option value="14">SAU</option>
          <option value="15">SAL</option>
          <option value="16">SAS</option>
          <option value="17">QAM</option>
        </select>
      </section>

      <section class="kiwi-panel">
        <label class="section-label">Frequency</label>
        <NumberInput
          label="Frequency"
          suffix="kHz"
          :min="0.001"
          :max="30000"
          :step="0.1"
          :value="freqKhz"
          @update:value="onFreqKhz"
        />
        <NumberInput
          label="Low Cut"
          suffix="Hz"
          :min="-8000"
          :max="0"
          :step="100"
          :value="lowCut"
          @update:value="onLowCut"
        />
        <NumberInput
          label="High Cut"
          suffix="Hz"
          :min="0"
          :max="8000"
          :step="100"
          :value="highCut"
          @update:value="onHighCut"
        />
      </section>

      <section class="kiwi-panel">
        <label class="section-label">Audio</label>
        <Toggle label="AGC" :active="agcOn" @update:active="onAgc" />
        <Slider label="Volume" :min="0" :max="1" :step="0.01" :value="volume" @update:value="onVolume" />
        <MuteButton :active="mute" @toggle="onMute" />
      </section>

      <section class="kiwi-panel">
        <label class="section-label">Display</label>
        <Toggle label="Waterfall" :active="wfOn" @update:active="onWf" />
      </section>
    </main>

    <!-- Status bar row (M4.1 layout) -->
    <footer class="kiwi-statusbar">
      <StatusBadge :status="status" />
    </footer>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import StationInput from '@/components/StationInput.vue'
import NumberInput from '@/components/NumberInput.vue'
import Toggle from '@/components/Toggle.vue'
import Slider from '@/components/Slider.vue'
import MuteButton from '@/components/MuteButton.vue'
import StatusBadge from '@/components/StatusBadge.vue'
import { pluginService, type PluginMessage } from '@/services/pluginService'

// State refs
const station = ref('kphsdr.com:8072')
const status = ref('Idle')
const mode = ref(0)
const freqKhz = ref(14100)
const lowCut = ref(-4900)
const highCut = ref(4900)
const agcOn = ref(true)
const volume = ref(1)
const mute = ref(false)
const wfOn = ref(true)

// --- Handlers ---

function onStation(hostPort: string) {
  pluginService.setStation(hostPort)
  status.value = 'Connecting...'
  // In dev mode just say Connected (dev)
  if (!pluginService.isInNative()) {
    status.value = 'Connected (dev)'
  }
}

function onDisconnect() {
  pluginService.disconnect()
  status.value = 'Disconnecting...'
}

function onFreqKhz(value: number) {
  freqKhz.value = value
  pluginService.setParameter('freqKhz', value)
}

function onLowCut(value: number) {
  lowCut.value = value
  pluginService.setParameter('lowCut', value)
}

function onHighCut(value: number) {
  highCut.value = value
  pluginService.setParameter('highCut', value)
}

function onModeChange(event: Event) {
  const target = event.target as HTMLSelectElement
  const value = Number(target.value)
  mode.value = value
  pluginService.setParameter('mode', value)
}

function onAgc(value: boolean) {
  agcOn.value = value
  pluginService.setParameter('agcOn', value ? 1 : 0)
}

function onVolume(value: number) {
  volume.value = value
  pluginService.setParameter('volume', value)
}

function onMute() {
  mute.value = !mute.value
  pluginService.setParameter('mute', mute.value ? 1 : 0)
}

function onWf(value: boolean) {
  wfOn.value = value
  pluginService.setParameter('wfOn', value ? 1 : 0)
}

onMounted(() => {
  pluginService.onMessage((message: PluginMessage) => {
    if (message.type === 'param' && typeof message.data === 'object' && message.data !== null) {
      const data = message.data as { id?: string; value?: number }
      if (data.id === 'freqKhz' && typeof data.value === 'number') freqKhz.value = data.value
      else if (data.id === 'volume' && typeof data.value === 'number') volume.value = data.value
      else if (data.id === 'mute' && typeof data.value === 'number') mute.value = data.value > 0.5
      else if (data.id === 'agcOn' && typeof data.value === 'number') agcOn.value = data.value > 0.5
      else if (data.id === 'mode' && typeof data.value === 'number') mode.value = data.value
      else if (data.id === 'lowCut' && typeof data.value === 'number') lowCut.value = data.value
      else if (data.id === 'highCut' && typeof data.value === 'number') highCut.value = data.value
      else if (data.id === 'wfOn' && typeof data.value === 'number') wfOn.value = data.value > 0.5
    }
    if (message.type === 'status' && typeof message.data === 'string') {
      status.value = message.data
    }
  })
  pluginService.getParameters()
})
</script>

<style scoped>
/* M4.1 Grundbedingung: the editor fills the WebView area entirely and the
   layout reflows at any size. Grid rows: header / controls / status bar.
   Below kMinSize the browser scrolls instead of clipping (overflow:auto). */
.kiwi-layout {
  display: grid;
  grid-template-rows: auto 1fr auto; /* header / main / status */
  grid-template-columns: 1fr;
  height: 100%;
  min-height: 0;
  overflow: auto;
  background: #222;
  color: #eee;
  font-family: 'Segoe UI', Arial, sans-serif;
  padding: 12px;
  gap: 10px;
}

/* Header row: title + station panel + status badge */
.kiwi-header {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 0.5rem 1rem;
  padding-bottom: 8px;
  border-bottom: 1px solid #444;
}

.kiwi-title {
  flex: 1 1 200px;
  min-width: 0;
}

h1 {
  color: #4CAF50;
  font-size: 20px;
  margin: 0;
}

.subtitle {
  color: #888;
  font-size: 12px;
  margin: 2px 0 0 0;
}

/* Main controls row: panels wrap at narrow widths */
.kiwi-controls-row {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: flex-start;
  align-content: flex-start;
  min-height: 0;
}

/* Each panel is a flex item that grows/shrinks with a ~220 px floor */
.kiwi-panel {
  flex: 1 1 220px;
  min-width: 0;
  background: #2a2a2a;
  border: 1px solid #444;
  border-radius: 6px;
  padding: 10px 12px;
}

.station-panel {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

/* Status bar row */
.kiwi-statusbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid #444;
  font-size: 12px;
  color: #999;
}

.kiwi-status-badge {
  margin-left: auto;
}

/* Section labels */
.section-label {
  color: #4CAF50;
  font-size: 12px;
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 8px;
  display: block;
}

/* -- Mode select --*/
select {
  width: 100%;
  padding: 8px;
  background: #333;
  color: #fff;
  border: 1px solid #555;
  border-radius: 4px;
  font-size: 13px;
}

/* -- NumberInput overrides --*/
.number-input-wrapper {
  margin-bottom: 12px;
}

/* -- Slider overrides --*/
.slider-wrapper {
  margin-bottom: 12px;
}
</style>