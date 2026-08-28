import { ref, onMounted, onBeforeUnmount, type Ref } from 'vue'

/**
 * Vectorial (proportional) UI scaling — the whole KiwiSDR design surface is
 * laid out at a fixed reference size (REF_WIDTH x REF_HEIGHT) and scaled
 * uniformly to fill the host window. When the window grows, fonts, controls
 * and the waterfall grow proportionally (like a modern VST3 plugin), instead
 * of merely reflowing.
 *
 * scale = min(w/REF_WIDTH, h/REF_HEIGHT)  (contain; centered letterbox)
 */
export const REF_WIDTH = 1280
export const REF_HEIGHT = 720

export function useUiScale(container: Ref<HTMLElement | null>) {
  const scale = ref(1)

  function update() {
    const el = container.value
    if (!el) return
    const w = el.clientWidth
    const h = el.clientHeight
    if (w <= 0 || h <= 0) return
    scale.value = Math.min(w / REF_WIDTH, h / REF_HEIGHT)
  }

  let observer: ResizeObserver | null = null

  onMounted(() => {
    update()
    if (typeof ResizeObserver !== 'undefined') {
      observer = new ResizeObserver(update)
      if (container.value) observer.observe(container.value)
    }
  })

  onBeforeUnmount(() => {
    observer?.disconnect()
  })

  return { scale }
}