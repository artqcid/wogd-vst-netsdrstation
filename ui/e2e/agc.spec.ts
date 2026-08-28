import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: AGC + Audio tab toggles (dev server).
 * The control panel uses a sub-tab structure. AGC and Audio controls
 * appear only when their respective tab is active.
 *
 * Buttons use CSS classes (kiwi-cpanel__btn--green = ON, --gray = OFF)
 * and text content ("ON"/"OFF" or "Mute"/"Unmute") — no aria-pressed.
 */

test.describe('Panel tab controls (AGC / Audio)', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('agc: AGC ON/OFF toggle switches button text and class', async ({ page }) => {
    // 1. Click the "AGC" sub-tab button
    const agcTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'AGC' })
    await agcTab.click()
    await page.waitForTimeout(300)

    // 2. Find the AGC ON/OFF button — it says "ON" by default (store.agcOn=true)
    const agcBtn = page.locator('.kiwi-cpanel__ctrl-row').first()
      .locator('.kiwi-cpanel__btn')

    await expect(agcBtn).toBeVisible()
    // Default: agcOn=true → text "ON", class includes --green
    await expect(agcBtn).toHaveText('ON')
    await expect(agcBtn).toHaveClass(/kiwi-cpanel__btn--green/)

    // 3. Click → OFF
    await agcBtn.click()
    await expect(agcBtn).toHaveText('OFF')
    await expect(agcBtn).toHaveClass(/kiwi-cpanel__btn--gray/)

    // 4. Click → ON again
    await agcBtn.click()
    await expect(agcBtn).toHaveText('ON')
    await expect(agcBtn).toHaveClass(/kiwi-cpanel__btn--green/)
  })

  test('audio: Mute toggle switches button text', async ({ page }) => {
    // 1. Click the "Audio" sub-tab button
    const audioTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'Audio' })
    await audioTab.click()
    await page.waitForTimeout(300)

    // 2. Find the Mute button (kiwi-cpanel__btn--red with text "Mute")
    const muteBtn = page.locator('.kiwi-cpanel__btn--red', { hasText: /Mute|Unmute/ })
    await expect(muteBtn).toBeVisible()

    // Default: store.mute=false → text "Mute"
    const initialText = await muteBtn.textContent()
    if (initialText === 'Mute') {
      // toggle on
      await muteBtn.click()
      await expect(muteBtn).toHaveText('Unmute')
      // toggle off
      await muteBtn.click()
      await expect(muteBtn).toHaveText('Mute')
    } else {
      // toggle off
      await muteBtn.click()
      await expect(muteBtn).toHaveText('Mute')
      // toggle on
      await muteBtn.click()
      await expect(muteBtn).toHaveText('Unmute')
    }
  })
})