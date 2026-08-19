<template>
  <div class="knob">
    <label class="knob__label">{{ label }}</label>
    <input
      type="range"
      class="knob__input"
      :min="min"
      :max="max"
      :step="step"
      :value="value"
      aria-label="label"
      @input="onInput"
    />
    <div class="knob__value">{{ displayValue }}</div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    label: string
    value: number
    min?: number
    max?: number
    step?: number
  }>(),
  { min: 0, max: 1, step: 0.01 },
)

const emit = defineEmits<{
  (e: 'update:value', value: number): void
}>()

const displayValue = computed(() =>
  props.max <= 1 ? props.value.toFixed(2) : props.value.toFixed(0),
)

function onInput(event: Event) {
  const target = event.target as HTMLInputElement
  emit('update:value', Number(target.value))
}
</script>

<style scoped>
.knob {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
  padding: 12px;
}

.knob__label {
  font-size: 13px;
  color: #aaa;
}

.knob__value {
  font-size: 14px;
  color: #42b983;
  font-variant-numeric: tabular-nums;
}
</style>
