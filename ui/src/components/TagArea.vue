<template>
  <div class="tag-area" aria-label="DX frequency labels">
    <span
      v-for="tag in visibleTags"
      :key="tag.label + tag.freqKhz"
      class="tag-area__tag"
      :style="{
        left: freqToPercent(tag.freqKhz) + '%',
        background: tag.bg,
        color: tag.fg ?? 'black',
      }"
      :title="`${tag.label} — ${tag.freqKhz} kHz`"
      @click.stop="onTagClick(tag, $event)"
    >{{ tag.label }}</span>

    <TagPopup
      :visible="popupVisible"
      :tag="popupTag"
      :position="popupPos"
      @close="popupVisible = false"
      @tune="(f) => $emit('tune', f)"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import TagPopup from '@/components/TagPopup.vue'
import type { PopupTag } from '@/components/TagPopup.vue'

const props = withDefaults(defineProps<{
  /** Low edge of visible window in kHz */
  viewLowKhz?: number
  /** High edge of visible window in kHz */  
  viewHighKhz?: number
  /** Custom tags (optional; falls back to built-in demo set) */
  tags?: PopupTag[]
}>(), {
  viewLowKhz: 0,
  viewHighKhz: 30000,
  tags: () => [],
})

defineEmits<{ (e: 'tune', freqKhz: number): void }>()

/** Built-in KiwiSDR-style DX demo labels with EiBi-style station info */
const DEMO_TAGS: PopupTag[] = [
  // --- Technical / Utility signals ---
  { label: 'NAVTEX', freqKhz: 518,    bg: '#4CAF50', description: 'Navigational telex 518 kHz' },
  { label: 'FT8',    freqKhz: 3573,   bg: '#4CAF50', description: 'FT8 QRP, 80m band' },
  { label: 'FT8',    freqKhz: 7074,   bg: '#4CAF50', description: 'FT8 QRP, 40m band' },
  { label: 'FT8',    freqKhz: 10136,  bg: '#4CAF50', description: 'FT8 QRP, 30m band' },
  { label: 'FT8',    freqKhz: 14074,  bg: '#4CAF50', description: 'FT8 QRP, 20m band' },
  { label: 'FT8',    freqKhz: 21074,  bg: '#4CAF50', description: 'FT8 QRP, 15m band' },
  { label: 'FT8',    freqKhz: 28074,  bg: '#4CAF50', description: 'FT8 QRP, 10m band' },
  { label: 'FAX',    freqKhz: 7880,   bg: 'yellow',  description: 'Weather fax, 40m' },
  { label: 'FAX',    freqKhz: 13882,  bg: 'yellow',  description: 'Weather fax, 22m' },
  { label: 'SSTV',   freqKhz: 14230,  bg: '#f06292', description: 'Slow-scan TV, 20m band' },
  { label: 'WWV',    freqKhz: 10000,  bg: 'orange',  description: 'NIST time signal 10 MHz' },
  { label: 'WWV',    freqKhz: 15000,  bg: 'orange',  description: 'NIST time signal 15 MHz' },
  { label: 'CHU',    freqKhz: 7850,   bg: 'orange',  description: 'CHU time signal, 40m' },
  { label: 'RTTY',   freqKhz: 14090,  bg: '#f06292', description: 'RTTY, 20m band' },
  { label: 'CW',     freqKhz: 7025,   bg: '#ef5350', fg: 'white', description: 'CW beacon, 40m' },
  { label: 'WSPR',   freqKhz: 14095,  bg: '#4CAF50', description: 'WSPR beacon, 20m' },
  { label: 'JS8',    freqKhz: 14078,  bg: '#4CAF50', description: 'JS8Call, 20m band' },
  { label: 'MOR',    freqKhz: 25670,  bg: 'orange',  description: 'Station marker' },
  // --- Real SWBC stations (EiBi-style) ---
  { label: 'RRI',    freqKhz: 3325,   bg: '#4fc3f7', country: 'Romania', language: 'Romanian',  schedule: '0600-2200', description: 'Radio Romania International, 90m' },
  { label: 'R.Cuba', freqKhz: 5040,   bg: '#4fc3f7', country: 'Cuba',    language: 'Spanish',   schedule: '1100-0500', description: 'Radio Habana Cuba, 60m' },
  { label: 'DW',     freqKhz: 6075,   bg: '#4fc3f7', country: 'Germany', language: 'German',    schedule: '0400-0800', description: 'Deutsche Welle, 49m' },
  { label: 'VOA',    freqKhz: 7485,   bg: '#4fc3f7', country: 'USA',     language: 'English',   schedule: '1300-1600', description: 'Voice of America, 41m' },
  { label: 'R.Rus',  freqKhz: 5905,   bg: '#4fc3f7', country: 'Russia',  language: 'Russian',   schedule: '0500-2100', description: 'Radio Rossii, 49m' },
  { label: 'BBC',    freqKhz: 9410,   bg: '#4fc3f7', country: 'UK',      language: 'English',   schedule: '0400-0800', description: 'BBC World Service, 31m' },
  { label: 'R.Japan',freqKhz: 11895,  bg: '#4fc3f7', country: 'Japan',   language: 'Japanese',  schedule: '0800-1200', description: 'NHK World Radio Japan, 25m' },
  { label: 'R.CAT',  freqKhz: 9440,   bg: '#4fc3f7', country: 'Spain',   language: 'Catalan',   schedule: '1800-2000', description: 'Ràdio Exterior de Catalunya, 31m' },
  { label: 'CRI',    freqKhz: 13755,  bg: '#4fc3f7', country: 'China',   language: 'English',   schedule: '1200-1400', description: 'China Radio International, 22m' },
  { label: 'R.Aust', freqKhz: 13730,  bg: '#4fc3f7', country: 'Austria', language: 'German',    schedule: '0700-0900', description: 'Radio Austria International, 22m' },
  { label: 'Vatican',freqKhz: 11855,  bg: '#4fc3f7', country: 'Vatican', language: 'Italian',   schedule: '0600-2200', description: 'Vatican Radio, 25m' },
  { label: 'R.Korea',freqKhz: 9770,   bg: '#4fc3f7', country: 'South Korea', language: 'Korean', schedule: '0900-1200', description: 'KBS World Radio, 31m' },
]

