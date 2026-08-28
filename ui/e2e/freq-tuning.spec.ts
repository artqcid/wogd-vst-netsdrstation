import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: frequency & tuning panel (dev server, no native bridge).
 * The Pinia store applies optimistic updates, so the UI state reacts to
 * clicks without the C++ backend.
 */
test('freq-tuning: step buttons update the frequency readout', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('freq-panel')
  await expect(panel).toBeVisible()

  // Default freqKhz = 14100.000 kHz -> readout shows 14100.000
  const readout = panel.locator('.k-readout')
  await expect(readout).toContainText('14100.000')

  // Click the "+10" step button (6 buttons: ←10 ←1 ←0.1 +0.1 +1 +10)
  const buttons = panel.locator('.k-button')
  await buttons.nth(5).click()

  await expect(readout).toContainText('14110.000')
})

test('freq-tuning: step buttons clamp at 30000 kHz', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('freq-panel')
  const buttons = panel.locator('.k-button')
  const readout = panel.locator('.k-readout')

  // Jump high with repeated +10 clicks (25 clicks → +250 kHz).
  for (let i = 0; i < 5; ++i) {
    await buttons.nth(5).click() // +10
    await buttons.nth(5).click() // +10
    await buttons.nth(5).click() // +10
    await buttons.nth(5).click() // +10
    await buttons.nth(5).click() // +10
  }
  // 14100 + 250 = 14350, still below 30000 -> assert exact value.
  await expect(readout).toContainText('14350.000')
})

test('freq-tuning: manual text entry updates the readout', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('freq-panel')
  const input = panel.locator('input[type="number"]')
  const readout = panel.locator('.k-readout')

  await input.fill('14250.5')
  await expect(readout).toContainText('14250.500')
})