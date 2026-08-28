import { test, expect } from '@playwright/test'

/**
 * M4.7.5 / 11.1-11.4 E2E: User sub-tab content panel.
 *
 * Verifies that after clicking the "User" tab button, the User controls
 * (Squelch toggle + threshold, NB toggle + threshold) are rendered.
 */
test.describe('User tab content panel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('User tab shows Squelch and NB controls', async ({ page }) => {
    // Click the User sub-tab button
    const userTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'User' })
    await userTab.click()
    await page.waitForTimeout(300)

    // 11.1 — Squelch button (toggle ON/OFF, class --green or --gray)
    const squelchRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Squelch' })
    const squelchBtn = squelchRow.locator('.kiwi-cpanel__btn')
    await expect(squelchBtn).toBeVisible()
    const squelchText = await squelchBtn.textContent()
    expect(squelchText).toBeTruthy()
    expect(['ON', 'OFF'].includes(squelchText!.trim())).toBe(true)
    const squelchClass = await squelchBtn.evaluate(el => (el as HTMLElement).className)
    expect(squelchClass).toContain('kiwi-cpanel__btn--' + (squelchText!.trim() === 'ON' ? 'green' : 'gray'))

    // 11.2 — Squelch threshold slider exists
    const squelchSlider = squelchRow.locator('input[type="range"]')
    await expect(squelchSlider).toBeVisible()

    // 11.3 — NB button (toggle ON/OFF)
    const nbRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'NB' })
    const nbBtn = nbRow.locator('.kiwi-cpanel__btn')
    await expect(nbBtn).toBeVisible()
    const nbText = await nbBtn.textContent()
    expect(nbText).toBeTruthy()
    expect(['ON', 'OFF'].includes(nbText!.trim())).toBe(true)

    // 11.4 — NB threshold slider exists
    const nbSlider = nbRow.locator('input[type="range"]')
    await expect(nbSlider).toBeVisible()
  })
})
