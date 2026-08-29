import { test, expect } from '@playwright/test'

test.describe('Band scale', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Bug 3: band scale has visible blocks', async ({ page }) => {
    const bandScale = page.locator('.band-scale')
    const bandBlocks = page.locator('.band-scale__block')

    await expect(bandScale).toBeVisible()

    const blockCount = await bandBlocks.count()
    expect(blockCount).toBeGreaterThanOrEqual(5)

    const firstBlock = bandBlocks.first()
    const firstBlockStyle = await firstBlock.evaluate(el => el.style.width)
    expect(firstBlockStyle).not.toEqual('0%')
    expect(firstBlockStyle).not.toEqual('0px')

    // At least one block shows a known shortwave band (49m or 40m).
    const has49mOr40m = await bandBlocks
      .filter({ hasText: /49m|40m/ })
      .count()
    expect(has49mOr40m).toBeGreaterThanOrEqual(1)
  })

  test('Band blocks have non-zero width', async ({ page }) => {
    const bandBlocks = page.locator('.band-scale__block')

    const blockCount = await bandBlocks.count()
    expect(blockCount).toBeGreaterThanOrEqual(1)

    for (let i = 0; i < blockCount; i++) {
      const block = bandBlocks.nth(i)
      const isVisible = await block.isVisible()
      if (!isVisible) continue

      const styleWidth = await block.evaluate(el => el.style.width)
      // Width should be a positive percentage like '12.5%'.
      expect(styleWidth).toMatch(/^\d+(\.\d+)?%$/)
      const numericValue = parseFloat(styleWidth)
      expect(numericValue).toBeGreaterThan(0)
    }
  })

  test('Clicking a band tunes to its frequency', async ({ page }) => {
    const bandBlocks = page.locator('.band-scale__block')

    const blockCount = await bandBlocks.count()
    expect(blockCount).toBeGreaterThanOrEqual(1)

    // Find a block labeled '40m' (40m shortwave band, ~7 MHz center).
    const targetBlock = bandBlocks.filter({ hasText: /^40m$/ })
    const targetCount = await targetBlock.count()
    expect(targetCount).toBeGreaterThanOrEqual(1)

    await targetBlock.first().click()

    const freqInput = page.locator('.kiwi-cpanel__freq-input')
    await expect(freqInput).toBeVisible()

    const freqText = await freqInput.inputValue()
    // 40m band center is around 7100 kHz; accept 7000–7200 kHz range.
    expect(freqText).toMatch(/^7[012]\d{2}$/)
  })
})
