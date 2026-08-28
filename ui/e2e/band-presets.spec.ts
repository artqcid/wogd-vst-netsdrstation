import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: band presets (dev server). Selecting a band must set the
 * frequency (visible in the freq panel readout) and save/load bookmarks.
 */
test('band-presets: selecting an amateur band sets the frequency', async ({ page }) => {
  await page.goto('/')

  const bandPanel = page.getByTestId('band-panel')
  await expect(bandPanel).toBeVisible()

  const freqPanel = page.getByTestId('freq-panel')
  const readout = freqPanel.locator('.k-readout')
  await expect(readout).toContainText('14100.000') // default

  // First dropdown = Amateur; option "20 m · 14200".
  const amateur = bandPanel.locator('select').nth(0)
  await amateur.selectOption({ label: '20 m · 14200' })

  await expect(readout).toContainText('14200.000')
})

test('band-presets: selecting a timesig band sets the frequency', async ({ page }) => {
  await page.goto('/')

  const bandPanel = page.getByTestId('band-panel')
  const freqPanel = page.getByTestId('freq-panel')
  const readout = freqPanel.locator('.k-readout')

  // Third dropdown = Utility/timesig; "DCF77 · 77.5".
  const utility = bandPanel.locator('select').nth(2)
  await utility.selectOption({ label: 'DCF77 · 77.5' })

  await expect(readout).toContainText('77.500')
})

test('band-presets: save current creates a bookmark that loads back', async ({ page }) => {
  await page.goto('/')

  const bandPanel = page.getByTestId('band-panel')
  const freqPanel = page.getByTestId('freq-panel')

  // Move the frequency somewhere distinct (e.g. +10 from default).
  await freqPanel.locator('.k-button').nth(5).click() // +10 kHz
  await expect(freqPanel.locator('.k-readout')).toContainText('14110.000')

  // Save current.
  await bandPanel.getByRole('button', { name: 'Save current' }).click()
  await expect(bandPanel.locator('.band-panel__item')).toHaveCount(1)

  // Change frequency, then load the bookmark back.
  await freqPanel.locator('.k-button').nth(5).click() // +10 -> 14120
  await expect(freqPanel.locator('.k-readout')).toContainText('14120.000')

  await bandPanel.locator('.band-panel__load').click()
  await expect(freqPanel.locator('.k-readout')).toContainText('14110.000')

  // Delete the bookmark.
  await bandPanel.locator('.band-panel__delete').click()
  await expect(bandPanel.locator('.band-panel__item')).toHaveCount(0)
})