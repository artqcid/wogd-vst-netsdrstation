import { test, expect } from '@playwright/test'

test.describe('Smoke tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('page loads and shows the plugin title', async ({ page }) => {
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('renders the band scale and tag area', async ({ page }) => {
    await expect(page.locator('.band-scale')).toBeVisible()
    await expect(page.locator('.tag-area')).toBeVisible()
  })

  test('renders the control panel', async ({ page }) => {
    await expect(page.locator('.kiwi-cpanel')).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__smeter')).toBeVisible()
  })

  test('renders the waterfall canvas', async ({ page }) => {
    await expect(page.locator('.kiwi-main')).toBeVisible()
    await expect(page.locator('canvas')).toBeVisible()
  })

  test('frequency ruler is present', async ({ page }) => {
    await expect(page.locator('.freq-ruler')).toBeVisible()
  })
})
