<template>
  <div class="status-bar" data-testid="status-bar">
    <SMeter :dbm="store.signalLevel" />

    <span class="status-bar__item" data-testid="status-users">users: {{ store.userCount }}</span>

    <span class="status-bar__item" :class="{ 'status-bar__gps--ok': store.gpsSync }" data-testid="status-gps">
      GPS: {{ store.gpsSync ? '✓' : '—' }}
    </span>

    <span class="status-bar__item" :class="bufferClass" data-testid="status-buffer">
      Buffer: {{ bufferLabel }}
    </span>

    <span class="status-bar__item status-bar__freq" data-testid="status-freq">
      {{ formattedFreq }} kHz
    </span>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import SMeter from '@/components/SMeter.vue'
import { useKiwiStore } from '@/store/kiwiStore'

const store = useKiwiStore()

/** Exact frequency with 3 decimals (KiwiSDR 7-digit format). */
const formattedFreq = computed(() => store.freqKhz.toFixed(3))

/**
 * Buffer health: currently derived from connection state (a live RMS level
 * implies the stream is flowing). A real jitter-buffer fill-level push from
 * C++ (2 Hz) is DEFERRED — it needs KiwiSDR MSG users/gps parsing in the
 * network layer first (see checklist M4.8 note).
 */
const bufferLabel = computed(() => {
  if (!store.connected) return '—'
  return store.signalLevel > -100 ? 'OK' : 'idle'
})

const bufferClass = computed(() => ({
  'status-bar__buffer--ok': bufferLabel.value === 'OK',
}))
</script>

<style scoped>
.status-bar {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  font-size: var(--kiwi-font-sm, 12px);
  color: #999;
}

.status-bar__item {
  white-space: nowrap;
}

.status-bar__gps--ok {
  color: var(--kiwi-accent, #4CAF50);
}

.status-bar__buffer--ok {
  color: var(--kiwi-accent, #4CAF50);
}

.status-bar__freq {
  margin-left: auto;
  font-family: Consolas, 'Courier New', monospace;
  color: var(--kiwi-text, #ddd);
}
</style>