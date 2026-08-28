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
    await expect(options).toHaveCount(1)
    await expect(options.first()).toContainText('extension')
  })

  test('extension select can be changed', async ({ page }) => {
    const select = page.locator('.kiwi-cpanel__select[aria-label="Extension"]')
    await select.selectOption({ label: 'extension ∨' })
    await expect(select).toHaveValue('extension ∨')
  })
})

test.describe('Play button', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  // --- PLAY BUTTON (ref-matrix 4.5) ---
  test('panel play button exists', async ({ page }) => {
    const btn = page.locator('.kiwi-cpanel__play-btn')
    await expect(btn).toBeVisible()
    await expect(btn).toHaveAttribute('aria-label', 'Play')
  })

  test('floating play button exists', async ({ page }) => {
    const btn = page.locator('.kiwi-play-btn')
    await expect(btn).toBeVisible()
    await expect(btn).toHaveAttribute('aria-label', 'Start audio')
  })

  test('panel play button click is actionable', async ({ page }) => {
    const btn = page.locator('.kiwi-cpanel__play-btn')
    await expect(btn).toBeVisible()
    await btn.click()
    // Clicking toggles mute state; no visible change on button itself but button remains present
    await expect(btn).toBeVisible()
  })
})
