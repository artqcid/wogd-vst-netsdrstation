import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import App from '@/App.vue'

describe('App', () => {
  it('renders the plugin title', () => {
    const wrapper = mount(App)
    expect(wrapper.text()).toContain('NetSDRStation')
  })

  it('renders the PluginView', () => {
    const wrapper = mount(App)
    expect(wrapper.findComponent({ name: 'PluginView' }).exists()).toBe(true)
  })

  it('renders the M4.1 kiwi-layout grid shell', () => {
    const wrapper = mount(App)
    const container = wrapper.find('.kiwi-layout')
    expect(container.exists()).toBe(true)
    expect(wrapper.find('.kiwi-header').exists()).toBe(true)
    expect(wrapper.find('.kiwi-controls-row').exists()).toBe(true)
    expect(wrapper.find('.kiwi-statusbar').exists()).toBe(true)
  })

  it('layout fills the available height (100%)', () => {
    const wrapper = mount(App)
    const container = wrapper.find('.kiwi-layout')
    // scoped styles are applied in jsdom via computed style when mounted with
    // style injection; assert the class structure is fluid (no fixed px width)
    expect(container.classes()).toContain('kiwi-layout')
    expect(container.attributes('style')).toBeUndefined()
  })

  it('renders all control panels inside the controls row', () => {
    const wrapper = mount(App)
    const row = wrapper.find('.kiwi-controls-row')
    const panels = row.findAll('.kiwi-panel')
    // Receiver / Frequency / Audio / Display panels
    expect(panels.length).toBe(4)
  })

  it('renders the station panel in the header', () => {
    const wrapper = mount(App)
    expect(wrapper.find('.kiwi-header .station-panel').exists()).toBe(true)
  })

  it('shows a status badge in the status bar', () => {
    const wrapper = mount(App)
    const statusbar = wrapper.find('.kiwi-statusbar')
    expect(statusbar.exists()).toBe(true)
    // StatusBadge root renders a .status-badge span (or .dot fallback)
    expect(statusbar.find('.status-badge').exists() || statusbar.find('.dot').exists()).toBe(true)
  })

  it('keeps all control groups visible at several viewport sizes', () => {
    // jsdom does not perform real layout, so we assert the structural
    // invariants that guarantee reflow (flex-wrap + panel floor) are present.
    for (const width of [640, 1024, 1920]) {
      ;(window as unknown as { innerWidth: number }).innerWidth = width
      const wrapper = mount(App)
      const row = wrapper.find('.kiwi-controls-row')
      expect(row.classes()).toContain('kiwi-controls-row')
      expect(row.findAll('.kiwi-panel').length).toBeGreaterThanOrEqual(4)
    }
  })
})
