import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import Knob from '@/components/Knob.vue'
import MuteButton from '@/components/MuteButton.vue'

describe('Knob', () => {
  it('emits update:value when the slider changes', async () => {
    const wrapper = mount(Knob, {
      props: { label: 'Frequency', value: 440, min: 20, max: 20000, step: 1 },
    })

    const input = wrapper.find('input[type="range"]')
    await input.setValue('1000')

    expect(wrapper.emitted('update:value')).toBeTruthy()
    expect(wrapper.emitted('update:value')![0]).toEqual([1000])
  })

  it('renders its label', () => {
    const wrapper = mount(Knob, {
      props: { label: 'Volume', value: 0.5 },
    })
    expect(wrapper.text()).toContain('Volume')
  })

  it('binds the accessibility label to the label prop', () => {
    const wrapper = mount(Knob, {
      props: { label: 'Frequency', value: 440, min: 20, max: 20000 },
    })
    expect(wrapper.find('input[type="range"]').attributes('aria-label')).toBe('Frequency')
  })
})

describe('MuteButton', () => {
  it('emits toggle when clicked', async () => {
    const wrapper = mount(MuteButton, { props: { active: false } })
    await wrapper.find('button').trigger('click')
    expect(wrapper.emitted('toggle')).toBeTruthy()
  })

  it('reflects the active state', () => {
    const wrapper = mount(MuteButton, { props: { active: true } })
    expect(wrapper.find('button').text()).toContain('Muted')
  })
})
