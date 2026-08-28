import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: mode & passband panel (dev server).
 * Selecting a mode must apply the mode + its default passband.
 */
test('mode-select: clicking USB applies the USB default passband', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('mode-panel')
  await expect(panel).toBeVisible()

  // 18 mode buttons; USB is the 4th (AM, AMN, AMW, USB, ...).
  const usbButton = panel.getByRole('button', { name: 'USB', exact: true })
  await usbButton.click()

  // USB defaults: lowCut=300, highCut=2700 -> number inputs reflect them.
  const inputs = panel.locator('input[type="number"]')
  await expect(inputs.nth(0)).toHaveValue('300') // Low
  await expect(inputs.nth(1)).toHaveValue('2700') // High

  // The active button gets the active modifier class.
  await expect(usbButton).toHaveClass(/k-button--active/)
})

test('mode-select: clicking CW applies its default passband', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('mode-panel')
  const cwButton = panel.getByRole('button', { name: 'CW', exact: true })
  await cwButton.click()

  // CW defaults: lowCut=300, highCut=800.
  const inputs = panel.locator('input[type="number"]')
  await expect(inputs.nth(0)).toHaveValue('300')
  await expect(inputs.nth(1)).toHaveValue('800')

  // Derived bandwidth readout: 800 - 300 = 500.
  await expect(panel.locator('.k-readout')).toContainText('500')
})

test('mode-select: Reset restores the current mode defaults', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('mode-panel')
  const inputs = panel.locator('input[type="number"]')

  // Change Low to -1000 (default AM low is -4900).
  await inputs.nth(0).fill('-1000')
  await expect(inputs.nth(0)).toHaveValue('-1000')

  // Reset (last button in the panel is "Reset").
  await panel.getByRole('button', { name: 'Reset', exact: true }).click()

  await expect(inputs.nth(0)).toHaveValue('-4900')
  await expect(inputs.nth(1)).toHaveValue('4900')
})