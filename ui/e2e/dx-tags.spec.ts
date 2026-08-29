import { test, expect } from '@playwright/test'

test.describe('DX tags', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  // --- DX TAGS (ref-matrix 3.1-3.2) ---
  test('tag area is visible and contains DX labels', async ({ page }) => {
    const tagArea = page.locator('.tag-area')
    await expect(tagArea).toBeVisible()
    await expect(tagArea).toContainText('WWV')
    await expect(tagArea).toContainText('FT8')
  })

  test('clicking a DX tag shows tag popup with freq and name', async ({ page }) => {
    const tagArea = page.locator('.tag-area')
    await expect(tagArea).toBeVisible()
    // Pick a specific tag that is unique and visible (NAVTEX at 518 kHz).
    // Two-row layout makes tags overlap, so a coordinate-based click can hit
    // a neighbouring tag — dispatch the click directly on the element instead.
    const uniqueTag = tagArea.locator('.tag-area__tag').filter({ hasText: 'NAVTEX' }).first()
    await uniqueTag.evaluate(el => (el as HTMLElement).click())
    const popup = page.locator('.tag-popup')
    await expect(popup).toBeVisible({ timeout: 4000 })
    // The popup header shows the tag label ('NAVTEX')
    await expect(popup.locator('.tag-popup__header')).toContainText('NAVTEX')
    // The popup info table includes the frequency line, e.g. "518 kHz"
    await expect(popup.locator('.tag-popup__info')).toContainText('518 kHz')
  })

  test('tag popup can be closed', async ({ page }) => {
    const tagArea = page.locator('.tag-area')
    await expect(tagArea).toBeVisible()
    // WWV label at 10000 kHz (unique in the 73-entry demo list)
    const tag = tagArea.locator('.tag-area__tag').filter({ hasText: 'WWV' }).first()
    await tag.evaluate(el => (el as HTMLElement).click())
    await expect(page.locator('.tag-popup')).toBeVisible({ timeout: 4000 })
    // Click on overlay background (outside popup)
    await page.locator('.tag-popup-overlay').click({ position: { x: 10, y: 10 } })
    await expect(page.locator('.tag-popup')).not.toBeVisible({ timeout: 4000 })
  })
})
