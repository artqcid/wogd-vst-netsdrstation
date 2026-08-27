<template>
  <div>
    <label :aria-label="label">{{ label }}</label>
    <input
      type="number"
      :min="min"
      :max="max"
      :step="step"
      :value="value"
      @input="onInput"
    />
    <span class="suffix" v-if="suffix">{{ suffix }}</span>
  </div>
</template>

<script setup lang="ts">
const props = withDefaults(
  defineProps<{
    label: string
    value: number
    min?: number
    max?: number
    step?: number
    suffix?: string
  }>(),
  { min: 0, max: 100, step: 1 }
)

const emit = defineEmits<{
  (e: 'update:value', value: number): void
}>()

function onInput(event: Event) {
  const target = event.target as HTMLInputElement
  const parsed = Number(target.value)
  if (!Number.isNaN(parsed)) {
    emit('update:value', parsed)
  }
}
</script>

<style scoped>
div {
  display: flex;
  align-items: center;
  gap: 4px;
  font-family: 'Segoe UI', Arial, sans-serif;
  color: #eee;
}

label {
  min-width: 80px;
  font-size: 13px;
}

input {
  flex: 1;
  padding: 4px 8px;
  background: #333;
  color: #fff;
  border: 1px solid #555;
  border-radius: 4px;
  font-size: 13px;
}

.suffix {
  padding: 0 4px;
  color: #888;
  font-size: 12px;
}
</style>