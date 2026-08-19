<template>
  <div class="container">
    <h1>NetSDRStation</h1>
    <p class="subtitle">Sine synthesizer - Milestone M1</p>

    <div class="controls">
      <Knob label="Frequency" :min="20" :max="20000" :step="1" :value="freq" @update:value="onFreq" />
      <Knob label="Volume" :min="0" :max="1" :step="0.01" :value="volume" @update:value="onVolume" />
      <MuteButton :active="mute" @toggle="onMute" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import Knob from '../components/Knob.vue'
import MuteButton from '../components/MuteButton.vue'
import { pluginService, type PluginMessage } from '../services/pluginService'

const freq = ref(440)
const volume = ref(1)
const mute = ref(false)

function onFreq(value: number) {
  freq.value = value
  pluginService.setParameter('freq', value)
}

function onVolume(value: number) {
  volume.value = value
  pluginService.setParameter('volume', value)
}

function onMute() {
  mute.value = !mute.value
  pluginService.setParameter('mute', mute.value ? 1 : 0)
}

onMounted(() => {
  pluginService.onMessage((message: PluginMessage) => {
    if (message.type === 'param' && typeof message.data === 'object' && message.data !== null) {
      const data = message.data as { id?: string; value?: number }
      if (data.id === 'freq' && typeof data.value === 'number') freq.value = data.value
      else if (data.id === 'volume' && typeof data.value === 'number') volume.value = data.value
      else if (data.id === 'mute' && typeof data.value === 'number') mute.value = data.value > 0.5
    }
  })
  pluginService.getParameters()
})
</script>

<style scoped>
.container {
  max-width: 640px;
  margin: 40px auto;
  padding: 20px;
  text-align: center;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: #e0e0e0;
}

h1 {
  color: #42b983;
  font-size: 28px;
  margin-bottom: 4px;
}

.subtitle {
  color: #888;
  font-size: 14px;
}

.controls {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  gap: 24px;
  margin-top: 24px;
}
</style>
