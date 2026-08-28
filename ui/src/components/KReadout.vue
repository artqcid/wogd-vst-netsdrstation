<template>
  <span class="k-readout" data-testid="k-readout">
    {{ formatted }}
    <span v-if="unit" class="k-readout__unit">{{ unit }}</span>
  </span>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    value: number | string
    unit?: string
    digits?: number
  }>(),
  { unit: undefined, digits: undefined }
)

// Monospace readout; numbers are formatted with fixed decimals.
const formatted = computed(() => {
  if (typeof props.value === 'number' && props.digits !== undefined) {
    return props.value.toFixed(props.digits)
  }
  return String(props.value)
})
</script>

<style scoped>
.k-readout {
  font-family: Consolas, 'Courier New', monospace;
  font-size: var(--kiwi-font-lg, 14px);
  color: var(--kiwi-text, #ddd);
  background: var(--kiwi-bg, #222);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  padding: 2px 8px;
  display: inline-block;
  min-width: 80px;
  text-align: center;
}

.k-readout__unit {
  font-size: var(--kiwi-font-sm, 11px);
  color: #999;
  margin-left: 4px;
}
</style>