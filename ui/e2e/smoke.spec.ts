import { test, expect } from '@playwright/test'

test('smoke: page loads and key elements are visible', async ({ page }) => {
  // 1. Open the app URL (Vite dev server runs on http://localhost:5173)
  await page.goto('/')

  // 2. Assert station input is visible
  // StationInput renders an <input type="text"> with placeholder the station call sign,
  // wrapped in a <div> with data-testid="station-input"
  const stationInput = page.getByTestId('station-input')
  await expect(stationInput).toBeVisible()

  // 3. Assert frequency field is visible
  // NumberInput with label="Frequency" renders <label>Frequency</label>
  const freqLabel = page.getByLabel('Frequency')
  await expect(freqLabel).toBeVisible()

  // 4. Assert mode select is visible
  // PluginView renders a <select> with mode options, with data-testid="mode-select"
  const modeSelect = page.getByTestId('mode-select')
  await expect(modeSelect).toBeVisible()
})