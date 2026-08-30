import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { nextTick } from 'vue'
import CursorBar from '@/components/CursorBar.vue'

// ---- jsdom polyfills (layout + PointerEvent are missing in jsdom) ----
const RULER_WIDTH = 800

Object.defineProperty(HTMLElement.prototype, 'clientWidth', {
  configurable: true,
  get() {
    return RULER_WIDTH
  },
})

Object.defineProperty(HTMLElement.prototype, 'getBoundingClientRect', {
  configurable: true,
  value() {
    return {
      left: 0, top: 0, right: RULER_WIDTH, bottom: 20,
      width: RULER_WIDTH, height: 20, x: 0, y: 0,
      toJSON: () => ({}),
    }
  },
})

if (typeof globalThis.PointerEvent === 'undefined') {
  class PointerEventPolyfill extends MouseEvent {
    pointerId = 1
    pointerType = 'mouse'
    isPrimary = true
    constructor(type: string, init: PointerEventInit = {}) {
      super(type, init)
    }
  }
  globalThis.PointerEvent = PointerEventPolyfill as unknown as typeof PointerEvent
}
// ----------------------------------------------------------------

async function mountBar(overrides: Record<string, unknown> = {}) {
  const wrapper = mount(CursorBar, {
    props: {
      viewLowKhz: 6950,   // 100 kHz window: [6950, 7050]
      viewHighKhz: 7050,
      cursorKhz: 7000,
      lowCutHz: -4900,
      highCutHz: 4900,
      ...overrides,
    },
    attachTo: document.body,  // needed for pointer events
  })
  // jsdom has no layout: force the width on the root element instance and
  // re-read it via the component's window resize handler.
  Object.defineProperty(wrapper.element, 'clientWidth', {
    configurable: true,
    value: RULER_WIDTH,
  })
  window.dispatchEvent(new Event('resize'))
  await nextTick()
  return wrapper
}

// Helper: simulate pointerdown + pointermove + pointerup on an element
async function drag(el: HTMLElement, dx: number) {
  const rect = el.getBoundingClientRect()
  const cx = rect.left + rect.width / 2
  el.dispatchEvent(new PointerEvent('pointerdown', { bubbles: true, clientX: cx }))
  el.dispatchEvent(new PointerEvent('pointermove', { bubbles: true, clientX: cx + dx }))
  el.dispatchEvent(new PointerEvent('pointerup', { bubbles: true }))
}

describe('CursorBar.vue', () => {
  beforeEach(() => {
    vi.useFakeTimers()
  })

  // Cleanup after each test
  const teardown = () => {
    vi.useRealTimers()
  }

  it('renders all zones when resizable', async () => {
    const wrapper = await mountBar()
    expect(wrapper.get('[data-testid="cursor-bar"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="cursor-zone-lo"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="cursor-zone-hi"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="cursor-zone-center"]').exists()).toBe(true)
    teardown()
    wrapper.unmount()
  })

  it('collapsed cursor has NO lo/hi zones', async () => {
    const wrapper = await mountBar({ lowCutHz: -100, highCutHz: 100 })
    expect(wrapper.get('[data-testid="cursor-bar"]').exists()).toBe(true)
    expect(wrapper.find('[data-testid="cursor-zone-lo"]').exists()).toBe(false)
    expect(wrapper.find('[data-testid="cursor-zone-hi"]').exists()).toBe(false)
    expect(wrapper.get('[data-testid="cursor-zone-center"]').exists()).toBe(true)
    teardown()
    wrapper.unmount()
  })

  it('center drag emits tune', async () => {
    const wrapper = await mountBar()
    const centerZone = wrapper.get('[data-testid="cursor-zone-center"]').element as HTMLElement
    await drag(centerZone, 100)
    const tuneEmitted = wrapper.emitted('tune')
    expect(tuneEmitted).toBeTruthy()
    expect(tuneEmitted!.length).toBe(1)
    const freqKhz = tuneEmitted![0][0] as number
    expect(freqKhz).toBeGreaterThan(7000)
    teardown()
    wrapper.unmount()
  })

  it('center drag does NOT emit lowCut/highCut', async () => {
    const wrapper = await mountBar()
    const centerZone = wrapper.get('[data-testid="cursor-zone-center"]').element as HTMLElement
    await drag(centerZone, 100)
    expect(wrapper.emitted('update:lowCut')).toBeFalsy()
    expect(wrapper.emitted('update:highCut')).toBeFalsy()
    teardown()
    wrapper.unmount()
  })

  it('lo zone drag emits update:lowCut', async () => {
    const wrapper = await mountBar()
    const loZone = wrapper.get('[data-testid="cursor-zone-lo"]').element as HTMLElement
    await drag(loZone, 50)
    const lowCutEmitted = wrapper.emitted('update:lowCut')
    expect(lowCutEmitted).toBeTruthy()
    expect(lowCutEmitted!.length).toBe(1)
    const lowCutValue = lowCutEmitted![0][0] as number
    // Moved right → value > -4900; moved left → value < -4900
    expect(lowCutValue).not.toBe(-4900)
    // No tune emitted
    expect(wrapper.emitted('tune')).toBeFalsy()
    teardown()
    wrapper.unmount()
  })

  it('hi zone drag emits update:highCut', async () => {
    const wrapper = await mountBar()
    const hiZone = wrapper.get('[data-testid="cursor-zone-hi"]').element as HTMLElement
    await drag(hiZone, 50)
    const highCutEmitted = wrapper.emitted('update:highCut')
    expect(highCutEmitted).toBeTruthy()
    expect(highCutEmitted!.length).toBe(1)
    const highCutValue = highCutEmitted![0][0] as number
    expect(highCutValue).not.toBe(4900)
    // No tune emitted
    expect(wrapper.emitted('tune')).toBeFalsy()
    teardown()
    wrapper.unmount()
  })

  it('carrier colour rule: wide passband → lime, narrow → yellow', async () => {
    // Wide passband → lime
    const wideWrapper = await mountBar({ lowCutHz: -4900, highCutHz: 4900 })
    const wideBar = wideWrapper.get('[data-testid="cursor-bar"]')
    expect(wideBar.attributes('data-cursor-color')).toBe('lime')
    teardown()
    wideWrapper.unmount()

    // Narrow passband → yellow
    const narrowWrapper = await mountBar({ lowCutHz: -100, highCutHz: 100 })
    const narrowBar = narrowWrapper.get('[data-testid="cursor-bar"]')
    expect(narrowBar.attributes('data-cursor-color')).toBe('yellow')
    teardown()
    narrowWrapper.unmount()
  })
})
