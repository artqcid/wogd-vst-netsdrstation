<template>
  <button
    class="toggle-button"
    :class="{ 'toggle-button--active': active }"
    :aria-pressed="active"
    @click="onClick"
  >
    {{ active ? 'On' : 'Off' }}
  </button>
</template>

<script setup lang="ts">
const props = withDefaults(
  defineProps<{
    label: string
    active: boolean
  }>(),
  { active: false }
)

const emit = defineEmits<{
  (e: 'update:active', value: boolean): void
}>()

function onClick() {
  emit('update:active', !props.active)
}
</script>

<style scoped>
.toggle-button {
  padding: 6px 12px;
  background: #3a3a3a;
  color: #e0e0e0;
  border: 1px solid #555;
  border-radius: 4px;
  cursor: pointer;
  font-size: 13px;
  transition: background 0.1s ease;
  white-space: nowrap;
}

.toggle-button--active {
  background: #4CAF50;
  color: #fff;
  border-color: #4CAF50;
}
</style>