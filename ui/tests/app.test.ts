import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import { defineComponent } from 'vue'

import App from '@/App.vue'

// Stub canvas-based components that crash in jsdom (no canvas support)
const WaterfallStub = defineComponent({ name: 'Waterfall', template: '<div data-testid="waterfall" />' })
const SMeterStub = defineComponent({ name: 'SMeter', template: '<div class="s-meter" />' })

function mountApp() {
  return mount(App, {
    global: {
      plugins: [createPinia()],
      stubs: { Waterfall: WaterfallStub, SMeter: SMeterStub },
    },
  })
}

describe('App', () => {
  it('renders the plugin title in the topbar', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-header__title').text()).toContain('NetSDRStation')
  })

  it('renders the PluginView', () => {
    const wrapper = mountApp()
    expect(wrapper.findComponent({ name: 'PluginView' }).exists()).toBe(true)
  })

  it('renders the KiwiSDR layout regions', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-header').exists()).toBe(true)
    expect(wrapper.find('.band-scale').exists()).toBe(true)
    expect(wrapper.find('.tag-area').exists()).toBe(true)
    expect(wrapper.find('.kiwi-main').exists()).toBe(true)
  })

  it('renders band scale and tag area above the main workspace', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.band-scale').exists()).toBe(true)
    expect(wrapper.find('.tag-area').exists()).toBe(true)
  })

  it('renders the waterfall in the main area', () => {
    const wrapper = mountApp()
    const main = wrapper.find('.kiwi-main')
    expect(main.find('[data-testid="waterfall"]').exists()).toBe(true)
  })

  it('renders the floating control panel', () => {
    const wrapper = mountApp()
    const canvas = wrapper.find('.kiwi-canvas-area')
    expect(canvas.find('.kiwi-cpanel').exists()).toBe(true)
  })

  it('shows the S-meter in the control panel', () => {
    const wrapper = mountApp()
    expect(wrapper.find('.kiwi-cpanel__smeter').exists()).toBe(true)
  })

  it('renders the station input in the header', () => {
    const wrapper = mountApp()
    expect(wrapper.findComponent({ name: 'StationInput' }).exists()).toBe(true)
  })
})