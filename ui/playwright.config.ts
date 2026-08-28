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
    // Error-handling artefacts: everything needed to analyse a failure is
    // captured automatically and lands in test-results/ (gitignored).
    screenshot: 'only-on-failure',
    trace: 'retain-on-failure',
    video: 'retain-on-failure',
  },

  // Retry policy: 0 locally (fast feedback), 1 in CI (absorbs flaky tests).
  // A retried test that passes keeps the artefacts of the failed attempt
  // (trace: 'retain-on-failure') for analysis.
  retries: process.env.CI ? 1 : 0,

  // Results for later analysis (all gitignored, kept locally):
  //   - list  : live console output (default)
  //   - html  : interactive report with per-test artefacts (playwright-report/)
  //   - json  : machine-readable summary for tooling (e2e-results/results.json)
  // Playwright prints the report paths at the end of every run.
  reporter: [
    ['list'],
    ['html', { outputFolder: 'playwright-report', open: 'never' }],
    ['json', { outputFile: 'e2e-results/results.json' }],
  ],

  webServer: {
    command: 'npm run dev',
    url: 'http://localhost:5173',
    reuseExistingServer: true,
    timeout: 120_000,
  },
})