<template>
  <div class="k-slider" data-testid="k-slider">
    <label v-if="label" :for="id" class="k-slider__label" :aria-label="label">{{ label }}</label>
    <input
      :id="id"
      type="range"
      class="k-slider__input"
      :min="min"
      :max="max"
      :step="step"
      :value="modelValue"
      :aria-label="label"
      @input="onInput"
    />
    <span v-if="unit" class="k-slider__unit">{{ modelValue }} {{ unit }}</span>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    modelValue: number
    min?: number
    max?: number
    step?: number
    unit?: string
    label?: string
  }>(),
  { min: 0, max: 1, step: 0.01, unit: undefined, label: undefined }
)

const emit = defineEmits<{ (e: 'update:modelValue', value: number): void }>()

// Stable id for label/input association (per-instance).
const id = computed(() => `k-slider-${Math.random().toString(36).slice(2, 8)}`)

function onInput(event: Event) {
  const target = event.target as HTMLInputElement
  emit('update:modelValue', Number(target.value))
}
</script>

<style scoped>
.k-slider {
  display: flex;
  flex-direction: column;
  gap: 2px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: var(--kiwi-text, #ddd);
}

.k-slider__label {
  font-size: var(--kiwi-font-sm, 11px);
}

/* w3_ext pattern: 3 px track, 18 px thumb */
.k-slider__input {
  appearance: none;
  -webkit-appearance: none;
  width: 100%;
  height: 3px;
  background: var(--kiwi-input-bg, #444);
  border-radius: 2px;
  outline: none;
}

.k-slider__input::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--kiwi-accent, #4CAF50);
  border: 2px solid var(--kiwi-border, #555);
  cursor: pointer;
}

.k-slider__input::-moz-range-thumb {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--kiwi-accent, #4CAF50);
  border: 2px solid var(--kiwi-border, #555);
  cursor: pointer;
}

.k-slider__unit {
  font-size: var(--kiwi-font-sm, 11px);
  color: #999;
}
</style>