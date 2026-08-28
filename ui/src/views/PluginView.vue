<template>
  <div class="kiwi-root">
    <!-- Topbar (header): logo + RX info + station + status -->
    <header class="kiwi-topbar">
      <div class="kiwi-topbar__logo" aria-hidden="true">K</div>
      <div class="kiwi-topbar__info">
        <div class="kiwi-topbar__title">NetSDRStation</div>
        <div class="kiwi-topbar__desc">{{ store.station || 'no station selected' }}</div>
        <div class="kiwi-topbar__antenna">{{ statusText }}</div>
      </div>
      <div class="kiwi-topbar__right">
        <StationInput
          :station="store.station"
          :status="store.status"
          @connect="onStation"
          @disconnect="store.disconnect()"
        />
        <KStatusBadge :state="statusState" :label="statusText" />
      </div>
    </header>

    <!-- Tuning area: stacked mode buttons + band tags directly above the waterfall -->
    <section class="kiwi-tuning">
      <ModePanel />
      <BandPanel />
    </section>

    <!-- Main area: full-bleed waterfall + floating control panel on the right -->
    <main class="kiwi-main">
      <Waterfall
        :bins="store.waterfallBins"
        :color-map="store.colorMap"
        :cursor-khz="store.freqKhz"
        :centre-khz="store.freqKhz"
        :low-cut-hz="store.lowCut"
        :high-cut-hz="store.highCut"
      />

      <aside class="kiwi-control-panel">
        <FreqPanel />
        <AudioPanel />
        <WaterfallPanel />
        <ExtensionPanel />
      </aside>
    </main>

    <!-- Status bar -->
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
import Waterfall from '@/components/Waterfall.vue'
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
/* KiwiSDR / OpenWebRX layout: fixed design surface (1280x720, scaled by App.vue). */
.kiwi-root {
  display: flex;
  flex-direction: column;
  height: 100%;
  width: 100%;
  background: var(--kiwi-bg, #1e1e1e);
  color: var(--kiwi-text, #ddd);
  font-family: 'Segoe UI', Arial, sans-serif;
}

/* --- Topbar --- */
.kiwi-topbar {
  flex: 0 0 56px;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 0 16px;
  background: var(--kiwi-topbar-bg, #ececec);
  color: var(--kiwi-topbar-text, #909090);
}

.kiwi-topbar__logo {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: var(--kiwi-accent, #4CAF50);
  color: #fff;
  font-weight: bold;
  font-size: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.kiwi-topbar__info {
  display: flex;
  flex-direction: column;
  line-height: 1.15;
  min-width: 0;
}

.kiwi-topbar__title {
  font-size: 14px;
  font-weight: bold;
  color: #404040;
}

.kiwi-topbar__desc,
.kiwi-topbar__antenna {
  font-size: 11px;
  color: var(--kiwi-topbar-text, #909090);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.kiwi-topbar__right {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 10px;
}

/* --- Tuning area (mode buttons + band tags) --- */
.kiwi-tuning {
  flex: 0 0 auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 8px 12px;
  background: #2a2a2a;
  border-bottom: 1px solid var(--kiwi-border, #555);
}

/* --- Main: waterfall + floating control panel --- */
.kiwi-main {
  flex: 1 1 auto;
  position: relative;
  min-height: 0;
  display: flex;
}

.kiwi-main .waterfall {
  flex: 1 1 auto;
  min-width: 0;
}

/* Floating control panel on the right (over the waterfall). */
.kiwi-control-panel {
  position: absolute;
  right: 12px;
  top: 12px;
  bottom: 12px;
  width: 360px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 10px;
  background: var(--kiwi-panel, #575757);
  border-radius: var(--kiwi-panel-radius, 15px);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
}

/* --- Status bar --- */
.kiwi-statusbar {
  flex: 0 0 auto;
  padding: 6px 16px;
  background: #2a2a2a;
  border-top: 1px solid var(--kiwi-border, #555);
}
</style>