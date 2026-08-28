<template>
  <span class="k-status-badge" :class="`k-status-badge--${state}`" data-testid="k-status-badge">
    <span class="k-status-badge__dot" aria-hidden="true"></span>
    <span class="k-status-badge__label">{{ label }}</span>
  </span>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    state?: 'ok' | 'warn' | 'error'
    label?: string
  }>(),
  { state: 'ok', label: '' }
)
</script>

<style scoped>
.k-status-badge {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  padding: 3px 8px;
  background: var(--kiwi-panel, #333);
  border: 1px solid var(--kiwi-border, #555);
  border-radius: 3px;
  font-size: var(--kiwi-font-sm, 11px);
  font-family: 'Segoe UI', Arial, sans-serif;
  color: var(--kiwi-text, #ddd);
}

.k-status-badge__dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.k-status-badge--ok .k-status-badge__dot {
  background: var(--kiwi-accent, #4CAF50);
}

.k-status-badge--warn .k-status-badge__dot {
  background: #f0c040;
}

.k-status-badge--error .k-status-badge__dot {
  background: var(--kiwi-warn, #f44336);
}
</style>