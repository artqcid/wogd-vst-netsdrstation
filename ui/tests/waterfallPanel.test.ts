import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import WaterfallPanel from '@/components/WaterfallPanel.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

let pinia: Pinia

function mountPanel() {
  return mount(WaterfallPanel, { global: { plugins: [pinia] } })
}

describe('WaterfallPanel', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
    delete (window as unknown as Record<string, unknown>).vstHost
  })

  it('renders the waterfall canvas + controls', () => {
    const wrapper = mountPanel()
    expect(wrapper.find('canvas').exists()).toBe(true)
    expect(wrapper.findAll('.k-select').length).toBe(3) // speed/color/mode
  })

  it('zoom + forwards wfZoom', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const plusBtn = wrapper.findAll('.k-button').find(b => b.text() === '+')!
    await plusBtn.trigger('click')

    expect(spy).toHaveBeenCalledWith('wfZoom', 1)
  })

  it('zoom Max In sets wfZoom=3', async () => {
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const maxIn = wrapper.findAll('.k-button').find(b => b.text() === 'Max In')!
    await maxIn.trigger('click')

    expect(spy).toHaveBeenCalledWith('wfZoom', 3)
  })

  it('WF Max dB slider forwards wfMaxDb', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const ranges = wrapper.findAll('input[type="range"]')
    await ranges[0].setValue('-10')

    expect(store.wfMaxDb).toBe(-10)
    expect(spy).toHaveBeenCalledWith('wfMaxDb', -10)
  })

  it('color map select updates the store display state', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()

    const selects = wrapper.findAll('select')
    await selects[1].setValue('rain') // Color

    expect(store.colorMap).toBe('rain')
  })

  it('speed select forwards wfSpeed', async () => {
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.findAll('select')[0].setValue('3') // Fast

    expect(spy).toHaveBeenCalledWith('wfSpeed', 3)
  })

  it('CIC toggle forwards wfComp', async () => {
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.find('.k-toggle').trigger('click') // CIC

    expect(spy).toHaveBeenCalledWith('wfComp', 1)
  })
})