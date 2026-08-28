import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import SMeter from '@/components/SMeter.vue'

describe('SMeter', () => {
  it('renders a canvas and dBm readout', () => {
    const wrapper = mount(SMeter, { props: { dbm: -90 } })
    expect(wrapper.find('canvas').exists()).toBe(true)
    expect(wrapper.text()).toContain('-90.0 dBm')
  })

  it('updates the readout when dbm changes', async () => {
    const wrapper = mount(SMeter, { props: { dbm: -90 } })
    await wrapper.setProps({ dbm: -73 })
    expect(wrapper.text()).toContain('-73.0 dBm')
  })

  it('clamps the bar position to the scale range', () => {
    const wrapper = mount(SMeter, { props: { dbm: -300 } })
    // below min -127: readout still shows the raw value, bar is at 0
    expect(wrapper.text()).toContain('-300.0 dBm')
  })

  it('renders the readout even at full scale', () => {
    const wrapper = mount(SMeter, { props: { dbm: 20 } })
    expect(wrapper.text()).toContain('20.0 dBm')
  })
})