/**
 * Bridge between the Vue UI and the C++ plugin (webview/webview, no JUCE).
 *
 * Communication follows the architecture (doc/architecture.md §5):
 *   - UI -> DSP:  window.vstHost.setParameter(...)  (JS -> C++ binding)
 *   - DSP -> UI:  window.updateVueState(...)        (C++ eval -> JS callback)
 *
 * In browser dev mode (Vite dev server) there is no native bridge, so calls
 * are logged instead of sent, and no global handler is installed.
 */

export interface PluginMessage {
  type: string
  data?: unknown
}

export interface VstHost {
  setParameter(id: string, value: number): void
  getParameters(): void
}

declare global {
  interface Window {
    vstHost?: VstHost
    updateVueState?: (message: PluginMessage) => void
  }
}

type MessageHandler = (message: PluginMessage) => void

class PluginService {
  private messageHandler: MessageHandler | null = null

  /** True when running inside the native WebView (window.vstHost present). */
  isInNative(): boolean {
    return typeof window.vstHost !== 'undefined'
  }

  /**
   * Sends a parameter change to the plugin. No-op (logged) in dev mode.
   */
  setParameter(id: string, value: number): void {
    if (this.isInNative()) {
      window.vstHost!.setParameter(id, value)
    } else {
      console.log('[Dev Mode] setParameter', id, value)
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
   */
  onMessage(handler: MessageHandler): void {
    this.messageHandler = handler
    window.updateVueState = (message: PluginMessage) => {
      this.messageHandler?.(message)
    }
  }
}

export const pluginService = new PluginService()
