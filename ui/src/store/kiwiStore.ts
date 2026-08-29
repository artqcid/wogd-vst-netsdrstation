/**
 * Central reactive state store (M4.2c, Pinia).
 *
 * Single source of truth for all KiwiSDR UI state. Every parameter change is
 * routed through `setParam`, which forwards to the C++ bridge
 * (pluginService.setParameter) and applies an optimistic update locally.
 * Backend -> UI updates arrive via pluginService.onMessage and mutate the
 * store directly (see PluginView.vue onMounted).
 */
import { defineStore } from 'pinia'
import type { ParamId } from '@/generated/bridge-validators'
import { pluginService } from '@/services/pluginService'

/** Param IDs that are booleans in the store (bridge sends 0/1). */
const BOOLEAN_PARAMS: ReadonlySet<ParamId> = new Set<ParamId>([
  'agcOn',
  'agcHang',
  'mute',
  'squelchOn',
  'nbOn',
  'nrOn',
  'cwPeaks',
])

/** Default station preloaded in the UI (until the M5 station tab lands). */
export const DEFAULT_STATION = 'kphsdr.com:8072'

/** Minimal validation: "host" or "host:port", no whitespace/empty. */
export function isValidStation(hostPort: string): boolean {
  const trimmed = hostPort.trim()
  if (!trimmed) return false
  return /^[^\s:]+(:\d{1,5})?$/.test(trimmed)
}

export const useKiwiStore = defineStore('kiwi', {
  state: () => ({
    station: DEFAULT_STATION,
    connected: false,
    status: 'Idle',
    freqKhz: 14100.0,
    mode: 0, // 0=AM ... 17=QAM (KiwiSDR numeric mode index)
    lowCut: -4900,
    highCut: 4900,
    agcOn: true,
    agcThresh: -100,
    agcDecay: 1000,
    agcHang: false,
    agcSlope: 0,
    agcManGain: 0,
    volume: 1.0,
    mute: false,
    squelchOn: false,
    squelchThr: 0.0,
    nbOn: false,
    nbThresh: 0.5,
    nrOn: false,
    wfOn: true,
    wfSpeed: 2,
    wfZoom: 0,
    wfMaxDb: -30,
    wfMinDb: -130,
    wfComp: false,
    // Waterfall display state (M4.7, UI-local, not bridged parameters).
    colorMap: 'default' as 'default' | 'rain' | 'grayscale',
    displayMode: 'waterfall' as 'waterfall' | 'spectrum' | 'both',
    waterfallBins: [] as number[],
    signalLevel: -140, // dBm, display only
    userCount: '?', // display only
    gpsSync: false, // display only
    // M4c.7 — UI-local state (Bug 1: Peak Hold, Bug 6.6: Spectrum Mode, Bug 6.8: RF)
    specPeak1: false,
    specPeak2: false,
    spectrumMode: 'waterfall' as 'waterfall' | 'specRF' | 'specAF',
    rfAttn: 0 as 0 | -10 | -20 | -30 | -40,
    cwPeaks: false,
    activeExtension: '',
  }),

  getters: {
    /** Display text for the status badge. */
    statusText: state => state.status,
    /** Colour state for the status badge. */
    statusState: state => {
      if (/connect/i.test(state.status)) return 'ok' as const
      if (/error|disconnect/i.test(state.status)) return 'error' as const
      return 'warn' as const
    },
  },

  actions: {
    /**
     * Sends a parameter change to the bridge and applies an optimistic
     * update. `name` must be a ParamId literal (validated by the schema).
     */
    setParam(name: ParamId, value: number) {
      pluginService.setParameter(name, value)
      // optimistic update (bools arrive as 0/1 numbers -> store keeps boolean)
      this.setLocal(name, value)
    },

    /** Replaces a whole parameter group from a backend param message. */
    applyParam(id: ParamId, value: number) {
      this.setLocal(id, value)
    },

    /** Writes a param value into the state, normalising booleans. */
    setLocal(name: ParamId, value: number) {
      const target = this as unknown as Record<string, unknown>
      target[name] = BOOLEAN_PARAMS.has(name) ? value > 0.5 : value
    },

    setStation(hostPort: string) {
      const trimmed = hostPort.trim()
      if (!trimmed) {
        this.status = 'Error: enter a station'
        return
      }
      if (!isValidStation(trimmed)) {
        this.status = 'Error: invalid station'
        return
      }
      this.station = trimmed
      this.connected = false
      this.status = 'Connecting...'
      pluginService.setStation(trimmed)
    },

    disconnect() {
      this.connected = false
      pluginService.disconnect()
    },

    setConnected(connected: boolean) {
      this.connected = connected
    },

    setStatus(status: string) {
      this.status = status
      this.connected = /connected/i.test(status)
    },

    setSignalLevel(dbm: number) {
      this.signalLevel = dbm
    },

    setWaterfallBins(bins: number[]) {
      this.waterfallBins = bins
    },
  },
})