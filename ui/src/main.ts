import './assets/kiwi-theme.css'
import './assets/master.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import { useKiwiStore } from './store/kiwiStore'
import type { ParamId } from '@/generated/bridge-validators'

declare global {
  interface Window {
    __vueStore?: {
      freqKhz: number
      panOffsetKhz: number
      lowCut: number
      highCut: number
      status: string
      setParam: (name: ParamId, value: number) => void
    }
  }
}

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)

// Expose the store for E2E tests (playwright). The store instance must be
// created AFTER pinia is installed (useKiwiStore needs an active pinia).
// Guarded so it never breaks in production.
if (import.meta.env.DEV) {
  window.__vueStore = useKiwiStore(pinia)
}

app.mount('#app')
