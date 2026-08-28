import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'

import ExtensionPanel from '@/components/ExtensionPanel.vue'

function mountPanel() {
  return mount(ExtensionPanel)
}

describe('ExtensionPanel', () => {
  it('renders the extension dropdown', () => {
    const wrapper = mountPanel()
    const select = wrapper.find('select')
    expect(select.exists()).toBe(true)
    expect(wrapper.findAll('option').length).toBe(7)
  })

  it('shows the CW decoder panel by default', () => {
    const wrapper = mountPanel()
    expect(wrapper.find('[data-testid="ext-cw"]').exists()).toBe(true)
  })

  it('switches the displayed panel on selection', async () => {
    const wrapper = mountPanel()
    await wrapper.find('select').setValue('wfax')
    expect(wrapper.find('[data-testid="ext-wfax"]').exists()).toBe(true)
    expect(wrapper.find('[data-testid="ext-cw"]').exists()).toBe(false)
  })

  it('switches to the IQ panel', async () => {
    const wrapper = mountPanel()
    await wrapper.find('select').setValue('iq')
    expect(wrapper.find('[data-testid="ext-iq"]').exists()).toBe(true)
  })

  it('switches to the antenna switch panel', async () => {
    const wrapper = mountPanel()
    await wrapper.find('select').setValue('antenna')
    expect(wrapper.find('[data-testid="ext-antenna"]').exists()).toBe(true)
  })
})