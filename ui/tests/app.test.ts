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
})
