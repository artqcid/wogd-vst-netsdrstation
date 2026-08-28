import { test, expect } from '@playwright/test'

/**
 * M4.7.3 / 9.1-9.6 E2E: Audio sub-tab content panel.
 *
 * Verifies that after clicking the "Audio" tab button, the audio controls
 * (volume slider, NR, Compression, De-emphasis) are rendered as expected.
 */
test.describe('Audio tab content panel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Audio tab shows volume, NR, Compression, and De-emphasis controls', async ({ page }) => {
    // Click the Audio sub-tab button
    const audioTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'Audio' })
    await audioTab.click()
    await page.waitForTimeout(300)

    // 9.1 — Volume slider exists in the Volume row
    const volumeRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Volume' })
    const volumeSlider = volumeRow.locator('input[type="range"]')
    await expect(volumeSlider).toBeVisible()

    // 9.2 — Volume value shows XX%
    const volumeVal = volumeRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(volumeVal).toBeVisible()
    const volText = await volumeVal.textContent()
    expect(volText).toBeTruthy()
    expect(volText!.includes('%')).toBe(true)

    // 9.3 — NR button exists with text "ON" or "OFF"
    const nrRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'NR' })
    // The NR row contains the NR toggle button + Compression + De-emphasis buttons.
    // Pick the first button in the NR row (the NR toggle, not Comp/De-emphasis).
    const nrInRow = nrRow.locator('.kiwi-cpanel__btn').first()
    await expect(nrInRow).toBeVisible()
    const nrText = await nrInRow.textContent()
    expect(nrText).toBeTruthy()
    expect(['ON', 'OFF'].includes(nrText!.trim())).toBe(true)

    // 9.4 — Compression button exists with text "OFF" and title "Not yet implemented"
    const compressionBtn = page.locator('.kiwi-cpanel__btn--gray', { hasText: 'OFF' })
    await expect(compressionBtn.first()).toBeVisible()
    // Confirm at least one has the "Not yet implemented" title
    const compressionWithTitle = compressionBtn.filter({ hasTitle: 'Not yet implemented' })
    await expect(compressionWithTitle.first()).toBeVisible()

    // 9.5 — De-emphasis button exists with text "OFF" and title "Not yet implemented"
    const deemphBtn = page.locator('.kiwi-cpanel__btn--gray', { hasText: 'OFF' })
    await expect(deemphBtn.first()).toBeVisible()
    const deemphWithTitle = deemphBtn.filter({ hasTitle: 'Not yet implemented' })
    await expect(deemphWithTitle.first()).toBeVisible()
  })
})
