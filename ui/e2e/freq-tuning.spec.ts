import { test, expect } from '@playwright/test'

test.describe('Frequency tuning', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('step +10 kHz increases the displayed frequency', async ({ page }) => {
    const input = page.locator('.kiwi-cpanel__freq-input')
    const initialValue = await input.inputValue()
    const initialNum = parseFloat(initialValue)
    await page.locator('button[title="+10 kHz"]').click()
    const newValue = parseFloat(await input.inputValue())
    expect(newValue).toBeCloseTo(initialNum + 10, 1)
  })

  test('step -10 kHz decreases the displayed frequency', async ({ page }) => {
    const input = page.locator('.kiwi-cpanel__freq-input')
    const initialValue = await input.inputValue()
    const initialNum = parseFloat(initialValue)
    await page.locator('button[title="−10 kHz"]').click()
    const newValue = parseFloat(await input.inputValue())
    expect(newValue).toBeCloseTo(initialNum - 10, 1)
  })

  test('+0.1 kHz steps by 0.1', async ({ page }) => {
    const input = page.locator('.kiwi-cpanel__freq-input')
    const initialNum = parseFloat(await input.inputValue())
    await page.locator('button[title="+0.1 kHz"]').click()
    const newValue = parseFloat(await input.inputValue())
    expect(newValue).toBeCloseTo(initialNum + 0.1, 1)
  })

  test('manual text entry updates the display', async ({ page }) => {
    const input = page.locator('.kiwi-cpanel__freq-input')
    await input.fill('14200.00')
    await input.dispatchEvent('change')
    await expect(input).toHaveValue(/14200/)
  })

  test('frequency step buttons are present in row 4', async ({ page }) => {
    await expect(page.locator('.kiwi-cpanel__row--nav button')).toHaveCount(6)
  })
})
