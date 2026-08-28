import { test, expect } from '@playwright/test'

/**
 * M4.7.2 / 14.1-14.8 E2E: Row 11 dropdowns + S-meter.
 *
 * These controls live below the WF0 tab content area (always visible,
 * not inside a tab template). Verifies the four small selects, the P2
 * button, and the S-meter after clicking the WF0 tab.
 */
test.describe('Row 11 — dropdowns + S-meter', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('WF0 area dropdowns and S-meter are rendered', async ({ page }) => {
    // Click WF0 first — these dropdowns are part of the WF0 area
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()
    await page.waitForTimeout(300)

    // The dropdown row (.kiwi-cpanel__row--dropdowns) always renders,
    // but we confirm it exists after the WF0 tab is active.
    const dropdownRow = page.locator('.kiwi-cpanel__row--dropdowns')
    await expect(dropdownRow).toBeVisible()

    // 14.1 — Colormap select (first .kiwi-cpanel__select--sm)
    const selects = dropdownRow.locator('.kiwi-cpanel__select--sm')
    await expect(selects.nth(0)).toBeVisible()
    const colormapSelect = selects.nth(0)
    const colormapOptions = colormapSelect.locator('option')
    await expect(colormapOptions.first()).toHaveText('Kiwi')

    // 14.2 — Aperture select (second select)
    const apertureSelect = selects.nth(1)
    await expect(apertureSelect).toBeVisible()
    const apertureOptions = apertureSelect.locator('option')
    await expect(apertureOptions.first()).toHaveText('auto')

    // 14.3 — WF filter select (3rd select, has "off" option)
    const wfFilterSelect = selects.nth(2)
    await expect(wfFilterSelect).toBeVisible()
    // Options are hidden in <select>; check for the option text via the select's value/option list
    const wfFilterOptions = wfFilterSelect.locator('option')
    await expect(wfFilterOptions.filter({ hasText: 'off' })).toHaveCount(1)

    // 14.4 — Spec filter select (4th select, has IIR/MMA/EMA options)
    const specFilterSelect = selects.nth(3)
    await expect(specFilterSelect).toBeVisible()
    const specFilterOptions = specFilterSelect.locator('option')
    // The spec filter select has IIR, MMA, EMA (no "off" — see PluginView.vue 330-332)
    await expect(specFilterOptions.filter({ hasText: 'IIR' })).toHaveCount(1)
    await expect(specFilterOptions.filter({ hasText: 'MMA' })).toHaveCount(1)
    await expect(specFilterOptions.filter({ hasText: 'EMA' })).toHaveCount(1)

    // 14.5 — P2 button exists
    const p2Btn = dropdownRow.locator('.kiwi-cpanel__btn--violet', { hasText: 'P2' })
    await expect(p2Btn).toBeVisible()

    // 14.6 — S-meter is visible
    const smeter = page.locator('.kiwi-cpanel__smeter')
    await expect(smeter).toBeVisible()
  })
})
