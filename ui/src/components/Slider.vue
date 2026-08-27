<template>
  <div class="slider-wrapper">
    <label :aria-label="label">{{ label }}</label>
    <input
      type="range"
      :min="min"
      :max="max"
      :step="step"
      :value="value"
      @input="onInput"
    />
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
  { min: 0, max: 100, step: 1 }
)

const emit = defineEmits<{
  (e: 'update:value', value: number): void
}>()

const ariaLabel = computed(() => props.label)

function onInput(event: Event) {
  const target = event.target as HTMLInputElement
  emit('update:value', Number(target.value))
}
</script>

<style scoped>
.slider-wrapper {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 4px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: #eee;
}

label {
  font-size: 13px;
  min-width: 80px;
}

input[type="range"] {
  flex: 1;
  width: 100%;
  height: 8px;
  background: #333;
  border-radius: 4px;
  -webkit-appearance: none;
  appearance: none;
}

input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 16px;
  height: 16px;
  background: #4CAF50;
  border-radius: 50%;
  cursor: pointer;
  margin-top: -4px;
}

input[type="range"]:focus {
  outline: none;
}
</style>