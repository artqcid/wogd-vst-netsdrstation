import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import Knob from '@/components/Knob.vue'
import MuteButton from '@/components/MuteButton.vue'
import NumberInput from '@/components/NumberInput.vue'
import Toggle from '@/components/Toggle.vue'
import Slider from '@/components/Slider.vue'
import StationInput from '@/components/StationInput.vue'
import StatusBadge from '@/components/StatusBadge.vue'

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

describe('NumberInput', () => {
  it('emits update:value with a number when the input changes', async () => {
    const wrapper = mount(NumberInput, {
      props: { label: 'Frequency', value: 440, min: 20, max: 20000, step: 1 },
    })

    const input = wrapper.find('input[type="number"]')
    await input.setValue('1000')

    expect(wrapper.emitted('update:value')).toBeTruthy()
    expect(wrapper.emitted('update:value')![0]).toEqual([1000])
  })

  it('renders its label', () => {
    const wrapper = mount(NumberInput, {
      props: { label: 'Volume', value: 0.5 },
    })
    expect(wrapper.text()).toContain('Volume')
  })

  it('binds the accessibility label to the label prop', () => {
    const wrapper = mount(NumberInput, {
      props: { label: 'Frequency', value: 440 },
    })
    expect(wrapper.find('label').attributes('aria-label')).toBe('Frequency')
  })

  it('renders suffix span', () => {
    const wrapper = mount(NumberInput, {
      props: { label: 'Test', value: 42, suffix: 'kHz' },
    })
    expect(wrapper.text()).toContain('kHz')
  })
})

describe('Toggle', () => {
  it('emits update:active with the negated boolean when clicked', async () => {
    const wrapper = mount(Toggle, { props: { active: false } })
    await wrapper.find('button').trigger('click')

    expect(wrapper.emitted('update:active')).toBeTruthy()
    expect(wrapper.emitted('update:active')![0]).toEqual([true])
  })

  it('reflects the initial active state in label', () => {
    const wrapper = mount(Toggle, { props: { active: true } })
    expect(wrapper.text()).toContain('On')
  })

  it('reflects the initial inactive state in label', () => {
    const wrapper = mount(Toggle, { props: { active: false } })
    expect(wrapper.text()).toContain('Off')
  })
})

describe('Slider', () => {
  it('emits update:value when the range changes', async () => {
    const wrapper = mount(Slider, {
      props: { label: 'Frequency', value: 440, min: 20, max: 20000, step: 1 },
    })

    const input = wrapper.find('input[type="range"]')
    await input.setValue('1000')

    expect(wrapper.emitted('update:value')).toBeTruthy()
    expect(wrapper.emitted('update:value')![0]).toEqual([1000])
  })

  it('binds the accessibility label to the label prop', () => {
    const wrapper = mount(Slider, {
      props: { label: 'Frequency', value: 440, min: 20, max: 20000 },
    })
    expect(wrapper.find('label').attributes('aria-label')).toBe('Frequency')
  })
})

describe('StationInput', () => {
  it('emits connect with the station string when Connect clicked', async () => {
    const wrapper = mount(StationInput, {
      props: { station: 'g8ure.ddns.net:8078' },
    })

    await wrapper.find('button').trigger('click')
    expect(wrapper.emitted('connect')).toBeTruthy()
    expect(wrapper.emitted('connect')![0]).toEqual(['g8ure.ddns.net:8078'])
  })

  it('emits connect with trimmed station string', async () => {
    const wrapper = mount(StationInput, {
      props: { station: '  g8ure.ddns.net:8078  ' },
    })

    await wrapper.find('button').trigger('click')
    expect(wrapper.emitted('connect')).toBeTruthy()
    expect(wrapper.emitted('connect')![0]).toEqual(['g8ure.ddns.net:8078'])
  })

  it('focus input and press Enter emits connect', async () => {
    const wrapper = mount(StationInput, {
      props: { station: 'g8ure.ddns.net:8078' },
    })

    const input = wrapper.find('input[type="text"]')
    await input.setValue('newstation:8078')
    await input.trigger('keydown.enter')

    expect(wrapper.emitted('connect')).toBeTruthy()
    expect(wrapper.emitted('connect')![0]).toEqual(['newstation:8078'])
  })
})

describe('StatusBadge', () => {
  it('renders the status text', () => {
    const wrapper = mount(StatusBadge, { props: { status: 'Connected' } })
    expect(wrapper.text()).toContain('Connected')
  })

  it('renders status with green dot for connect', () => {
    const wrapper = mount(StatusBadge, { props: { status: 'Connecting' } })
    expect(wrapper.find('.dot').classes('dot--green')).toBe(true)
  })

  it('renders status with grey dot for idle', () => {
    const wrapper = mount(StatusBadge, { props: { status: 'Idle' } })
    expect(wrapper.find('.dot').classes('dot--grey')).toBe(true)
  })
})