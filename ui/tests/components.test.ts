import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import KSlider from '@/components/KSlider.vue'
import KNumberInput from '@/components/KNumberInput.vue'
import KSelect from '@/components/KSelect.vue'
import KToggle from '@/components/KToggle.vue'
import KButton from '@/components/KButton.vue'
import KReadout from '@/components/KReadout.vue'
import KPanel from '@/components/KPanel.vue'
import KStatusBadge from '@/components/KStatusBadge.vue'

describe('KSlider', () => {
  it('emits update:modelValue when the range changes', async () => {
    const wrapper = mount(KSlider, {
      props: { modelValue: 0.5, min: 0, max: 1, step: 0.01 },
    })

    const input = wrapper.find('input[type="range"]')
    await input.setValue('0.75')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')![0]).toEqual([0.75])
  })

  it('binds the accessibility label', () => {
    const wrapper = mount(KSlider, { props: { modelValue: 0.5, label: 'Volume' } })
    expect(wrapper.find('input[type="range"]').attributes('aria-label')).toBe('Volume')
  })

  it('renders its unit readout', () => {
    const wrapper = mount(KSlider, { props: { modelValue: 0.5, unit: '%' } })
    expect(wrapper.text()).toContain('%')
  })
})

describe('KNumberInput', () => {
  it('emits update:modelValue with a number on input', async () => {
    const wrapper = mount(KNumberInput, {
      props: { modelValue: 440, min: 0, max: 20000, step: 1 },
    })

    const input = wrapper.find('input[type="number"]')
    await input.setValue('1000')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')![0]).toEqual([1000])
  })

  it('emits update:modelValue on arrow-up key with step', async () => {
    const wrapper = mount(KNumberInput, {
      props: { modelValue: 100, min: 0, max: 20000, step: 10 },
    })

    await wrapper.find('input[type="number"]').trigger('keydown.up')

    expect(wrapper.emitted('update:modelValue')![0]).toEqual([110])
  })

  it('clamps arrow-key increments to max', async () => {
    const wrapper = mount(KNumberInput, {
      props: { modelValue: 19995, min: 0, max: 20000, step: 10 },
    })

    await wrapper.find('input[type="number"]').trigger('keydown.up')

    expect(wrapper.emitted('update:modelValue')![0]).toEqual([20000])
  })

  it('renders its suffix', () => {
    const wrapper = mount(KNumberInput, { props: { modelValue: 42, unit: 'kHz' } })
    expect(wrapper.text()).toContain('kHz')
  })
})

describe('KSelect', () => {
  const options = [
    { value: 0, label: 'AM' },
    { value: 3, label: 'USB' },
  ]

  it('renders all options', () => {
    const wrapper = mount(KSelect, { props: { modelValue: 0, options } })
    const opts = wrapper.findAll('option')
    expect(opts.length).toBe(2)
    expect(opts[1].text()).toBe('USB')
  })

  it('emits update:modelValue on change', async () => {
    const wrapper = mount(KSelect, { props: { modelValue: 0, options } })
    await wrapper.find('select').setValue('3')
    expect(wrapper.emitted('update:modelValue')![0]).toEqual(['3'])
  })
})

describe('KToggle', () => {
  it('emits the negated value when clicked', async () => {
    const wrapper = mount(KToggle, { props: { modelValue: false, label: 'AGC' } })
    await wrapper.find('button').trigger('click')
    expect(wrapper.emitted('update:modelValue')![0]).toEqual([true])
  })

  it('reflects active state via aria-pressed', () => {
    const wrapper = mount(KToggle, { props: { modelValue: true, label: 'AGC' } })
    expect(wrapper.find('button').attributes('aria-pressed')).toBe('true')
  })
})

describe('KButton', () => {
  it('emits click when clicked', async () => {
    const wrapper = mount(KButton, { props: { label: 'AM' } })
    await wrapper.find('button').trigger('click')
    expect(wrapper.emitted('click')).toBeTruthy()
  })

  it('applies the active modifier class', () => {
    const wrapper = mount(KButton, { props: { label: 'AM', active: true } })
    expect(wrapper.find('button').classes()).toContain('k-button--active')
  })
})

describe('KReadout', () => {
  it('formats numeric values with fixed digits', () => {
    const wrapper = mount(KReadout, { props: { value: 14100.0, digits: 3, unit: 'kHz' } })
    expect(wrapper.text()).toContain('14100.000')
    expect(wrapper.text()).toContain('kHz')
  })

  it('renders string values verbatim', () => {
    const wrapper = mount(KReadout, { props: { value: 'Connected' } })
    expect(wrapper.text()).toContain('Connected')
  })
})

describe('KPanel', () => {
  it('renders its title and slot content', () => {
    const wrapper = mount(KPanel, {
      props: { title: 'Audio' },
      slots: { default: '<span>content</span>' },
    })
    expect(wrapper.text()).toContain('Audio')
    expect(wrapper.text()).toContain('content')
  })
})

describe('KStatusBadge', () => {
  it('renders label and ok state', () => {
    const wrapper = mount(KStatusBadge, { props: { state: 'ok', label: 'Connected' } })
    expect(wrapper.text()).toContain('Connected')
    expect(wrapper.classes()).toContain('k-status-badge--ok')
  })

  it('applies error state class', () => {
    const wrapper = mount(KStatusBadge, { props: { state: 'error', label: 'Error' } })
    expect(wrapper.classes()).toContain('k-status-badge--error')
  })
})