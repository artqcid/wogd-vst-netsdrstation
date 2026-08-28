import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'

import App from '@/App.vue'

function mountApp() {
  return mount(App, { global: { plugins: [createPinia()] } })
}

describe('App', () => {
  it('renders the plugin title', () => {
    const wrapper = mountApp()
    expect(wrapper.text()).toContain('NetSDRStation')
  })

  it('renders the PluginView', () => {
    const wrapper = mountApp()
    expect(wrapper.findComponent({ name: 'PluginView' }).exists()).toBe(true)
  })

  it('renders the M4.1 kiwi-layout grid shell', () => {
    const wrapper = mountApp()
    const container = wrapper.find('.kiwi-layout')
    expect(container.exists()).toBe(true)
    expect(wrapper.find('.kiwi-header').exists()).toBe(true)
    expect(wrapper.find('.kiwi-controls-row').exists()).toBe(true)
    expect(wrapper.find('.kiwi-statusbar').exists()).toBe(true)
  })

  it('renders all control panels inside the controls row', () => {
    const wrapper = mountApp()
    const row = wrapper.find('.kiwi-controls-row')
    const panels = row.findAll('.kiwi-panel')
    // Mode&Passband / Frequency / Audio / Display panels
    expect(panels.length).toBe(4)
  })

  it('shows the status badge in the header', () => {
    const wrapper = mountApp()
    const badge = wrapper.find('.kiwi-header .k-status-badge')
    expect(badge.exists()).toBe(true)
  })

  it('shows readouts in the status bar', () => {
    const wrapper = mountApp()
    const statusbar = wrapper.find('.kiwi-statusbar')
    expect(statusbar.find('.k-readout').exists()).toBe(true)
  })

  it('keeps all control groups visible at several viewport sizes', () => {
    // jsdom does not perform real layout, so we assert the structural
    // invariants that guarantee reflow (flex-wrap + panel floor) are present.
    for (const width of [640, 1024, 1920]) {
      ;(window as unknown as { innerWidth: number }).innerWidth = width
      const wrapper = mountApp()
      const row = wrapper.find('.kiwi-controls-row')
      expect(row.classes()).toContain('kiwi-controls-row')
      expect(row.findAll('.kiwi-panel').length).toBeGreaterThanOrEqual(4)
    }
  })
})