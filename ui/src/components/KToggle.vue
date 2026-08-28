<template>
  <button
    type="button"
    class="k-toggle"
    :class="{ 'k-toggle--on': modelValue }"
    :aria-pressed="modelValue"
    :aria-label="label"
    data-testid="k-toggle"
    @click="emit('update:modelValue', !modelValue)"
  >
    <span class="k-toggle__box" aria-hidden="true"></span>
    <span class="k-toggle__label">{{ label }}</span>
  </button>
</template>

<script setup lang="ts">
const props = withDefaults(
  defineProps<{
    modelValue: boolean
    label?: string
  }>(),
  { label: '' }
)

const emit = defineEmits<{ (e: 'update:modelValue', value: boolean): void }>()
</script>

<style scoped>
.k-toggle {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 3px 8px;
  background: var(--kiwi-panel, #333);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  color: var(--kiwi-text, #ddd);
  font-size: var(--kiwi-font-md, 12px);
  font-family: 'Segoe UI', Arial, sans-serif;
  cursor: pointer;
}

.k-toggle__box {
  width: 12px;
  height: 12px;
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 2px;
  background: transparent;
}

.k-toggle--on {
  border-color: var(--kiwi-accent, #4CAF50);
  color: #fff;
}

.k-toggle--on .k-toggle__box {
  background: var(--kiwi-accent, #4CAF50);
}
</style>