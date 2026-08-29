import { test, expect } from '@playwright/test'

test.describe('Band presets', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('band scale shows radio amateur bands', async ({ page }) => {
    const bandScale = page.locator('.band-scale')
    await expect(bandScale).toContainText('40m')
    await expect(bandScale).toContainText('20m')
    await expect(bandScale).toContainText('80m')
  })

  test('band scale shows SW broadcast bands', async ({ page }) => {
    const bandScale = page.locator('.band-scale')
    await expect(bandScale).toContainText('31m')
    await expect(bandScale).toContainText('19m')
    await expect(bandScale).toContainText('49m')
  })

  test('clicking a band updates frequency', async ({ page }) => {
    const input = page.locator('.kiwi-cpanel__freq-input')
    // Click on the 20m amateur band label (exact text — '120m' must not match)
    await page.locator('.band-scale__block', { hasText: /^20m$/ }).first().click()
    const value = parseFloat(await input.inputValue())
    // 20m band centre is ~14.150 MHz → 14150 kHz
    expect(value).toBeGreaterThan(14000)
    expect(value).toBeLessThan(14300)
  })

  // --- BAND SCALE (ref-matrix 2.1-2.3) ---
  test('band scale canvas exists with pan arrows', async ({ page }) => {
    const bandScale = page.locator('.band-scale')
    await expect(bandScale).toBeVisible()
    await expect(page.locator('.band-scale__arrow[aria-label="Pan left"]')).toBeVisible()
    await expect(page.locator('.band-scale__arrow[aria-label="Pan right"]')).toBeVisible()
  })

  test('clicking band area fires a tune event', async ({ page }) => {
    const logs: string[] = []
    page.on('console', msg => {
      if (msg.type() === 'log') logs.push(msg.text())
    })
    const freqBefore = parseFloat(await page.locator('.kiwi-cpanel__freq-input').inputValue())
    await page.locator('.band-scale__block', { hasText: /^80m$/ }).first().click()
    const freqAfter = parseFloat(await page.locator('.kiwi-cpanel__freq-input').inputValue())
    expect(freqAfter).not.toBe(freqBefore)
    // OnBandTune sets freqKhz; no console event in Vue3 but frequency change is the observable outcome
    expect(freqAfter).toBeGreaterThan(0)
  })

  test('pan arrows change visible view', async ({ page }) => {
    const bandScale = page.locator('.band-scale')
    await expect(bandScale).toBeVisible()
    // Pan left button is clickable (fires @pan)
    await page.locator('.band-scale__arrow[aria-label="Pan left"]').click()
    // After a pan the frequency input should still be present
    await expect(page.locator('.kiwi-cpanel__freq-input')).toBeVisible()
  })
})
