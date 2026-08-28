<template>
  <div class="ext-ignore" data-testid="ext-iq">
    <p class="ext-ignore__title">IQ Display</p>
    <p class="ext-ignore__msg">
      I/Q scatter plot — will render from the existing audio stream (deferred).
    </p>
  </div>
</template>

<style scoped>
.ext-ignore__title {
  color: var(--kiwi-accent, #4CAF50);
  font-size: var(--kiwi-font-md, 12px);
  margin: 0 0 4px;
}
.ext-ignore__msg {
  color: #888;
  font-size: var(--kiwi-font-sm, 11px);
  margin: 0;
}
</style>