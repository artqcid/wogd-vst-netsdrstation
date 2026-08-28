import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import FreqPanel from '@/components/FreqPanel.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

let pinia: Pinia

function mountPanel() {
  return mount(FreqPanel, { global: { plugins: [pinia] } })
}

function buttonLabels(wrapper: ReturnType<typeof mountPanel>): string[] {
  return wrapper.findAll('.k-button').map(b => b.text())
}

describe('FreqPanel', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
    delete (window as unknown as Record<string, unknown>).vstHost
  })

  it('renders the six step buttons and a readout', () => {
    const wrapper = mountPanel()
    expect(wrapper.findAll('.k-button').length).toBe(6)
    expect(wrapper.find('.k-readout').exists()).toBe(true)
  })

  it('labels the step buttons in KiwiSDR order', () => {
    const wrapper = mountPanel()
    expect(buttonLabels(wrapper)).toEqual(['←10', '←1', '←0.1', '+0.1', '+1', '+10'])
  })

  it('+10 step adds 10 kHz and forwards to the bridge', async () => {
    const store = useKiwiStore()
    store.freqKhz = 14100
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.findAll('.k-button')[5].trigger('click') // +10

    expect(store.freqKhz).toBe(14110)
    expect(spy).toHaveBeenCalledWith('freqKhz', 14110)
  })

  it('-0.1 step subtracts 0.1 kHz', async () => {
    const store = useKiwiStore()
    store.freqKhz = 14100
    const wrapper = mountPanel()

    await wrapper.findAll('.k-button')[2].trigger('click') // -0.1

    expect(store.freqKhz).toBe(14099.9)
  })

  it('clamps step buttons to the [0.001, 30000] range', async () => {
    const store = useKiwiStore()
    store.freqKhz = 29999
    const wrapper = mountPanel()

    await wrapper.findAll('.k-button')[5].trigger('click') // +10

    expect(store.freqKhz).toBe(30000)
  })

  it('manual text entry updates the store (clamped)', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()

    const input = wrapper.find('input[type="number"]')
    await input.setValue('14250.5')

    expect(store.freqKhz).toBe(14250.5)
  })

  it('shows the frequency in a 7-digit readout', () => {
    const store = useKiwiStore()
    store.freqKhz = 14100
    const wrapper = mountPanel()

    expect(wrapper.find('.k-readout').text()).toContain('14100.000')
  })
})