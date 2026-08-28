import { test, expect } from '@playwright/test'

test('smoke: page loads and all KiwiSDR regions are present', async ({ page }) => {
  await page.goto('/')

  // Topbar: title + station input + status badge.
  await expect(page.locator('.kiwi-topbar__title')).toContainText('NetSDRStation')
  await expect(page.getByTestId('station-input')).toBeVisible()

  // Tuning area (mode buttons + band tags stacked above the waterfall).
  await expect(page.getByTestId('mode-panel')).toBeVisible()
  await expect(page.getByTestId('band-panel')).toBeVisible()

  // Full-bleed waterfall + floating control panel on the right.
  await expect(page.getByTestId('waterfall')).toBeVisible()
  const control = page.locator('.kiwi-control-panel')
  await expect(control).toBeVisible()
  for (const id of ['freq-panel', 'audio-panel', 'waterfall-panel', 'extension-panel']) {
    await expect(control.getByTestId(id)).toBeVisible()
  }

  // Status bar with S-meter + readouts.
  await expect(page.getByTestId('status-bar')).toBeVisible()
  await expect(page.getByTestId('status-bar').locator('.s-meter')).toBeVisible()
  await expect(page.getByTestId('status-freq')).toContainText('kHz')
})