import { test, expect } from '@playwright/test'

/**
 * M4.7.2 / 8.1-8.11 E2E: WF0 sub-tab content panel.
 *
 * Verifies that after clicking the "WF0" tab button, the WF0 control rows
 * (ceil/floor/rate, Spec Δ, Auto Scale, Spec Color, P1, colormap) are
 * rendered with the expected selectors and default values.
 */
test.describe('WF0 tab content panel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('WF0 tab shows all control rows and buttons', async ({ page }) => {
    // Click the WF0 sub-tab button
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()
    await page.waitForTimeout(300)

    // 8.1 — WF ceil slider (range input in the "WF ceil" row)
    const wfCeilRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'WF ceil' })
    const wfCeilSlider = wfCeilRow.locator('input[type="range"]')
    await expect(wfCeilSlider).toBeVisible()

    // 8.2 — WF ceil value shows "+X dB" or "X dB"
    const wfCeilVal = wfCeilRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(wfCeilVal).toBeVisible()
    const ceilText = await wfCeilVal.textContent()
    expect(ceilText).toBeTruthy()
    expect(ceilText!.includes('dB')).toBe(true)

    // 8.3 — WF floor slider
    const wfFloorRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'WF floor' })
    const wfFloorSlider = wfFloorRow.locator('input[type="range"]')
    await expect(wfFloorSlider).toBeVisible()

    // 8.4 — WF floor value shows "X dB"
    const wfFloorVal = wfFloorRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(wfFloorVal).toBeVisible()
    const floorText = await wfFloorVal.textContent()
    expect(floorText).toBeTruthy()
    expect(floorText!.includes('dB')).toBe(true)

    // 8.5 — WF rate slider
    const wfRateRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'WF rate' })
    const wfRateSlider = wfRateRow.locator('input[type="range"]')
    await expect(wfRateSlider).toBeVisible()

    // 8.6 — WF rate shows one of: pause, slow, med, fast, max
    const wfRateVal = wfRateRow.locator('.kiwi-cpanel__ctrl-val')
    await expect(wfRateVal).toBeVisible()
    const rateText = await wfRateVal.textContent()
    expect(rateText).toBeTruthy()
    expect(['pause', 'slow', 'med', 'fast', 'max'].includes(rateText!.toLowerCase().trim())).toBe(true)

    // 8.7 — Spec Δ slider
    const specRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Spec Δ' })
    const specSlider = specRow.locator('input[type="range"]')
    await expect(specSlider).toBeVisible()

    // 8.8 — Auto Scale button (.kiwi-cpanel__btn--green)
    const autoScaleBtn = page.locator('.kiwi-cpanel__btn--green', { hasText: 'Auto Scale' })
    await expect(autoScaleBtn).toBeVisible()

    // 8.9 — Spec Color button (.kiwi-cpanel__btn--gray)
    const specColorBtn = page.locator('.kiwi-cpanel__btn--gray', { hasText: 'Spec Color' })
    await expect(specColorBtn).toBeVisible()

    // 8.10 — P1 button (.kiwi-cpanel__btn--violet)
    const p1Btn = page.locator('.kiwi-cpanel__btn--violet', { hasText: 'P1' })
    await expect(p1Btn).toBeVisible()

    // 8.11 — Colormap bar
    const colormap = page.locator('.kiwi-cpanel__colormap')
    await expect(colormap).toBeVisible()
  })
})
