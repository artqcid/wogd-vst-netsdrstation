<template>
  <span class="status-badge">
    <span class="dot" :class="dotClass"></span>
    {{ status }}
  </span>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    status: string
  }>(),
  { status: 'Idle' }
)

const dotClass = computed(() =>
  /connect/i.test(props.status) ? 'dot--green' : 'dot--grey'
)
</script>

<style scoped>
.status-badge {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  background: #333;
  border: 1px solid #555;
  border-radius: 4px;
  font-size: 12px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: #eee;
}

.dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
}

.dot--green {
  background: #4CAF50;
}

.dot--grey {
  background: #666;
}
</style>