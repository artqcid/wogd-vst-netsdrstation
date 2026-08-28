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
])

export const useKiwiStore = defineStore('kiwi', {
  state: () => ({
    station: '',
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
    signalLevel: -140, // dBm, display only
    userCount: '?', // display only
    gpsSync: false, // display only
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
      this.station = hostPort
      this.connected = false
      pluginService.setStation(hostPort)
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
  },
})