const activeTags = computed<PopupTag[]>(() => props.tags.length > 0 ? props.tags as PopupTag[] : DEMO_TAGS)

const visibleTags = computed<PopupTag[]>(() =>
  activeTags.value.filter(t =>
    t.freqKhz >= props.viewLowKhz && t.freqKhz <= props.viewHighKhz
  )
)

// --- Popup state ---
const popupVisible = ref(false)
const popupTag = ref<PopupTag | null>(null)
const popupPos = ref({ x: 0, y: 0 })

function onTagClick(tag: PopupTag, event: MouseEvent) {
  popupTag.value = tag
  popupPos.value = { x: event.clientX + 12, y: event.clientY + 8 }
  popupVisible.value = true
}

function freqToPercent(freqKhz: number): number {
  const span = props.viewHighKhz - props.viewLowKhz
  if (span <= 0) return 0
  return ((freqKhz - props.viewLowKhz) / span) * 100
}
</script>

<style scoped>
.tag-area {
  position: relative;
  height: 22px;
  background: #aaaaaa;
  flex-shrink: 0;
  overflow: hidden;
  border-bottom: 1px solid #888;
}

.tag-area__tag {
  position: absolute;
  top: 2px;
  height: 18px;
  font-size: 9px;
  padding: 1px 3px;
  border: 1px solid rgba(0,0,0,0.25);
  border-radius: 2px;
  cursor: pointer;
  white-space: nowrap;
  line-height: 16px;
  transform: translateX(-50%);
  user-select: none;
  font-weight: bold;
}

.tag-area__tag:hover { filter: brightness(1.1); z-index: 1; }
</style>
