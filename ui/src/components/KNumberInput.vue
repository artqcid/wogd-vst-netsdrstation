<template>
  <div class="k-number" data-testid="k-number-input">
    <label v-if="label" :for="id" class="k-number__label" :aria-label="label">{{ label }}</label>
    <input
      :id="id"
      type="number"
      class="k-number__input"
      :min="min"
      :max="max"
      :step="step"
      :value="modelValue"
      :aria-label="label"
      @input="onInput"
      @keydown.up.prevent="stepBy(1)"
      @keydown.down.prevent="stepBy(-1)"
    />
    <span v-if="unit" class="k-number__unit">{{ unit }}</span>
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
  { min: 0, max: 100, step: 1, unit: undefined, label: undefined }
)

const emit = defineEmits<{ (e: 'update:modelValue', value: number): void }>()

const id = computed(() => `k-number-${Math.random().toString(36).slice(2, 8)}`)

function onInput(event: Event) {
  const target = event.target as HTMLInputElement
  emit('update:modelValue', Number(target.value))
}

/** Arrow-key increment (w3_util pattern). */
function stepBy(direction: number) {
  const next = clamp(props.modelValue + direction * props.step)
  emit('update:modelValue', next)
}

function clamp(value: number): number {
  return Math.min(props.max, Math.max(props.min, value))
}
</script>

<style scoped>
.k-number {
  display: flex;
  align-items: center;
  gap: 6px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: var(--kiwi-text, #ddd);
}

.k-number__label {
  font-size: var(--kiwi-font-sm, 11px);
  min-width: 60px;
}

.k-number__input {
  width: 90px;
  padding: 3px 6px;
  background: var(--kiwi-input-bg, #444);
  color: #fff;
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  font-size: var(--kiwi-font-md, 12px);
}

.k-number__unit {
  font-size: var(--kiwi-font-sm, 11px);
  color: #999;
}
</style>