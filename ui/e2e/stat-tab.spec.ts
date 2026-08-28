import { test, expect } from '@playwright/test'

/**
 * M4.7.6 / 12.1-12.4 E2E: Stat sub-tab content panel.
 *
 * Verifies that after clicking the "Stat" tab button, the stat display rows
 * (GPS, Users, Buffer, SNR) show the expected default values.
 */
test.describe('Stat tab content panel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Stat tab shows GPS, Users, Buffer, and SNR values', async ({ page }) => {
    // Click the Stat sub-tab button
    const statTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'Stat' })
    await statTab.click()
    await page.waitForTimeout(300)

    // 12.1 — GPS row shows "locked" or "—"
    const gpsRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'GPS' })
    const gpsVal = gpsRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(gpsVal).toBeVisible()
    const gpsText = await gpsVal.textContent()
    expect(gpsText).toBeTruthy()
    expect(['locked', '—'].some(s => gpsText!.includes(s))).toBe(true)

    // 12.2 — Users row shows "0/4"
    const usersRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Users' })
    const usersVal = usersRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(usersVal).toBeVisible()
    await expect(usersVal).toHaveText('0/4')

    // 12.3 — Buffer row shows "OK" with green class
    const bufferRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Buffer' })
    const bufferVal = bufferRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(bufferVal).toBeVisible()
    await expect(bufferVal).toHaveText('OK')
    await expect(bufferVal).toHaveClass(/kiwi-cpanel__ctrl-val--green/)

    // 12.4 — SNR row shows readable text (32 dB or —)
    const snrRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'SNR' })
    const snrVal = snrRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(snrVal).toBeVisible()
    const snrText = await snrVal.textContent()
    expect(snrText).toBeTruthy()
    expect(['32 dB', '—'].includes(snrText!.trim())).toBe(true)
  })
})
