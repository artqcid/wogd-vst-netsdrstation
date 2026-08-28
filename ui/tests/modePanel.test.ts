import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import ModePanel from '@/components/ModePanel.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

let pinia: Pinia

function mountPanel() {
  return mount(ModePanel, { global: { plugins: [pinia] } })
}

describe('ModePanel', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
    delete (window as unknown as Record<string, unknown>).vstHost
  })

  it('renders all 18 mode buttons', () => {
    const wrapper = mountPanel()
    expect(wrapper.findAll('.k-button').length).toBeGreaterThanOrEqual(18)
  })

  it('highlights the active mode button', () => {
    const store = useKiwiStore()
    store.mode = 3 // USB
    const wrapper = mountPanel()

    const usb = wrapper.findAll('.k-button')[3]
    expect(usb.classes()).toContain('k-button--active')
  })

  it('selecting USB applies mode + default passband (300/-2700)', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.findAll('.k-button')[3].trigger('click') // USB

    expect(store.mode).toBe(3)
    expect(store.lowCut).toBe(300)
    expect(store.highCut).toBe(2700)
    expect(spy).toHaveBeenCalledWith('mode', 3)
    expect(spy).toHaveBeenCalledWith('lowCut', 300)
    expect(spy).toHaveBeenCalledWith('highCut', 2700)
  })

  it('selecting CW applies its default passband', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()

    await wrapper.findAll('.k-button')[7].trigger('click') // CW

    expect(store.mode).toBe(7)
    expect(store.lowCut).toBe(300)
    expect(store.highCut).toBe(800)
  })

  it('shows derived bandwidth readout', () => {
    const store = useKiwiStore()
    store.lowCut = -4900
    store.highCut = 4900
    const wrapper = mountPanel()

    expect(wrapper.find('.k-readout').text()).toContain('9800')
  })

  it('reset restores the current mode defaults', async () => {
    const store = useKiwiStore()
    store.mode = 0 // AM
    store.lowCut = -1000
    store.highCut = 1000
    const wrapper = mountPanel()

    const resetButton = wrapper
      .findAll('.k-button')
      .find(b => b.text() === 'Reset')!
    await resetButton.trigger('click')

    expect(store.lowCut).toBe(-4900)
    expect(store.highCut).toBe(4900)
  })

  it('lowCut input clamps to <= 0', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()

    const lowInput = wrapper.findAll('input[type="number"]')[0]
    await lowInput.setValue('500')

    expect(store.lowCut).toBe(0)
  })
})