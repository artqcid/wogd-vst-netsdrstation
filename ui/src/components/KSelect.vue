<template>
  <div class="k-select" data-testid="k-select">
    <label v-if="label" :for="id" class="k-select__label" :aria-label="label">{{ label }}</label>
    <select :id="id" class="k-select__control" :value="modelValue" @change="onChange" :aria-label="label">
      <option v-for="opt in options" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
    </select>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface SelectOption {
  value: string | number
  label: string
}

const props = withDefaults(
  defineProps<{
    modelValue: string | number
    options: SelectOption[]
    label?: string
  }>(),
  { label: undefined }
)

const emit = defineEmits<{ (e: 'update:modelValue', value: string | number): void }>()

const id = computed(() => `k-select-${Math.random().toString(36).slice(2, 8)}`)

function onChange(event: Event) {
  const target = event.target as HTMLSelectElement
  emit('update:modelValue', target.value)
}
</script>

<style scoped>
.k-select {
  display: flex;
  flex-direction: column;
  gap: 2px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: var(--kiwi-text, #ddd);
}

.k-select__label {
  font-size: var(--kiwi-font-sm, 11px);
}

.k-select__control {
  padding: 4px 8px;
  background: var(--kiwi-input-bg, #444);
  color: #fff;
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  font-size: var(--kiwi-font-md, 12px);
}
</style>