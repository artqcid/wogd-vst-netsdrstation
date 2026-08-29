import { test, expect } from '@playwright/test'

test.describe('Tag area', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Bug 4: Tag area is visible and has many tags', async ({ page }) => {
    const tagArea = page.locator('.tag-area')
    await expect(tagArea).toBeVisible()

    const tags = tagArea.locator('.tag-area__tag')
    const count = await tags.count()
    expect(count).toBeGreaterThanOrEqual(20)
  })

  test('Bug 4: Some tags have hasExt class', async ({ page }) => {
    const extTags = page.locator('.tag-area__tag--ext')
    const count = await extTags.count()
    expect(count).toBeGreaterThanOrEqual(3)
  })

  test('Bug 4: Tags are distributed across two rows', async ({ page }) => {
    const tags = page.locator('.tag-area__tag')
    let foundRow1 = false

    for (let i = 0; i < await tags.count(); i++) {
      const style = await tags.nth(i).getAttribute('style')
      if (style && style.includes('24px')) {
        foundRow1 = true
        break
      }
    }

    expect(foundRow1).toBe(true)
  })

  test('Bug 4: Specific tags from the 73-entry list are present', async ({ page }) => {
    const tagArea = page.locator('.tag-area')

    await expect(tagArea).toContainText('NAVTEX')
    await expect(tagArea).toContainText('FT8')
    await expect(tagArea).toContainText('SSTV')
    await expect(tagArea).toContainText('WWV')
  })
})
