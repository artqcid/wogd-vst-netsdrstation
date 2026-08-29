import { test, expect } from '@playwright/test'

test.describe('Waterfall', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('No-signal overlay visible when disconnected (Bug 2)', async ({ page }) => {
    await expect(page.locator('.waterfall__no-signal')).toBeVisible()
    await expect(page.locator('.waterfall__no-signal')).toContainText(/No signal|connect to a KiwiSDR station/)
  })

  test('Waterfall canvas is rendered', async ({ page }) => {
    await expect(page.locator('.waterfall canvas')).toBeVisible()
  })

  // Skipping: No-signal overlay disappears when bins are present.
  // This requires a real WebSocket connection (or mocking) to populate bins
  // and is not feasible in a plain E2E test environment.
  test.skip('No-signal overlay disappears when bins are present', async ({ page }) => {
    // Requires real WebSocket feed or bin-mock injection — skip in E2E.
  })
})
