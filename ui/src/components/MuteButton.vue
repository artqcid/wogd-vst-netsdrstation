<template>
  <button
    class="mute-button"
    :class="{ 'mute-button--active': active }"
    :aria-pressed="active"
    @click="emit('toggle')"
  >
    {{ active ? 'Muted' : 'Mute' }}
  </button>
</template>

<script setup lang="ts">
defineProps<{ active: boolean }>()
const emit = defineEmits<{ (e: 'toggle'): void }>()
</script>

<style scoped>
.mute-button {
  padding: 10px 24px;
  background: #3a3a3a;
  color: #e0e0e0;
  border: 1px solid #555;
  border-radius: 6px;
  cursor: pointer;
  font-size: 15px;
  transition: background 0.1s ease;
}

.mute-button--active {
  background: #c0392b;
  color: #fff;
  border-color: #c0392b;
}
</style>
