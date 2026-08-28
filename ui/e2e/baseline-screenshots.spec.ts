import { test } from '@playwright/test'

test.describe('Baseline screenshots', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await page.waitForSelector('.kiwi-header__title')
    // Wait for initial render to stabilize
    await page.waitForTimeout(2000)
  })

  test('full-page baseline', async ({ page }) => {
    await page.screenshot({ path: 'e2e/reference/plugin-baseline-full.png', fullPage: true })
  })

  test('header area', async ({ page }) => {
    const header = page.locator('.kiwi-header')
    await header.screenshot({ path: 'e2e/reference/plugin-baseline-header.png' })
  })

  test('control panel', async ({ page }) => {
    const cpanel = page.locator('.kiwi-cpanel')
    await cpanel.screenshot({ path: 'e2e/reference/plugin-baseline-cpanel.png' })
  })

  test('band scale + tags', async ({ page }) => {
    await page.locator('.band-scale').screenshot({ path: 'e2e/reference/plugin-baseline-band.png' })
    await page.locator('.tag-area').screenshot({ path: 'e2e/reference/plugin-baseline-tags.png' })
  })

  test('mode buttons', async ({ page }) => {
    const modeRow = page.locator('.kiwi-cpanel__row--modes')
    await modeRow.screenshot({ path: 'e2e/reference/plugin-baseline-modes.png' })
  })

  test('step/zoom icons', async ({ page }) => {
    const iconRow = page.locator('.kiwi-cpanel__row--icons')
    await iconRow.screenshot({ path: 'e2e/reference/plugin-baseline-icons.png' })
  })

  test('nav step buttons', async ({ page }) => {
    const navRow = page.locator('.kiwi-cpanel__row--nav')
    await navRow.screenshot({ path: 'e2e/reference/plugin-baseline-steps.png' })
  })

  test('sub-tabs', async ({ page }) => {
    const tabsRow = page.locator('.kiwi-cpanel__row--tabs')
    await tabsRow.screenshot({ path: 'e2e/reference/plugin-baseline-tabs.png' })
  })

  test('waterfall canvas', async ({ page }) => {
    await page.locator('canvas').first().screenshot({ path: 'e2e/reference/plugin-baseline-waterfall.png' })
  })

  test('freq ruler', async ({ page }) => {
    await page.locator('.freq-ruler').screenshot({ path: 'e2e/reference/plugin-baseline-ruler.png' })
  })

  test('smeter', async ({ page }) => {
    await page.locator('.kiwi-cpanel__smeter').screenshot({ path: 'e2e/reference/plugin-baseline-smeter.png' })
  })
})