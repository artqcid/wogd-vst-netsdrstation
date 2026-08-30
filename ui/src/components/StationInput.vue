<template>
  <div data-testid="station-input">
    <label :aria-label="label">{{ label }}</label>
    <input
      type="text"
      :placeholder="station"
      v-model="editable"
      @keydown.enter="onConnect"
    />
    <span class="status-label" :class="statusClass">{{ statusLabel }}</span>
    <button @click="onConnect">{{ buttonLabel }}</button>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue'

const props = withDefaults(
  defineProps<{
    label?: string
    station: string
    status?: string
  }>(),
  { label: 'Station', station: 'kphsdr.com:8073', status: '' }
)

const emit = defineEmits<{
  (e: 'connect', value: string): void
  (e: 'disconnect'): void
}>()

const isConnected = computed(() => props.status.toLowerCase() === 'connected')

const buttonLabel = computed(() => (isConnected.value ? 'Disconnect' : 'Connect'))

const statusClass = computed(() => {
  const s = props.status.toLowerCase()
  if (s === 'connected') return 'status--connected'
  if (s === 'connecting') return 'status--connecting'
  if (s === 'error' || s === 'disconnected') return 'status--error'
  return 'status--idle'
})

const statusLabel = computed(() => {
  if (!props.status) return 'Not connected'
  return props.status
})

// Local editable copy so typing doesn't fight the prop binding. The parent
// owns the committed value; we only reflect prop changes into the field.
const editable = ref(props.station)
watch(
  () => props.station,
  value => {
    editable.value = value
  }
)

function onConnect() {
  if (isConnected.value) {
    emit('disconnect')
  } else {
    emit('connect', editable.value.trim())
  }
}
</script>

<style scoped>
div {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 4px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: #eee;
}

label {
  font-size: 13px;
  min-width: 60px;
}

input {
  padding: 4px 8px;
  background: #333;
  color: #fff;
  border: 1px solid #555;
  border-radius: 4px;
  font-size: 13px;
}

button {
  padding: 4px 12px;
  background: #4CAF50;
  color: #fff;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 13px;
  margin-left: 8px;
}

button:hover {
  background: #45a049;
}

.status-label {
  font-size: 11px;
  font-weight: 600;
  padding: 2px 6px;
  border-radius: 3px;
  margin-bottom: 2px;
}

.status--idle {
  color: #888;
  background: transparent;
}

.status--connecting {
  color: #f0c040;
  background: rgba(240, 192, 64, 0.12);
}

.status--connected {
  color: #4CAF50;
  background: rgba(76, 175, 80, 0.12);
}

.status--error {
  color: #f44336;
  background: rgba(244, 67, 54, 0.12);
}
</style>