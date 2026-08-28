import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'

import App from '@/App.vue'

function mountApp() {
  return mount(App, { global: { plugins: [createPinia()] } })
}

describe('App', () => {
  it('renders the plugin title in the topbar', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-topbar__title').text()).toContain('NetSDRStation')
  })

  it('renders the PluginView', () => {
    const wrapper = mountApp()
    expect(wrapper.findComponent({ name: 'PluginView' }).exists()).toBe(true)
  })

  it('renders the KiwiSDR layout regions (topbar / tuning / main / statusbar)', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-topbar').exists()).toBe(true)
    expect(wrapper.find('.kiwi-tuning').exists()).toBe(true)
    expect(wrapper.find('.kiwi-main').exists()).toBe(true)
    expect(wrapper.find('.kiwi-statusbar').exists()).toBe(true)
  })

  it('renders mode + band tags in the tuning area (above the waterfall)', () => {
    const wrapper = mountApp()
    const tuning = wrapper.find('.kiwi-tuning')
    expect(tuning.find('[data-testid="mode-panel"]').exists()).toBe(true)
    expect(tuning.find('[data-testid="band-panel"]').exists()).toBe(true)
  })

  it('renders the full-bleed waterfall and the floating control panel', () => {
    const wrapper = mountApp()
    const main = wrapper.find('.kiwi-main')
    expect(main.find('[data-testid="waterfall"]').exists()).toBe(true)
    const control = main.find('.kiwi-control-panel')
    expect(control.exists()).toBe(true)
    expect(control.find('[data-testid="freq-panel"]').exists()).toBe(true)
    expect(control.find('[data-testid="audio-panel"]').exists()).toBe(true)
    expect(control.find('[data-testid="waterfall-panel"]').exists()).toBe(true)
    expect(control.find('[data-testid="extension-panel"]').exists()).toBe(true)
  })

  it('shows the status badge in the topbar', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-topbar .k-status-badge').exists()).toBe(true)
  })

  it('shows the S-meter in the status bar', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-statusbar .s-meter').exists()).toBe(true)
  })

  it('renders the station input in the topbar', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-topbar [data-testid="station-input"]').exists()).toBe(true)
  })
})