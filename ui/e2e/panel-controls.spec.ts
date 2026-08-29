import { test, expect } from '@playwright/test'

test.describe('Panel controls', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Bug 1: P1 button toggles active class', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find Spec Δ row
    const specDeltaRow = page.locator('.kiwi-cpanel__ctrl-row', { hasText: 'Spec Δ' })

    // Find P1 button inside Spec Δ row
    const p1Btn = specDeltaRow.locator('.kiwi-cpanel__btn--violet', { hasText: 'P1' })

    // Verify initial state
    await expect(p1Btn).not.toHaveClass(/kiwi-cpanel__btn--violet-active/)

    // Click P1, expect active class added
    await p1Btn.click()
    await expect(p1Btn).toHaveClass(/kiwi-cpanel__btn--violet-active/)

    // Click again, expect class removed
    await p1Btn.click()
    await expect(p1Btn).not.toHaveClass(/kiwi-cpanel__btn--violet-active/)
  })

  test('Bug 1: P2 button toggles active class', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // P2 is in Row 11 (.kiwi-cpanel__row--dropdowns)
    const dropdownsRow = page.locator('.kiwi-cpanel__row--dropdowns')

    // Find P2 button inside dropdowns row
    const p2Btn = dropdownsRow.locator('.kiwi-cpanel__btn--violet', { hasText: 'P2' })

    // Verify initial state
    await expect(p2Btn).not.toHaveClass(/kiwi-cpanel__btn--violet-active/)

    // Click P2, expect active class added
    await p2Btn.click()
    await expect(p2Btn).toHaveClass(/kiwi-cpanel__btn--violet-active/)

    // Click again, expect class removed
    await p2Btn.click()
    await expect(p2Btn).not.toHaveClass(/kiwi-cpanel__btn--violet-active/)
  })

  test('Bug 6.2: Band-Select dropdown has expected options', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find band select
    const bandSelect = page.locator('select[aria-label="Band"]')

    // Expect at least 3 optgroup elements
    const optgroups = bandSelect.locator('optgroup')
    await expect(optgroups).toHaveCount({ min: 3 })

    // Expect at least 30 option elements
    const options = bandSelect.locator('option')
    await expect(options).toHaveCount({ min: 30 })
  })

  test('Bug 6.3: Extension-Select dropdown has expected options', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find extension select
    const extensionSelect = page.locator('select[aria-label="Extension"]')

    // Expect at least 20 option elements
    const options = extensionSelect.locator('option')
    await expect(options).toHaveCount({ min: 20 })
  })

  test('Bug 6.6: Spectrum button cycles through states', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find spectrum button in icons row
    const spectrumBtn = page.locator('.kiwi-cpanel__spectrum-btn')

    // Expect initial text to be 'Spectrum'
    await expect(spectrumBtn).toHaveText('Spectrum')

    // Click, expect 'Spec RF'
    await spectrumBtn.click()
    await expect(spectrumBtn).toHaveText('Spec RF')

    // Click, expect 'Spec AF'
    await spectrumBtn.click()
    await expect(spectrumBtn).toHaveText('Spec AF')

    // Click, expect back to 'Spectrum'
    await spectrumBtn.click()
    await expect(spectrumBtn).toHaveText('Spectrum')
  })

  test('Bug 6.7: Audio button toggles mute/unmute', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find audio button in icons row
    const audioBtn = page.locator('.kiwi-cpanel__icon-btn--green')

    // Expect it shows 🔊 emoji
    await expect(audioBtn).toContainText('🔊')

    // Click it
    await audioBtn.click()

    // Expect it shows 🔇 emoji and has red class
    await expect(audioBtn).toContainText('🔇')
    await expect(audioBtn).toHaveClass(/kiwi-cpanel__icon-btn--red/)
  })

  test('Bug 6.1: Pan buttons show correct symbols', async ({ page }) => {
    // Click WF0 sub-tab
    const wf0Tab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'WF0' })
    await wf0Tab.click()

    // Find Shift left button
    const shiftLeftBtn = page.locator('button[title="Shift left"]')
    await expect(shiftLeftBtn).toHaveText('«')

    // Find Shift right button
    const shiftRightBtn = page.locator('button[title="Shift right"]')
    await expect(shiftRightBtn).toHaveText('»')
  })

  test('Bug 6.8: RF tab shows Attn buttons, NB level, CW peaks', async ({ page }) => {
    // Click RF sub-tab
    const rfTab = page.locator('.kiwi-cpanel__tab-btn', { hasText: 'RF' })
    await rfTab.click()

    // Expect to see Attn buttons
    await expect(page.locator('.kiwi-cpanel__btn--attn', { hasText: '0 dB' })).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__btn--attn', { hasText: '-10 dB' })).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__btn--attn', { hasText: '-20 dB' })).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__btn--attn', { hasText: '-30 dB' })).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__btn--attn', { hasText: '-40 dB' })).toBeVisible()

    // Expect to see "NB level" label
    await expect(page.locator('text=NB level')).toBeVisible()

    // Expect to see "CW peaks" label and ON/OFF toggle
    await expect(page.locator('text=CW peaks')).toBeVisible()
    await expect(page.locator('.kiwi-cpanel__toggle')).toContainText('ON')
  })
})
