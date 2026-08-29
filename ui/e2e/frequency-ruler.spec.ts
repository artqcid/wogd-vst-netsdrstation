import { test, expect } from '@playwright/test'

test.describe('Frequency ruler', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Frequency ruler is visible at default state', async ({ page }) => {
    await expect(page.locator('.freq-ruler')).toBeVisible()

    const ticks = page.locator('.freq-ruler__tick')
    await expect(ticks).toHaveCount(6)
  })

  test('Frequency ruler shows labels at default zoom', async ({ page }) => {
    const labels = page.locator('.freq-ruler__label')
    await expect(labels.filter({ hasText: 'MHz' })).toHaveCount(5)
    await expect(labels.filter({ hasText: '0' })).toHaveCount(1)
  })

  test('Ticks increase when zooming in (Bug 5)', async ({ page }) => {
    const zoomBtn = page.locator('.kiwi-cpanel__icon-btn--zoom').first()
    await expect(zoomBtn).toBeVisible()

    const ticksBefore = page.locator('.freq-ruler__tick')
    const countBefore = await ticksBefore.count()

    for (let i = 0; i < 4; i++) {
      await zoomBtn.click()
    }

    const ticksAfter = page.locator('.freq-ruler__tick')
    const countAfter = await ticksAfter.count()

    expect(countAfter).toBeGreaterThan(countBefore)
  })

  test('Tick labels show kHz at medium zoom', async ({ page }) => {
    const zoomBtn = page.locator('.kiwi-cpanel__icon-btn--zoom').first()
    await expect(zoomBtn).toBeVisible()

    for (let i = 0; i < 4; i++) {
      await zoomBtn.click()
    }

    const labels = page.locator('.freq-ruler__label')
    await expect(labels.filter({ hasText: 'kHz' })).toHaveCount(1)
  })
})
