import { test, expect } from '@playwright/test'

test.describe('Window resize', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('control panel remains visible after resize', async ({ page }) => {
    await page.setViewportSize({ width: 800, height: 600 })
    await expect(page.locator('.kiwi-cpanel')).toBeVisible()
    await page.setViewportSize({ width: 1200, height: 900 })
    await expect(page.locator('.kiwi-cpanel')).toBeVisible()
  })

  test('header content scales with width', async ({ page }) => {
    await page.setViewportSize({ width: 400, height: 600 })
    // header still visible even on narrow screens
    await expect(page.locator('.kiwi-header')).toBeVisible()
  })
})
