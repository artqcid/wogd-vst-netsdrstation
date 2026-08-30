<template>
  <header class="kiwi-header" data-testid="header-bar">
    <div class="kiwi-header__grid">
      <!-- L: Logo + station title + subtitle + antenna -->
      <div class="kiwi-header__col-left">
        <svg class="kiwi-header__logo" viewBox="0 0 40 40" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
          <circle cx="20" cy="20" r="19" fill="#4CAF50" stroke="#2e7d32" stroke-width="1.5"/>
          <ellipse cx="21" cy="23" rx="11" ry="8" fill="#2e7d32"/>
          <circle cx="29" cy="17" r="5" fill="#2e7d32"/>
          <line x1="33" y1="15" x2="40" y2="14" stroke="#2e7d32" stroke-width="2.5" stroke-linecap="round"/>
          <circle cx="31" cy="16" r="1.5" fill="white"/>
        </svg>
        <div class="kiwi-header__title-stack">
          <div class="kiwi-header__title">NetSDRStation</div>
          <div class="kiwi-header__sub">{{ store.station || 'no station' }}</div>
          <div class="kiwi-header__sub">Antenna: KiwiSDR broadband</div>
        </div>
      </div>

      <!-- ML: Owner info (Provided by + links) -->
      <div class="kiwi-header__col-ml">
        <div class="kiwi-header__owner">
          <span class="kiwi-header__owner-label">Provided by:</span>
          <span class="kiwi-header__owner-name">{{ store.station ? store.station.split(':')[0] : '—' }}</span>
        </div>
        <div class="kiwi-header__owner">
          <span class="kiwi-header__owner-label">Location:</span>
          <span class="kiwi-header__owner-name">KiwiSDR receiver</span>
        </div>
      </div>

      <!-- MR: User ident (callsign) -->
      <div class="kiwi-header__col-mr">
        <span class="kiwi-header__label">Your name or callsign:</span>
        <input
          type="text"
          class="kiwi-header__callsign-input"
          v-model="callsign"
          placeholder="callsign"
          maxlength="12"
        />
      </div>

      <!-- R: Time + timezone + powered by -->
      <div class="kiwi-header__col-right">
        <div class="kiwi-header__time-stack">
          <span class="kiwi-header__time-utc">{{ utcStr }}</span>
          <span class="kiwi-header__time-local">{{ localStr }}</span>
          <span class="kiwi-header__timezone">{{ tzName }}</span>
        </div>
        <div class="kiwi-header__powered">
          <span class="kiwi-header__powered-text">Powered by</span>
          <span class="kiwi-header__powered-name">OpenWebRX</span>
        </div>
      </div>
    </div>

    <!-- Chevron toggle -->
    <button
      class="kiwi-header__chevron"
      @click="togglePhoto"
      :title="photoExpanded ? 'Collapse' : 'Expand'"
      :aria-expanded="photoExpanded"
      data-testid="header-chevron"
    >
      <svg width="43" height="12" viewBox="0 0 43 12" xmlns="http://www.w3.org/2000/svg">
        <polyline
          :points="photoExpanded ? '5,9 21.5,3 38,9' : '5,3 21.5,9 38,3'"
          fill="none" stroke="#909090" stroke-width="2" stroke-linecap="round"
        />
      </svg>
    </button>

    <!-- Expandable panorama photo section -->
    <div
      class="kiwi-header__photo-clip"
      :class="{ 'kiwi-header__photo-clip--expanded': photoExpanded }"
      data-testid="header-photo-clip"
    >
      <div class="kiwi-header__photo-spacer" :style="{ height: '67px' }"></div>
      <div class="kiwi-header__photo">
        <div class="kiwi-header__photo-placeholder">
          <span>Panorama image (RX_PHOTO_FILE)</span>
        </div>
      </div>
      <div class="kiwi-header__photo-title">Panorama</div>
      <div class="kiwi-header__photo-desc">Station antenna view</div>
    </div>
  </header>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { useKiwiStore } from '@/store/kiwiStore'

const store = useKiwiStore()
const callsign = ref('')
const photoExpanded = ref(false)
const utcStr = ref('')
const localStr = ref('')
const tzName = ref('')
let timeInterval: ReturnType<typeof setInterval> | null = null

function pad(n: number): string {
  return String(n).padStart(2, '0')
}

