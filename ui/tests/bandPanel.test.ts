import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import BandPanel from '@/components/BandPanel.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

let pinia: Pinia

function mountPanel() {
  return mount(BandPanel, { global: { plugins: [pinia] } })
}

/** In-memory localStorage (Node 25 + jsdom collide on the native one). */
function installStorageMock() {
  const store = new Map<string, string>()
  Object.defineProperty(window, 'localStorage', {
    value: {
      getItem: (k: string) => store.get(k) ?? null,
      setItem: (k: string, v: string) => void store.set(k, String(v)),
      removeItem: (k: string) => void store.delete(k),
      clear: () => store.clear(),
    },
    configurable: true,
  })
}

describe('BandPanel', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
    delete (window as unknown as Record<string, unknown>).vstHost
    installStorageMock()
  })

  it('renders the three band dropdowns', () => {
    const wrapper = mountPanel()
    expect(wrapper.findAll('.k-select').length).toBe(3)
  })

  it('selecting an amateur band emits its frequency', async () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')
    const wrapper = mountPanel()

    const amateur = wrapper.findAll('.k-select')[0]
    await amateur.find('select').setValue('14200') // 20 m

    expect(store.freqKhz).toBe(14200)
    expect(spy).toHaveBeenCalledWith('freqKhz', 14200)
  })

  it('selecting a timesig band emits its frequency', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()

    const utility = wrapper.findAll('.k-select')[2]
    await utility.find('select').setValue('77.5') // DCF77

    expect(store.freqKhz).toBe(77.5)
  })

  it('save current appends a bookmark', async () => {
    const store = useKiwiStore()
    store.freqKhz = 7100
    store.mode = 0
    const wrapper = mountPanel()

    const saveBtn = wrapper.findAll('.k-button').find(b => b.text() === 'Save current')!
    await saveBtn.trigger('click')

    expect(wrapper.findAll('.band-panel__item').length).toBe(1)
    expect(wrapper.text()).toContain('7100.000 kHz')
    // persisted to localStorage
    expect(localStorage.getItem('netsdrstation.bookmarks.v1')).toContain('7100')
  })

  it('load bookmark restores frequency and mode', async () => {
    const store = useKiwiStore()
    store.freqKhz = 1000
    store.mode = 0
    const wrapper = mountPanel()
    await wrapper.find('.k-button').trigger('click') // save (freq 1000)

    // change state, then load the bookmark back
    store.freqKhz = 2000
    store.mode = 3
    await wrapper.find('.band-panel__load').trigger('click')

    expect(store.freqKhz).toBe(1000)
    expect(store.mode).toBe(0)
  })

  it('delete bookmark removes it and persists', async () => {
    const store = useKiwiStore()
    const wrapper = mountPanel()
    await wrapper.find('.k-button').trigger('click') // save
    expect(wrapper.findAll('.band-panel__item').length).toBe(1)

    await wrapper.find('.band-panel__delete').trigger('click')

    expect(wrapper.findAll('.band-panel__item').length).toBe(0)
    expect(localStorage.getItem('netsdrstation.bookmarks.v1')).toBe('[]')
  })
})