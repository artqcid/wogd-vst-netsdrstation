import { describe, it, expect, vi, beforeEach } from 'vitest'

import { pluginService, type PluginMessage } from '@/services/pluginService'

describe('pluginService', () => {
  beforeEach(() => {
    delete (window as unknown as Record<string, unknown>).vstHost
    delete (window as unknown as Record<string, unknown>).updateVueState
  })

  it('isInNative is false when window.vstHost is absent', () => {
    expect(pluginService.isInNative()).toBe(false)
  })

  it('setParameter sends a message via window.vstHost.setParameter', () => {
    const setParameter = vi.fn()
    ;(window as unknown as { vstHost: { setParameter: unknown } }).vstHost = {
      setParameter,
      getParameters: vi.fn(),
    }

    pluginService.setParameter('freq', 440)

    expect(setParameter).toHaveBeenCalledWith('freq', 440)
  })

  it('onMessage registers a callback invoked by window.updateVueState', () => {
    const handler = vi.fn()
    pluginService.onMessage(handler)

    const message: PluginMessage = { type: 'param', data: { id: 'freq', value: 440 } }
    window.updateVueState!(message)

    expect(handler).toHaveBeenCalledWith(message)
  })

  it('setParameter logs in dev mode when window.vstHost is absent', () => {
    const log = vi.spyOn(console, 'log').mockImplementation(() => {})
    pluginService.setParameter('freq', 440)
    expect(log).toHaveBeenCalled()
    log.mockRestore()
  })
})
