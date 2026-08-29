<template>
  <div class="tag-area" aria-label="DX frequency labels">
    <span
      v-for="tag in visibleTags"
      :key="tag.label + tag.freqKhz"
      class="tag-area__tag"
      :class="{ 'tag-area__tag--ext': tag.hasExt }"
      :style="{
        left: freqToPercent(tag.freqKhz) + '%',
        top: tag.row === 0 ? '2px' : '24px',
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
  { label: 'LW 243', freqKhz: 243, bg: '#4CAF50' },
  { label: 'NAVTEX', freqKhz: 518, bg: '#4CAF50', hasExt: true },
  { label: 'WSPR', freqKhz: 630, bg: '#4CAF50', hasExt: true },
  { label: 'DSC', freqKhz: 2187.5, bg: '#4CAF50', hasExt: true },
  { label: 'STANAG DHO26', freqKhz: 1131, bg: '#f06292' },
  { label: 'STANAG OUA4', freqKhz: 1268, bg: '#f06292' },
  { label: 'MWARA CAR', freqKhz: 1377, bg: '#f06292' },
  { label: 'STANAG IDN', freqKhz: 1519, bg: '#f06292' },
  { label: 'MWARA SAT-1,2', freqKhz: 1638, bg: '#f06292' },
  { label: 'SSTV EU', freqKhz: 1890, bg: '#f06292', hasExt: true },
  { label: 'FAX ZAF', freqKhz: 2070, bg: 'yellow', hasExt: true },
  { label: 'STANAG DHJ58', freqKhz: 2138, bg: '#f06292' },
  { label: 'FAX GRC', freqKhz: 2248, bg: 'yellow', hasExt: true },
  { label: 'The Air Horn', freqKhz: 2612, bg: '#f06292' },
  { label: 'L marker', freqKhz: 2718, bg: '#f06292' },
  { label: 'The Pip', freqKhz: 3003, bg: '#f06292' },
  { label: 'VOLMET', freqKhz: 3485, bg: '#f06292' },
  { label: 'R. Mi Amigo Intl', freqKhz: 3315, bg: '#f06292' },
  { label: 'DSC (distress)', freqKhz: 2187.5, bg: '#f06292', hasExt: true },
  { label: 'MWARA SEA-1', freqKhz: 4125, bg: '#f06292' },
  { label: 'WSPR ISM', freqKhz: 3570, bg: '#4CAF50', hasExt: true },
  { label: 'V marker', freqKhz: 4625, bg: '#f06292' },
  { label: 'FAX THA', freqKhz: 4298, bg: 'yellow', hasExt: true },
  { label: 'DDH7 GER', freqKhz: 4583, bg: 'yellow', hasExt: true },
  { label: 'FAX GER', freqKhz: 4882, bg: 'yellow', hasExt: true },
  { label: 'VMW AUS', freqKhz: 4426, bg: '#f06292' },
  { label: 'STANAG PBC', freqKhz: 5680, bg: '#f06292' },
  { label: 'STANAG FUM', freqKhz: 6215, bg: '#f06292' },
  { label: 'MWARA SAT-1', freqKhz: 5505, bg: '#f06292' },
  { label: 'STANAG FUG', freqKhz: 6215, bg: '#f06292' },
  { label: 'HM01 CUB', freqKhz: 5820, bg: '#f06292' },
  { label: 'SuperDARN radar', freqKhz: 8000, bg: '#f06292' },
  { label: 'FAX RUS', freqKhz: 7781, bg: 'yellow', hasExt: true },
  { label: 'FAX AUS', freqKhz: 7535, bg: 'yellow', hasExt: true },
  { label: 'D marker', freqKhz: 8000, bg: '#f06292' },
  { label: 'FAX UK', freqKhz: 7880, bg: 'yellow', hasExt: true },
  { label: 'HFDL ZAF', freqKhz: 8825, bg: '#f06292', hasExt: true },
  { label: 'HM01 CUB', freqKhz: 9330, bg: '#f06292' },
  { label: 'STANAG FUJ', freqKhz: 9007, bg: '#f06292' },
  { label: 'VMW AUS', freqKhz: 9355, bg: '#f06292' },
  { label: 'FAX CHN', freqKhz: 10010, bg: 'yellow', hasExt: true },
  { label: 'PBB NLD', freqKhz: 11527, bg: 'yellow', hasExt: true },
  { label: 'FAX JPN', freqKhz: 13988, bg: 'yellow', hasExt: true },
  { label: 'MWARA NAT-B/D/F', freqKhz: 11384, bg: '#f06292' },
  { label: 'D marker', freqKhz: 14670, bg: '#f06292' },
  { label: 'FAX GER', freqKhz: 14467.3, bg: 'yellow', hasExt: true },
  { label: 'SSTV', freqKhz: 14230, bg: '#f06292', hasExt: true },
  { label: 'DDH8 GER', freqKhz: 14863, bg: 'yellow', hasExt: true },
  { label: 'STANAG FUG', freqKhz: 15867, bg: '#f06292' },
  { label: 'RWM RUS', freqKhz: 14996, bg: '#f06292' },
  { label: 'FAX AUS', freqKhz: 16135, bg: 'yellow', hasExt: true },
  { label: 'FAX GER', freqKhz: 17800, bg: 'yellow', hasExt: true },
  { label: 'D marker', freqKhz: 20048, bg: '#f06292' },
  { label: 'FAX CHN', freqKhz: 18010, bg: 'yellow', hasExt: true },
  { label: 'DSC (distress)', freqKhz: 16804.5, bg: '#f06292', hasExt: true },
  { label: 'FAX JPN', freqKhz: 17445, bg: 'yellow', hasExt: true },
  { label: 'FAX SGP', freqKhz: 16035, bg: 'yellow', hasExt: true },
  { label: 'HFDL PAN', freqKhz: 17919, bg: '#f06292', hasExt: true },
  { label: 'FAX ZAF', freqKhz: 18910, bg: 'yellow', hasExt: true },
  { label: 'STANAG FUV', freqKhz: 19680, bg: '#f06292' },
  { label: 'DSC', freqKhz: 19680.5, bg: '#f06292', hasExt: true },
  { label: 'NAVTEX', freqKhz: 24084, bg: '#4CAF50', hasExt: true },
  { label: 'WWV', freqKhz: 10000, bg: 'orange' },
  { label: 'FAX AUS', freqKhz: 20469, bg: 'yellow', hasExt: true },
  { label: 'FT8', freqKhz: 14074, bg: '#4CAF50', hasExt: true },
  { label: 'SSTV', freqKhz: 14230, bg: '#f06292', hasExt: true },
  { label: 'MWARA NP', freqKhz: 23210, bg: '#f06292' },
  { label: 'DSC', freqKhz: 22374, bg: '#f06292', hasExt: true },
  { label: 'FT8', freqKhz: 21074, bg: '#4CAF50', hasExt: true },
  { label: 'DSC', freqKhz: 23100, bg: '#f06292', hasExt: true },
  { label: 'NAVTEX', freqKhz: 25170, bg: '#4CAF50', hasExt: true },
  { label: 'FT8', freqKhz: 28074, bg: '#4CAF50', hasExt: true },
  { label: 'SSTV', freqKhz: 28680, bg: '#f06292', hasExt: true },
]

