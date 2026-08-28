import { describe, it, expect, vi } from 'vitest'

import { pluginService, type BackendMessage } from '@/services/pluginService'

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

    pluginService.setParameter('freqKhz', 440)

    expect(setParameter).toHaveBeenCalledWith('freqKhz', 440)
  })

  it('onMessage registers a callback invoked by window.updateVueState', () => {
    const handler = vi.fn()
    pluginService.onMessage(handler)

    const message: BackendMessage = { type: 'param', data: { id: 'freqKhz', value: 440 } }
    window.updateVueState!(message)

    expect(handler).toHaveBeenCalledWith(message)
  })

  it('onMessage rejects messages that do not match the bridge schema', () => {
    const handler = vi.fn()
    const warn = vi.spyOn(console, 'warn').mockImplementation(() => {})
    pluginService.onMessage(handler)

    // data.value is a string -> violates ParamUpdateMessage (id: ParamId, value: number)
    window.updateVueState!({ type: 'param', data: { id: 'freqKhz', value: '440' } } as never)

    expect(handler).not.toHaveBeenCalled()
    expect(warn).toHaveBeenCalled()
    warn.mockRestore()
  })

  it('onMessage rejects unknown message types', () => {
    const handler = vi.fn()
    const warn = vi.spyOn(console, 'warn').mockImplementation(() => {})
    pluginService.onMessage(handler)

    window.updateVueState!({ type: 'nope', data: null } as never)

    expect(handler).not.toHaveBeenCalled()
    expect(warn).toHaveBeenCalled()
    warn.mockRestore()
  })

  it('onLevel registers window.setLevel invoked by the backend', () => {
    const handler = vi.fn()
    pluginService.onLevel(handler)

    window.setLevel!(-73.5)

    expect(handler).toHaveBeenCalledWith(-73.5)
  })

  it('setParameter logs in dev mode when window.vstHost is absent', () => {
    const log = vi.spyOn(console, 'log').mockImplementation(() => {})
    pluginService.setParameter('freqKhz', 440)
    expect(log).toHaveBeenCalled()
    log.mockRestore()
  })

  it('setStation calls window.vstHost.setStation in native mode', () => {
    const setStation = vi.fn()
    ;(window as unknown as { vstHost: { setStation: unknown } }).vstHost = {
      setStation,
    }

    pluginService.setStation('host:port')

    expect(setStation).toHaveBeenCalledWith('host:port')
  })

  it('setStation logs in dev mode when window.vstHost is absent', () => {
    const log = vi.spyOn(console, 'log').mockImplementation(() => {})
    pluginService.setStation('host:port')
    expect(log).toHaveBeenCalled()
    log.mockRestore()
  })
})