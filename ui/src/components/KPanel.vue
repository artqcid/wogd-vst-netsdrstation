<template>
  <section class="k-panel" data-testid="k-panel">
    <header v-if="title" class="k-panel__title">{{ title }}</header>
    <div class="k-panel__body">
      <slot />
    </div>
  </section>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    title?: string
  }>(),
  { title: undefined }
)
</script>

<style scoped>
.k-panel {
  background: var(--kiwi-panel, #333);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 6px;
  padding: 8px 10px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: var(--kiwi-text, #ddd);
}

.k-panel__title {
  color: var(--kiwi-accent, #4CAF50);
  font-size: var(--kiwi-font-sm, 11px);
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 6px;
}

.k-panel__body {
  display: flex;
  flex-direction: column;
  gap: 6px;
}
</style>