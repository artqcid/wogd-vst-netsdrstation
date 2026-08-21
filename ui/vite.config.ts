import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { viteSingleFile } from 'vite-plugin-singlefile'

// https://vite.dev/config/
export default defineConfig({
  // Relative asset paths so the built bundle works when served from a
  // file:// URL in the release plugin (see source/editor/plugin_editor.cpp).
  base: './',
  plugins: [
    vue(),
    // Inline JS/CSS into index.html: external ES-module scripts are blocked
    // by CORS under file:// (opaque origin), which left the plugin GUI blank.
    viteSingleFile(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
})