function updateTime() {
  const now = new Date()
  utcStr.value = `${pad(now.getUTCHours())}:${pad(now.getUTCMinutes())}:${pad(now.getUTCSeconds())} UTC`
  localStr.value = now.toLocaleTimeString()
  tzName.value = Intl.DateTimeFormat().resolvedOptions().timeZone
}

function togglePhoto() {
  photoExpanded.value = !photoExpanded.value
}

onMounted(() => {
  updateTime()
  timeInterval = setInterval(updateTime, 1000)
})

onBeforeUnmount(() => {
  if (timeInterval) clearInterval(timeInterval)
})
</script>

<style scoped>
.kiwi-header {
  background: #2a2a2a;
  color: #ddd;
  height: 67px;
  position: relative;
  font-family: 'Segoe UI', Arial, sans-serif;
  font-size: 11px;
  user-select: none;
  border-bottom: 1px solid #444;
}

.kiwi-header__grid {
  display: grid;
  grid-template-columns: auto 1fr auto auto;
  gap: 8px;
  align-items: center;
  height: 67px;
  padding: 0 8px;
}

/* L: Logo + title */
.kiwi-header__col-left {
  display: flex;
  align-items: center;
  gap: 6px;
  white-space: nowrap;
}

.kiwi-header__logo {
  width: 40px;
  height: 40px;
  flex-shrink: 0;
}

.kiwi-header__title-stack {
  display: flex;
  flex-direction: column;
  line-height: 1.2;
}

.kiwi-header__title {
  font-weight: 700;
  font-size: 13px;
  color: #eee;
}

.kiwi-header__sub {
  font-size: 10px;
  color: #999;
}

/* ML: Owner info */
.kiwi-header__col-ml {
  display: flex;
  flex-direction: column;
  gap: 2px;
  padding: 0 4px;
  white-space: nowrap;
  overflow: hidden;
}

.kiwi-header__owner {
  display: flex;
  gap: 4px;
  font-size: 10px;
}

.kiwi-header__owner-label {
  color: #888;
}

.kiwi-header__owner-name {
  color: #ccc;
}

/* MR: User callsign */
.kiwi-header__col-mr {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 2px;
  white-space: nowrap;
}

.kiwi-header__label {
  font-size: 9px;
  color: #888;
}

.kiwi-header__callsign-input {
  background: #3a3a3a;
  border: 1px solid #555;
  color: #ddd;
  font-size: 11px;
  padding: 2px 6px;
  width: 100px;
  border-radius: 3px;
  outline: none;
}

.kiwi-header__callsign-input:focus {
  border-color: #4CAF50;
}

/* R: Time stack + powered by */
.kiwi-header__col-right {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 2px;
  white-space: nowrap;
}

.kiwi-header__time-stack {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  line-height: 1.2;
}

.kiwi-header__time-utc,
.kiwi-header__time-local {
  font-size: 13px;
  font-weight: 700;
  color: #aaa;
}

.kiwi-header__timezone {
  font-size: 9px;
  color: #888;
}

.kiwi-header__powered {
  display: flex;
  gap: 4px;
  font-size: 8px;
  color: #777;
}

.kiwi-header__powered-name {
  font-weight: 600;
  color: #999;
}

/* Chevron toggle button */
.kiwi-header__chevron {
  position: absolute;
  right: 8px;
  bottom: -4px;
  background: none;
  border: none;
  cursor: pointer;
  padding: 2px;
  line-height: 0;
  z-index: 2;
}

.kiwi-header__chevron:hover polyline {
  stroke: #ccc;
}

/* Photo expandable section */
.kiwi-header__photo-clip {
  overflow: hidden;
  max-height: 0;
  transition: max-height 0.4s ease;
  background: #1a1a1a;
}

.kiwi-header__photo-clip--expanded {
  max-height: 300px;
}

.kiwi-header__photo-spacer {
  flex-shrink: 0;
}

.kiwi-header__photo {
  padding: 8px;
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100px;
}

.kiwi-header__photo-placeholder {
  background: #333;
  border: 1px dashed #555;
  border-radius: 4px;
  padding: 40px 60px;
  color: #888;
  font-size: 12px;
  text-align: center;
}

.kiwi-header__photo-title {
  font-size: 12px;
  font-weight: 600;
  color: #ccc;
  padding: 0 8px;
}

.kiwi-header__photo-desc {
  font-size: 10px;
  color: #888;
  padding: 0 8px 8px;
}
</style>