const activeTags = computed<PopupTag[]>(() => props.tags.length > 0 ? props.tags as PopupTag[] : DEMO_TAGS)

interface TagItem extends PopupTag { row: number }

const visibleTags = computed<TagItem[]>(() => {
  const sorted = activeTags.value
    .filter(t => t.freqKhz >= props.viewLowKhz && t.freqKhz <= props.viewHighKhz)
    .map(t => ({ ...t, row: 0 }))
    .sort((a, b) => a.freqKhz - b.freqKhz)

  // Kollisions-Detektion: wenn zwei Tags zu nahe, zweite Reihe
  const MIN_GAP_PCT = 3
  for (let i = 1; i < sorted.length; i++) {
    const prev = sorted[i - 1]
    const curr = sorted[i]
    if (prev && curr) {
      const pctDiff = freqToPercent(curr.freqKhz) - freqToPercent(prev.freqKhz)
      if (pctDiff < MIN_GAP_PCT) curr.row = prev.row === 0 ? 1 : 0
    }
  }
  return sorted
})

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
  height: 44px;        /* 2 Reihen à 22px */
  background: #aaaaaa;
  flex-shrink: 0;
  overflow: hidden;
  border-bottom: 1px solid #888;
}

.tag-area__tag {
  position: absolute;
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

.tag-area__tag--ext { border-color: #FFD700; border-style: dashed; }

.tag-area__tag:hover { filter: brightness(1.1); z-index: 1; }
</style>
