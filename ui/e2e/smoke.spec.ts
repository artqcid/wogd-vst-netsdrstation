import { test, expect } from '@playwright/test'

test('smoke: page loads and all M4 panels are present', async ({ page }) => {
  await page.goto('/')

  // Header: title + station input + status badge.
  await expect(page.getByRole('heading', { name: /NetSDRStation/ })).toBeVisible()
  await expect(page.getByTestId('station-input')).toBeVisible()

  // Control panels.
  for (const id of ['mode-panel', 'freq-panel', 'band-panel', 'audio-panel', 'extension-panel', 'waterfall-panel']) {
    await expect(page.getByTestId(id)).toBeVisible()
  }

  // Status bar with S-meter + readouts.
  await expect(page.getByTestId('status-bar')).toBeVisible()
  await expect(page.getByTestId('status-bar').locator('.s-meter')).toBeVisible()
  await expect(page.getByTestId('status-freq')).toContainText('kHz')
})