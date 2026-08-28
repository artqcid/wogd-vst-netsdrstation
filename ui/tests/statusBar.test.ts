import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia, type Pinia } from 'pinia'

import StatusBar from '@/components/StatusBar.vue'
import { useKiwiStore } from '@/store/kiwiStore'

let pinia: Pinia

function mountBar() {
  return mount(StatusBar, { global: { plugins: [pinia] } })
}

describe('StatusBar', () => {
  beforeEach(() => {
    pinia = createPinia()
    setActivePinia(pinia)
  })

  it('shows the S-meter', () => {
    const wrapper = mountBar()
    expect(wrapper.find('.s-meter').exists()).toBe(true)
  })

  it('shows the user count from the store', () => {
    const store = useKiwiStore()
    store.userCount = '3/12'
    const wrapper = mountBar()
    expect(wrapper.find('[data-testid="status-users"]').text()).toContain('3/12')
  })

  it('shows GPS sync state', () => {
    const store = useKiwiStore()
    store.gpsSync = true
    const wrapper = mountBar()
    expect(wrapper.find('[data-testid="status-gps"]').text()).toContain('✓')
  })

  it('formats the frequency with 3 decimals', () => {
    const store = useKiwiStore()
    store.freqKhz = 14100
    const wrapper = mountBar()
    expect(wrapper.find('[data-testid="status-freq"]').text()).toContain('14100.000 kHz')
  })

  it('buffer shows OK when connected and audio flowing', () => {
    const store = useKiwiStore()
    store.connected = true
    store.signalLevel = -80
    const wrapper = mountBar()
    expect(wrapper.find('[data-testid="status-buffer"]').text()).toContain('OK')
  })

  it('buffer shows idle when connected but silent', () => {
    const store = useKiwiStore()
    store.connected = true
    store.signalLevel = -140
    const wrapper = mountBar()
    expect(wrapper.find('[data-testid="status-buffer"]').text()).toContain('idle')
  })
})