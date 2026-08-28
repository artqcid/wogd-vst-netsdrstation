import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: AGC toggle (dev server). Without the native bridge the actual
 * setParameter call cannot be observed; we assert the UI state (aria-pressed)
 * and that the store toggles via the visible readout/class.
 */
test('agc: AGC On toggle switches aria-pressed state', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('audio-panel')
  await expect(panel).toBeVisible()

  // AudioPanel toggles order: Mute, AGC On, AGC Hang, Squelch On, NB, NR.
  // Find the AGC toggle by its section + label context: the "On" toggle
  // right after the AGC section title.
  const agcSection = panel.locator('.audio-panel__section').nth(0) // AGC section
  const agcOnToggle = agcSection.getByRole('button', { name: /On/ })
  await expect(agcOnToggle).toBeVisible()

  // Store default agcOn=true -> aria-pressed="true".
  await expect(agcOnToggle).toHaveAttribute('aria-pressed', 'true')

  await agcOnToggle.click()
  await expect(agcOnToggle).toHaveAttribute('aria-pressed', 'false')

  await agcOnToggle.click()
  await expect(agcOnToggle).toHaveAttribute('aria-pressed', 'true')
})

test('agc: Mute toggle switches aria-pressed state', async ({ page }) => {
  await page.goto('/')

  const panel = page.getByTestId('audio-panel')
  const muteToggle = panel.getByRole('button', { name: /Mute/ })
  await expect(muteToggle).toHaveAttribute('aria-pressed', 'false')

  await muteToggle.click()
  await expect(muteToggle).toHaveAttribute('aria-pressed', 'true')
})