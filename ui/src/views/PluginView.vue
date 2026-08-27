<template>
  <div class="container">
    <h1>NetSDRStation</h1>
    <p class="subtitle">KiwiSDR Receiver - Milestone M3</p>

    <div class="station-section">
      <StationInput :station="station" :status="status" @connect="onStation" />
      <StatusBadge :status="status" />
    </div>

    <div class="receiver-section">
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
    </div>

    <div class="freq-section">
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
    </div>

    <div class="audio-section">
      <label class="section-label">Audio</label>
      <Toggle label="AGC" :active="agcOn" @update:active="onAgc" />
      <Slider label="Volume" :min="0" :max="1" :step="0.01" :value="volume" @update:value="onVolume" />
      <MuteButton :active="mute" @toggle="onMute" />
    </div>

    <div class="display-section">
      <label class="section-label">Display</label>
      <Toggle label="Waterfall" :active="wfOn" @update:active="onWf" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import StationInput from '@/components/StationInput.vue'
import NumberInput from '@/components/NumberInput.vue'
import Toggle from '@/components/Toggle.vue'
import Slider from '@/components/Slider.vue'
import MuteButton from '@/components/MuteButton.vue'
import { pluginService, type PluginMessage } from '@/services/pluginService'

// State refs
const station = ref('kphsdr.com:8073')
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
.container {
  background: #222;
  color: #eee;
  font-family: 'Segoe UI', Arial, sans-serif;
  padding: 20px;
  max-width: 640px;
  margin: 40px auto;
}

h1 {
  color: #4CAF50;
  font-size: 28px;
  margin-bottom: 4px;
}

.subtitle {
  color: #888;
  font-size: 14px;
  margin-bottom: 20px;
}

/* Section labels */
.section-label {
  color: #4CAF50;
  font-size: 13px;
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 10px;
  display: block;
}

/* Sections */
.station-section,
.receiver-section,
.freq-section,
.audio-section,
.display-section {
  margin-bottom: 20px;
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
  margin-bottom: 12px;
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