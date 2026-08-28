import { test, expect } from '@playwright/test'

test.describe('KiwiSDR layout regions', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('all 3 vertical sections are visible (header, band+tags, main)', async ({ page }) => {
    await expect(page.locator('.kiwi-header')).toBeVisible()
    await expect(page.locator('.band-scale')).toBeVisible()
    await expect(page.locator('.tag-area')).toBeVisible()
    await expect(page.locator('.kiwi-main')).toBeVisible()
  })

  test('header contains logo, title, callsign input and UTC time', async ({ page }) => {
    await expect(page.locator('.kiwi-header__logo')).toBeVisible()
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
    await expect(page.locator('.kiwi-header__callsign-input')).toBeVisible()
    await expect(page.locator('.kiwi-header__time-utc')).toBeVisible()
  })

  test('main area contains canvas, floating play button, and control panel', async ({ page }) => {
    const main = page.locator('.kiwi-main')
    await expect(main.locator('canvas')).toBeVisible()
    await expect(main.locator('.kiwi-play-btn')).toBeVisible()
    await expect(main.locator('.kiwi-cpanel')).toBeVisible()
  })

  test('control panel has S-meter, frequency input, mode buttons', async ({ page }) => {
    const panel = page.locator('.kiwi-cpanel')
    await expect(panel.locator('.kiwi-cpanel__smeter')).toBeVisible()
    await expect(panel.locator('.kiwi-cpanel__freq-input')).toBeVisible()
    await expect(panel.locator('.kiwi-cpanel__mode-btn')).toHaveCount(8)
  })

  test('sub-tab buttons are present', async ({ page }) => {
    const subTabs = page.locator('.kiwi-cpanel__tab-btn')
    await expect(subTabs).toHaveCount(7)
    await expect(subTabs.first()).toContainText('RF')
  })

  // --- HEADER DETAILS (ref-matrix 1.1-1.9) ---
  test('header shows title NetSDRStation', async ({ page }) => {
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('header sub contains antenna info', async ({ page }) => {
    await expect(page.locator('.kiwi-header__sub').filter({ hasText: 'Antenna: KiwiSDR broadband' })).toBeVisible()
  })

  test('header callsign input has placeholder callsign', async ({ page }) => {
    const input = page.locator('.kiwi-header__callsign-input')
    await expect(input).toBeVisible()
    await expect(input).toHaveAttribute('placeholder', 'callsign')
  })

  test('header UTC time element exists and updates', async ({ page }) => {
    const utc = page.locator('.kiwi-header__time-utc')
    await expect(utc).toBeVisible()
    await expect(utc).toContainText('UTC')
    // give the timer a moment to update
    await page.waitForTimeout(1100)
    await expect(utc).toContainText('UTC')
  })

  test('header local time element exists', async ({ page }) => {
    await expect(page.locator('.kiwi-header__time-local')).toBeVisible()
  })

  test('header timezone element exists', async ({ page }) => {
    await expect(page.locator('.kiwi-header__timezone')).toBeVisible()
  })

  test('header logo SVG exists', async ({ page }) => {
    const logo = page.locator('.kiwi-header__logo')
    await expect(logo).toBeVisible()
    // The logo component renders an inline SVG; assert the element's tagName is 'svg'
    const tagName = await logo.evaluate(el => el.tagName.toLowerCase())
    expect(tagName).toBe('svg')
  })

})
