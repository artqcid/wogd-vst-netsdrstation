<template>
  <button
    type="button"
    class="k-button"
    :class="{ 'k-button--active': active }"
    :aria-pressed="active ?? undefined"
    data-testid="k-button"
    @click="emit('click')"
  >
    <slot>{{ label }}</slot>
  </button>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    label?: string
    active?: boolean
  }>(),
  { label: '', active: false }
)

const emit = defineEmits<{ (e: 'click'): void }>()
</script>

<style scoped>
.k-button {
  padding: 4px 10px;
  background: var(--kiwi-input-bg, #444);
  color: var(--kiwi-text, #ddd);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  font-size: var(--kiwi-font-md, 12px);
  font-family: 'Segoe UI', Arial, sans-serif;
  cursor: pointer;
}

.k-button:hover {
  background: #4d4d4d;
}

.k-button--active {
  background: var(--kiwi-accent, #4CAF50);
  border-color: var(--kiwi-accent, #4CAF50);
  color: #fff;
}
</style>