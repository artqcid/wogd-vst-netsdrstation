import { defineConfig } from '@playwright/test'

export default defineConfig({
  testDir: './e2e',
  fullyParallel: false,
  timeout: 30_000,

  use: {
    baseURL: 'http://localhost:5173',
    headless: true,
    viewport: { width: 800, height: 600 },
    actionTimeout: 5_000,
    navigationTimeout: 20_000,
  },

  webServer: {
    command: 'npm run dev',
    url: 'http://localhost:5173',
    reuseExistingServer: true,
    timeout: 120_000,
    // Starts from the project root, so we need to set the working directory
    // or rely on the test running from the ui directory.
    // Since the config is in ui/, the command runs from ui/ automatically.
  },
})