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
      <ModePanel />

      <FreqPanel />

      <BandPanel />

      <AudioPanel />

      <ExtensionPanel />

      <WaterfallPanel />
    </main>

    <!-- Status bar row (M4.1/M4.8 layout) -->
    <footer class="kiwi-statusbar">
      <StatusBar />
    </footer>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { storeToRefs } from 'pinia'
import StationInput from '@/components/StationInput.vue'
import ModePanel from '@/components/ModePanel.vue'
import FreqPanel from '@/components/FreqPanel.vue'
import BandPanel from '@/components/BandPanel.vue'
import AudioPanel from '@/components/AudioPanel.vue'
import ExtensionPanel from '@/components/ExtensionPanel.vue'
import WaterfallPanel from '@/components/WaterfallPanel.vue'
import StatusBar from '@/components/StatusBar.vue'
import KStatusBadge from '@/components/KStatusBadge.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

const store = useKiwiStore()
const { statusText, statusState } = storeToRefs(store)

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
  pluginService.onLevel(dbm => {
    store.setSignalLevel(dbm)
  })
  pluginService.onWaterfall(bins => {
    store.setWaterfallBins(bins)
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