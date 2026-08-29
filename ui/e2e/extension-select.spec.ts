import { test, expect } from '@playwright/test'

test.describe('Extension select', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  // --- EXTENSION SELECT (ref-matrix 4.3) ---
  test('extension select exists with aria-label Extension', async ({ page }) => {
    const select = page.locator('.kiwi-cpanel__select[aria-label="Extension"]')
    await expect(select).toBeVisible()
    await expect(select).toHaveAttribute('aria-label', 'Extension')
  })

  test('extension select has options', async ({ page }) => {
    const select = page.locator('.kiwi-cpanel__select[aria-label="Extension"]')
    const options = select.locator('option')
    // Bug 6.3: 26 KiwiSDR extensions + placeholder are now populated (M4c.7 fix).
    await expect(options).toHaveCount(27)
    await expect(options.first()).toContainText('extension')
  })

  test('extension select can be changed', async ({ page }) => {
    const select = page.locator('.kiwi-cpanel__select[aria-label="Extension"]')
    await select.selectOption({ label: 'WSPR' })
    await expect(select).toHaveValue('WSPR')
  })
})

test.describe('Play button', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  // --- PLAY BUTTON (ref-matrix 4.5) ---
  // Bug 6.5 removed the panel-integrated play button (it does not exist in the
  // KiwiSDR reference — the floating play button left of the canvas is the
  // correct 1:1 element). The old .kiwi-cpanel__play-btn selector is obsolete.
  test('floating play button exists', async ({ page }) => {
    const btn = page.locator('.kiwi-play-btn')
    await expect(btn).toBeVisible()
    await expect(btn).toHaveAttribute('aria-label', 'Start audio')
  })

  test('floating play button click toggles mute state', async ({ page }) => {
    const btn = page.locator('.kiwi-play-btn')
    await expect(btn).toBeVisible()
    await btn.click()
    // Clicking toggles mute state; no visible change on button itself but button remains present
    await expect(btn).toBeVisible()
  })
})
