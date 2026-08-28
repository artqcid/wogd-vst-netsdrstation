<template>
  <Teleport to="body">
    <div
      v-if="visible"
      class="tag-popup-overlay"
      @click.self="$emit('close')"
    >
      <div
        class="tag-popup"
        :style="positionStyle"
      >
        <div class="tag-popup__header">{{ tag?.label }}</div>
        <table class="tag-popup__info">
          <tbody>
            <tr>
              <td class="tag-popup__label">Freq</td>
              <td>{{ tag?.freqKhz }} kHz</td>
            </tr>
            <tr v-if="tag?.country">
              <td class="tag-popup__label">Country</td>
              <td>{{ tag.country }}</td>
            </tr>
            <tr v-if="tag?.language">
              <td class="tag-popup__label">Language</td>
              <td>{{ tag.language }}</td>
            </tr>
            <tr v-if="tag?.schedule">
              <td class="tag-popup__label">Schedule</td>
              <td>{{ tag.schedule }}</td>
            </tr>
            <tr v-if="tag?.description">
              <td class="tag-popup__label">Info</td>
              <td>{{ tag.description }}</td>
            </tr>
          </tbody>
        </table>
        <button class="tag-popup__tune" @click="onTune">Tune →</button>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import { computed } from 'vue'

export interface PopupTag {
  label: string
  freqKhz: number
  bg: string
  fg?: string
  country?: string
  language?: string
  schedule?: string
  description?: string
}

const props = withDefaults(defineProps<{
  visible: boolean
  tag: PopupTag | null
  /** Preferred position {x, y} near the clicked tag (viewport-relative px) */
  position?: { x: number; y: number }
}>(), {
  visible: false,
  tag: null,
  position: () => ({ x: 0, y: 0 }),
})

const emit = defineEmits<{
  (e: 'close'): void
  (e: 'tune', freqKhz: number): void
}>()

const positionStyle = computed(() => ({
  left: Math.max(10, Math.min(window.innerWidth - 260, props.position.x)) + 'px',
  top: Math.max(10, Math.min(window.innerHeight - 200, props.position.y)) + 'px',
}))

function onTune() {
  if (props.tag) emit('tune', props.tag.freqKhz)
}
</script>

<style scoped>
.tag-popup-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
  background: transparent;
}

.tag-popup {
  position: fixed;
  min-width: 180px;
  max-width: 250px;
  background: #1a1a1a;
  border: 1px solid #555;
  border-radius: 6px;
  padding: 8px 12px;
  box-shadow: 0 4px 16px rgba(0,0,0,0.6);
  font-family: monospace;
  font-size: 11px;
  color: #ddd;
  z-index: 10000;
}

.tag-popup__header {
  font-size: 13px;
  font-weight: bold;
  color: #fff;
  margin-bottom: 6px;
  padding-bottom: 4px;
  border-bottom: 1px solid #444;
}

.tag-popup__info {
  width: 100%;
  border-collapse: collapse;
}

.tag-popup__info td {
  padding: 1px 4px;
  vertical-align: top;
}

.tag-popup__label {
  color: #999;
  width: 60px;
  white-space: nowrap;
}

.tag-popup__tune {
  display: block;
  width: 100%;
  margin-top: 8px;
  padding: 4px 0;
  background: #7c4dff;
  color: white;
  border: none;
  border-radius: 4px;
  font-size: 11px;
  font-family: inherit;
  cursor: pointer;
}

.tag-popup__tune:hover {
  background: #9c7cff;
}
</style>
