import { test, expect } from '@playwright/test'

/**
 * M4.7.7 / 13.1-13.2 E2E: Off sub-tab content panel.
 *
 * Verifies that after clicking the "Off" tab button, the Off tab shows
 * the MUTE button and the "Audio output disabled" notice.
 */
test.describe('Off tab content panel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Off tab shows MUTE button and audio disabled notice', async ({ page }) => {
    // Click the Off sub-tab button
    const offTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'Off' })
    await offTab.click()
    await page.waitForTimeout(300)

    // 13.1 — MUTE button exists (.kiwi-cpanel__btn--red with text "MUTE")
    const muteBtn = page.locator('.kiwi-cpanel__btn--red', { hasText: 'MUTE' })
    await expect(muteBtn).toBeVisible()

    // 13.2 — "Audio output disabled" text exists
    const disabledNotice = page.locator('text=Audio output disabled').first()
    await expect(disabledNotice).toBeVisible()
  })
})
