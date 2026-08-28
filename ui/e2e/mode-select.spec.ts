import { test, expect } from '@playwright/test'

test.describe('Mode selection', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('mode buttons are present', async ({ page }) => {
    await expect(page.locator('.kiwi-cpanel__mode-btn')).toHaveCount(8)
  })

  test('clicking USB selects USB mode', async ({ page }) => {
    const usbBtn = page.locator('.kiwi-cpanel__mode-btn', { hasText: 'USB' })
    await usbBtn.click()
    await expect(usbBtn).toHaveClass(/active/)
  })

  test('clicking LSB deselects previous mode and selects LSB', async ({ page }) => {
    // First select USB
    await page.locator('.kiwi-cpanel__mode-btn', { hasText: 'USB' }).click()
    // Then LSB
    const lsbBtn = page.locator('.kiwi-cpanel__mode-btn', { hasText: 'LSB' })
    await lsbBtn.click()
    await expect(lsbBtn).toHaveClass(/active/)
    // USB is no longer active
    await expect(page.locator('.kiwi-cpanel__mode-btn', { hasText: 'USB' })).not.toHaveClass(/active/)
  })

  test('selecting CW mode shows CW in active state', async ({ page }) => {
    await page.locator('.kiwi-cpanel__mode-btn', { hasText: 'CW' }).click()
    await expect(page.locator('.kiwi-cpanel__mode-btn', { hasText: 'CW' })).toHaveClass(/active/)
  })
})
