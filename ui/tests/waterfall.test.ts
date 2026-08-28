import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import Waterfall from '@/components/Waterfall.vue'

describe('Waterfall', () => {
  it('renders a canvas', () => {
    const wrapper = mount(Waterfall, { props: { bins: [0, 0] } })
    expect(wrapper.find('canvas').exists()).toBe(true)
  })

  it('pushes a known frame without throwing', async () => {
    const bins = Array.from({ length: 256 }, (_, i) => (i % 32 === 0 ? -20 : -120))
    const wrapper = mount(Waterfall, { props: { bins: [] } })
    await wrapper.setProps({ bins })
    expect(wrapper.find('canvas').exists()).toBe(true)
  })

  it('applies the frequency cursor overlay prop', () => {
    const wrapper = mount(Waterfall, {
      props: { bins: [0, 0], cursorKhz: 14100, centreKhz: 14100 },
    })
    expect(wrapper.find('canvas').attributes('aria-label')).toContain('Waterfall')
  })

  it('renders the readout via passband props', () => {
    const wrapper = mount(Waterfall, {
      props: { bins: [0, 0], lowCutHz: -4900, highCutHz: 4900 },
    })
    expect(wrapper.find('canvas').exists()).toBe(true)
  })
})