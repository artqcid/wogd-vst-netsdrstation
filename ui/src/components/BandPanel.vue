<template>
  <KPanel title="Bands &amp; Memory" class="kiwi-panel" data-testid="band-panel">
    <div class="band-panel__dropdowns">
      <KSelect :model-value="''" :options="amateurBands" label="Amateur" @update:model-value="onBand" />
      <KSelect :model-value="''" :options="broadcastBands" label="Broadcast" @update:model-value="onBand" />
      <KSelect :model-value="''" :options="utilityBands" label="Utility / Timesig" @update:model-value="onBand" />
    </div>

    <div class="band-panel__bookmarks">
      <header class="band-panel__bookmarks-header">
        <span class="band-panel__bookmarks-title">Bookmarks</span>
        <KButton label="Save current" @click="saveBookmark" />
      </header>
      <ul v-if="bookmarks.length" class="band-panel__list">
        <li v-for="(b, i) in bookmarks" :key="i" class="band-panel__item">
          <button type="button" class="band-panel__load" @click="loadBookmark(b)">
            <span class="band-panel__item-label">{{ b.label }}</span>
            <span class="band-panel__item-freq">{{ b.freqKhz.toFixed(3) }} kHz · {{ modeName(b.mode) }}</span>
          </button>
          <button type="button" class="band-panel__delete" aria-label="Delete bookmark" @click="deleteBookmark(i)">
            &times;
          </button>
        </li>
      </ul>
      <p v-else class="band-panel__empty">No bookmarks yet.</p>
    </div>
  </KPanel>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import KPanel from '@/components/KPanel.vue'
import KSelect from '@/components/KSelect.vue'
import KButton from '@/components/KButton.vue'
import { useKiwiStore } from '@/store/kiwiStore'

const store = useKiwiStore()

interface BandOption {
  value: string
  label: string
}

interface Bookmark {
  label: string
  freqKhz: number
  mode: number
}

// --- Band tables (key entries; editable via ui/public/bands.json alternative) ---

const amateurBands: BandOption[] = [
  { value: '1850', label: '160 m · 1850' },
  { value: '3700', label: '80 m · 3700' },
  { value: '7100', label: '40 m · 7100' },
  { value: '14200', label: '20 m · 14200' },
]

const broadcastBands: BandOption[] = [
  { value: '720', label: 'MW · 720' },
  { value: '6100', label: 'SW 49 m · 6100' },
  { value: '9700', label: 'SW 31 m · 9700' },
  { value: '15400', label: 'SW 19 m · 15400' },
]

const utilityBands: BandOption[] = [
  { value: '77.5', label: 'DCF77 · 77.5' },
  { value: '10000', label: 'WWV · 10000' },
  { value: '15000', label: 'WWVH · 15000' },
  { value: '7850', label: 'CHU · 7850' },
]

const MODE_NAMES = [
  'AM', 'AMN', 'AMW', 'USB', 'USN', 'LSB', 'LSN', 'CW', 'CWN', 'NBFM',
  'NNFM', 'IQ', 'DRM', 'SAM', 'SAU', 'SAL', 'SAS', 'QAM',
]

// --- Bookmarks (localStorage, no C++ change needed) ---

const STORAGE_KEY = 'netsdrstation.bookmarks.v1'
const bookmarks = ref<Bookmark[]>([])

onMounted(() => {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    if (raw) bookmarks.value = JSON.parse(raw) as Bookmark[]
  } catch {
    bookmarks.value = []
  }
})

function persist() {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(bookmarks.value))
  } catch {
    // storage unavailable (e.g. private mode) - keep in-memory only
  }
}

/** Band dropdown: emit the frequency for the selected band. */
function onBand(value: string | number) {
  const freq = Number(value)
  if (Number.isFinite(freq) && freq > 0) {
    store.setParam('freqKhz', freq)
  }
}

function saveBookmark() {
  const label = `${store.freqKhz.toFixed(3)} kHz`
  bookmarks.value.push({ label, freqKhz: store.freqKhz, mode: store.mode })
  persist()
}

function loadBookmark(b: Bookmark) {
  store.setParam('freqKhz', b.freqKhz)
  store.setParam('mode', b.mode)
}

function deleteBookmark(index: number) {
  bookmarks.value.splice(index, 1)
  persist()
}

function modeName(mode: number): string {
  return MODE_NAMES[mode] ?? String(mode)
}
</script>

<style scoped>
.band-panel__dropdowns {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.band-panel__bookmarks {
  margin-top: 10px;
}

.band-panel__bookmarks-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  margin-bottom: 6px;
}

.band-panel__bookmarks-title {
  font-size: var(--kiwi-font-sm, 11px);
  color: var(--kiwi-accent, #4CAF50);
  text-transform: uppercase;
  letter-spacing: 1px;
}

.band-panel__list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.band-panel__item {
  display: flex;
  align-items: center;
  gap: 4px;
  background: var(--kiwi-input-bg, #444);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  padding: 2px 4px 2px 8px;
}

.band-panel__load {
  flex: 1;
  display: flex;
  justify-content: space-between;
  gap: 8px;
  background: none;
  border: none;
  color: var(--kiwi-text, #ddd);
  font-size: var(--kiwi-font-sm, 11px);
  font-family: inherit;
  cursor: pointer;
  text-align: left;
  padding: 2px 0;
}

.band-panel__item-freq {
  color: #999;
}

.band-panel__delete {
  background: none;
  border: none;
  color: var(--kiwi-warn, #f44336);
  font-size: 14px;
  cursor: pointer;
  padding: 0 4px;
}

.band-panel__empty {
  color: #888;
  font-size: var(--kiwi-font-sm, 11px);
  margin: 0;
}
</style>