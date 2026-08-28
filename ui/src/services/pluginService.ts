/**
 * Bridge between the Vue UI and the C++ plugin (webview/webview, no JUCE).
 *
 * Communication follows the architecture (doc/architecture.md §5):
 *   - UI -> DSP:  window.vstHost.setParameter(...)  (JS -> C++ binding)
 *   - DSP -> UI:  window.updateVueState(...)        (C++ eval -> JS callback)
 *
 * Types and runtime validators are GENERATED / mirrored from
 * schema/bridge.schema.json (the single source of truth, M4.1.5):
 *   - TypeScript types:      src/generated/bridge.ts       (json2ts)
 *   - Zod validators:        src/generated/bridge-validators.ts
 *
 * In browser dev mode (Vite dev server) there is no native bridge, so calls
 * are logged instead of sent, and no global handler is installed.
 */

import {
  BackendMessageSchema,
  type BackendMessage,
  type ParamId,
} from '@/generated/bridge-validators'

export type { BackendMessage }

export interface VstHost {
  setParameter(id: string, value: number): void
  getParameters(): void
  setStation(hostPort: string): void
  disconnect(): void
}

declare global {
  interface Window {
    vstHost?: VstHost
    updateVueState?: (message: BackendMessage) => void
    setLevel?: (dbm: number) => void
  }
}

type MessageHandler = (message: BackendMessage) => void
type LevelHandler = (dbm: number) => void

class PluginService {
  private messageHandler: MessageHandler | null = null
  private levelHandler: LevelHandler | null = null

  /** True when running inside the native WebView (window.vstHost present). */
  isInNative(): boolean {
    return typeof window.vstHost !== 'undefined'
  }

  /**
   * Sends a parameter change to the plugin. No-op (logged) in dev mode.
   * `id` is a ParamId literal, validated against the bridge schema.
   */
  setParameter(id: ParamId, value: number): void {
    if (this.isInNative()) {
      window.vstHost!.setParameter(id, value)
    } else {
      console.log('[Dev Mode] setParameter', id, value)
    }
  }

  /**
   * Sends a station host:port to the plugin. No-op (logged) in dev mode.
   */
  setStation(hostPort: string): void {
    if (this.isInNative()) {
      window.vstHost!.setStation(hostPort)
    } else {
      console.log('[Dev Mode] setStation', hostPort)
    }
  }

  /**
   * Requests the plugin to disconnect from the current station. No-op (logged)
   * in dev mode.
   */
  disconnect(): void {
    if (this.isInNative()) {
      window.vstHost!.disconnect()
    } else {
      console.log('[Dev Mode] disconnect')
    }
  }

  /** Requests the current parameter values from the plugin. */
  getParameters(): void {
    if (this.isInNative()) {
      window.vstHost!.getParameters()
    }
  }

  /**
   * Registers a callback for plugin -> UI messages. Exposes
   * window.updateVueState, which the C++ side invokes via eval().
   * Incoming messages are validated against the bridge schema (Zod).
   */
  onMessage(handler: MessageHandler): void {
    this.messageHandler = handler
    window.updateVueState = (message: BackendMessage) => {
      const parsed = BackendMessageSchema.safeParse(message)
      if (!parsed.success) {
        console.warn('[Bridge] rejected message not matching bridge.schema.json:', message)
        return
      }
      this.messageHandler?.(parsed.data)
    }
  }

  /**
   * Registers a callback for the C++ -> UI S-meter level (dBm). Exposes
   * window.setLevel, which the editor invokes via eval() at ~10 Hz.
   */
  onLevel(handler: LevelHandler): void {
    this.levelHandler = handler
    window.setLevel = (dbm: number) => {
      this.levelHandler?.(dbm)
    }
  }
}

export const pluginService = new PluginService()