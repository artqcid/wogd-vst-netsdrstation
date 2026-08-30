import { describe, it, expect, vi, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'

import { useKiwiStore, isValidStation, DEFAULT_STATION } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'

describe('kiwiStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    delete (window as unknown as Record<string, unknown>).vstHost
  })

  it('preloads the default station', () => {
    const store = useKiwiStore()
    expect(store.station).toBe(DEFAULT_STATION)
    expect(DEFAULT_STATION).toBe('kphsdr.com:8073')
  })

  it('setParam forwards to pluginService and applies an optimistic update', () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setParameter')

    store.setParam('freqKhz', 14100.5)

    expect(spy).toHaveBeenCalledWith('freqKhz', 14100.5)
    expect(store.freqKhz).toBe(14100.5)
  })

  it('setStation updates station and calls pluginService.setStation', () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setStation')

    store.setStation('kphsdr.com:8072')

    expect(spy).toHaveBeenCalledWith('kphsdr.com:8072')
    expect(store.station).toBe('kphsdr.com:8072')
    expect(store.connected).toBe(false)
  })

  it('setStation with empty string sets error status and does NOT bridge', () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setStation')

    store.setStation('   ')

    expect(spy).not.toHaveBeenCalled()
    expect(store.status).toContain('Error')
  })

  it('setStation with an invalid station sets error status and does NOT bridge', () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'setStation')

    store.setStation('no port and spaces here')

    expect(spy).not.toHaveBeenCalled()
    expect(store.status).toContain('Error')
  })

  it('isValidStation accepts host and host:port, rejects empty/garbage', () => {
    expect(isValidStation('kphsdr.com:8072')).toBe(true)
    expect(isValidStation('localhost')).toBe(true)
    expect(isValidStation('g8ure.ddns.net:8075')).toBe(true)
    expect(isValidStation('')).toBe(false)
    expect(isValidStation('   ')).toBe(false)
    expect(isValidStation('host with spaces')).toBe(false)
    expect(isValidStation('host:')).toBe(false)
    expect(isValidStation('host:999999')).toBe(false) // port > 5 digits
  })

  it('disconnect calls pluginService.disconnect', () => {
    const store = useKiwiStore()
    const spy = vi.spyOn(pluginService, 'disconnect')

    store.disconnect()

    expect(spy).toHaveBeenCalled()
  })

  it('applyParam updates the matching state key', () => {
    const store = useKiwiStore()
    store.applyParam('volume', 0.25)
    expect(store.volume).toBe(0.25)
  })

  it('setStatus updates status and connected flag', () => {
    const store = useKiwiStore()
    store.setStatus('Connected')
    expect(store.connected).toBe(true)
    expect(store.statusText).toBe('Connected')
    expect(store.statusState).toBe('ok')
  })

  it('setStatus maps error text to error state', () => {
    const store = useKiwiStore()
    store.setStatus('Error: timeout')
    expect(store.connected).toBe(false)
    expect(store.statusState).toBe('error')
  })
})