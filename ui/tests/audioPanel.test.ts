import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import AudioPanel from '@/components/AudioPanel.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

let pinia: Pinia

function mountPanel() {
  return mount(AudioPanel, { global: { plugins: [pinia] } })
}

describe('AudioPanel', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
    delete (window as unknown as Record<string, unknown>).vstHost
  })

  it('volume slider forwards to pluginService.setParameter("volume")', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.find('input[type="range"]').setValue('0.5')

    expect(store.volume).toBe(0.5)
    expect(spy).toHaveBeenCalledWith('volume', 0.5)
  })

  it('mute toggle forwards 0/1', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const toggles = wrapper.findAll('.k-toggle')
    await toggles[0].trigger('click') // Mute (first toggle in the row)

    expect(store.mute).toBe(true)
    expect(spy).toHaveBeenCalledWith('mute', 1)
  })

  it('AGC on toggle forwards 1', async () => {
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    await wrapper.findAll('.k-toggle')[1].trigger('click') // AGC On (starts true)

    expect(spy).toHaveBeenCalledWith('agcOn', 0)
  })

  it('AGC threshold input forwards agcThresh', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const numberInputs = wrapper.findAll('input[type="number"]')
    await numberInputs[0].setValue('-90')

    expect(store.agcThresh).toBe(-90)
    expect(spy).toHaveBeenCalledWith('agcThresh', -90)
  })

  it('squelch toggle + threshold', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    // order: Mute, AGC On, AGC Hang, Squelch On, NB, NR
    await wrapper.findAll('.k-toggle')[3].trigger('click') // Squelch On
    expect(spy).toHaveBeenCalledWith('squelchOn', 1)

    // second range input is the squelch threshold
    const ranges = wrapper.findAll('input[type="range"]')
    await ranges[1].setValue('0.8')
    expect(store.squelchThr).toBe(0.8)
    expect(spy).toHaveBeenCalledWith('squelchThr', 0.8)
  })

  it('NB and NR toggles forward correct IDs', async () => {
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const toggles = wrapper.findAll('.k-toggle')
    await toggles[4].trigger('click') // NB
    expect(spy).toHaveBeenCalledWith('nbOn', 1)

    await toggles[5].trigger('click') // NR
    expect(spy).toHaveBeenCalledWith('nrOn', 1)
  })
})