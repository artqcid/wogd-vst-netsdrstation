<script setup lang="ts">
import { ref } from 'vue'
import PluginView from './views/PluginView.vue'
import { useUiScale, REF_WIDTH, REF_HEIGHT } from './composables/useUiScale'

const viewport = ref<HTMLElement | null>(null)
const { scale } = useUiScale(viewport)
</script>

<template>
  <div ref="viewport" class="ui-viewport">
    <div
      class="ui-surface"
      :style="{
        width: REF_WIDTH + 'px',
        height: REF_HEIGHT + 'px',
        transform: `scale(${scale})`,
      }"
    >
      <PluginView />
    </div>
  </div>
</template>

<style scoped>
/* Vectorial scaling: a fixed 1280x720 design surface is scaled uniformly to
   the host window (contain, centered). Everything grows proportionally. */
.ui-viewport {
  width: 100%;
  height: 100%;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--kiwi-bg, #1e1e1e);
}

.ui-surface {
  transform-origin: center center;
  flex-shrink: 0;
  overflow: hidden;
  background: var(--kiwi-bg, #1e1e1e);
}
</style>