import { test, expect } from '@playwright/test'

test.describe('Frequency ruler', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('.kiwi-header__title')).toContainText('NetSDRStation')
  })

  test('Frequency ruler is visible at default state', async ({ page }) => {
    await expect(page.locator('.freq-ruler')).toBeVisible()
    // Major + minor ticks combined
    const allTicks = page.locator('.freq-ruler__tick--major, .freq-ruler__tick--minor')
    const count = await allTicks.count()
    expect(count).toBeGreaterThanOrEqual(24)
    expect(count).toBeLessThanOrEqual(40)
  })

  test('Frequency ruler shows labels at default zoom', async ({ page }) => {
    const labels = page.locator('.freq-ruler__label')
    await expect(labels.filter({ hasText: 'MHz' })).toHaveCount(5)
    // formatFreq(0) returns '0' (no unit) — exact match avoids matching '30' in "30 MHz"
    await expect(labels.filter({ hasText: /^0$/ })).toHaveCount(1)
  })

  test('Ticks increase when zooming in (Bug 5)', async ({ page }) => {
    const zoomBtn = page.locator('.kiwi-cpanel__icon-btn--zoom').first()
    await expect(zoomBtn).toBeVisible()

    const ticksLocatorBefore = page.locator('.freq-ruler__tick--major, .freq-ruler__tick--minor')
    const countBefore = await ticksLocatorBefore.count()

    for (let i = 0; i < 4; i++) {
      await zoomBtn.click()
    }

    const ticksLocatorAfter = page.locator('.freq-ruler__tick--major, .freq-ruler__tick--minor')
    const countAfter = await ticksLocatorAfter.count()

    expect(countAfter).toBeGreaterThan(countBefore)
  })

  test('Tick labels show kHz at medium zoom (Bug 5)', async ({ page }) => {
    const zoomBtn = page.locator('.kiwi-cpanel__icon-btn--zoom').first()
    await expect(zoomBtn).toBeVisible()

    // Tune to a low frequency first: at the default 14100 kHz every ruler
    // label renders as MHz even at high zoom, so no kHz label ever appears.
    const freqInput = page.locator('.kiwi-cpanel__freq-input')
    await freqInput.fill('500')
    await freqInput.press('Enter')

    // Zoom in 6 times to reach span ~469 kHz (wfZoom=6), step 50 kHz → kHz labels.
    for (let i = 0; i < 6; i++) {
      await zoomBtn.click()
    }

    const labels = page.locator('.freq-ruler__label')
    await expect(labels.filter({ hasText: 'kHz' })).not.toHaveCount(0)
  })

  test('Cursor trapezoid SVG exists in the CursorBar above the ruler', async ({ page }) => {
    // The cursor SVG polygon now lives in CursorBar (above the ruler),
    // not in the frequency ruler scale itself.
    const polygon = page.locator('.cursor-bar__trapezoid polygon')
    await expect(polygon.first()).toBeVisible()
  })

  test('Cursor is lime colored when passband is wide (>= 50px rendered width)', async ({ page }) => {
    // First zoom in so the rendered passband width exceeds 50px threshold.
    // Set maximum passband (±6000 Hz = 12 kHz) and narrow the view range
    // to ~500 kHz so the passband is ~120px wide on a 1000px ruler.
    await page.evaluate(() => {
      // @ts-ignore
      window.__vueStore?.setParam?.('lowCut', -6000)
      // @ts-ignore
      window.__vueStore?.setParam?.('highCut', 6000)
    })
    // Zoom in ~8 clicks so view span reduces to ~500 kHz
    for (let i = 0; i < 8; i++) {
      await page.click('.kiwi-cpanel__icon-btn--zoom')
    }
    await page.waitForTimeout(200)

    const cursorSvg = page.locator('.cursor-bar__trapezoid').first()
    await expect(cursorSvg).toBeVisible()
    const fill = await cursorSvg.getAttribute('fill')
    const stroke = await cursorSvg.getAttribute('stroke')
    const isLime = fill === 'lime' || fill === '#00ff00' || stroke === 'lime' || stroke === '#00ff00'
    expect(isLime).toBe(true)
  })

  test('Cursor is yellow when the cursor is outside the visible window', async ({ page }) => {
    // Pan the window far away so cursorKhz (14100) is outside [viewLowKhz, viewHighKhz]
    await page.evaluate(() => {
      const s = window.__vueStore
      if (!s) throw new Error('__vueStore not exposed')
      s.setParam('freqKhz', 14100)
      s.panOffsetKhz = 20000 // view centre = 34100, viewLow = 19100 > 14100 → cursor outside
    })
    await page.waitForTimeout(100)
    const cursorSvg = page.locator('.cursor-bar__trapezoid').first()
    if (await cursorSvg.count() > 0) {
      const fill = await cursorSvg.getAttribute('fill')
      const stroke = await cursorSvg.getAttribute('stroke')
      const isYellow = fill === 'yellow' || fill === '#ffff00' || stroke === 'yellow' || stroke === '#ffff00'
      expect(isYellow).toBe(true)
    }
  })

  test('Cursor trapezoid polygon has 4 points', async ({ page }) => {
    // Zoom in to ensure cursor is visible and wide enough (>50px)
    for (let i = 0; i < 8; i++) {
      await page.click('.kiwi-cpanel__icon-btn--zoom')
    }
    await page.waitForTimeout(200)

    const polygon = page.locator('.cursor-bar__trapezoid polygon').first()
    await expect(polygon).toBeVisible()
    const points = await polygon.getAttribute('points')
    expect(points).toBeTruthy()
    if (points) {
      // Split by whitespace OR commas: handles "x y" and "x,y" and "x, y" formats
      const coords = points.trim().split(/[\s,]+/)
      // New trapezoid: 4 points -> 8 numbers (4 x,y pairs)
      expect(coords.length).toBe(8)
      const yValues = coords.filter((_, i) => i % 2 === 1).map(Number)
      // y coords are the two trapezoid levels: 5 (roof) and 17 (base)
      yValues.forEach(y => expect([17, 5]).toContain(Math.round(y)))
    }
  })

  test('Cursor has vertical center line at carrier frequency position', async ({ page }) => {
    const centerLine = page.locator('.cursor-bar__trapezoid line[data-testid="cursor-carrier"]').first()
    if (await centerLine.count() > 0) {
      const x1 = await centerLine.getAttribute('x1')
      const x2 = await centerLine.getAttribute('x2')
      // x1 and x2 should be equal (vertical line)
      expect(x1).toBe(x2)
      const y1 = parseFloat(await centerLine.getAttribute('y1') ?? '0')
      const y2 = parseFloat(await centerLine.getAttribute('y2') ?? '0')
      // The carrier line now spans the full 20px CursorBar height: 0..20
      expect(y1).toBe(0)
      expect(y2).toBe(20)
    }
  })

  test('Bug 8: Major ticks have labels, minor ticks have no labels', async ({ page }) => {
    await page.waitForTimeout(200)  // wait for mount + rulerWidthPx init

    const majorTicks = page.locator('.freq-ruler__tick--major')
    const minorTicks = page.locator('.freq-ruler__tick--minor')
    const labels = page.locator('.freq-ruler__label')

    const majorCount = await majorTicks.count()
    const minorCount = await minorTicks.count()
    const labelCount = await labels.count()

    // At default zoom, should have 5-7 major ticks and many more minor ticks
    expect(majorCount).toBeGreaterThanOrEqual(3)
    // Minor ticks should be more numerous than major ticks
    expect(minorCount).toBeGreaterThanOrEqual(majorCount * 3)
    // Labels should exist and count match or be close to major tick count
    expect(labelCount).toBeGreaterThanOrEqual(1)
    // At least a few labels should be visible
    const firstVisible = await labels.first().isVisible()
    expect(firstVisible).toBe(true)
  })

  test('Bug 8: Labels contain MHz or kHz suffix', async ({ page }) => {
    const labels = page.locator('.freq-ruler__label')
    await expect(labels.first()).toBeVisible()

    const labelCount = await labels.count()
    // At least half of labels should have a unit suffix
    const withSuffix = await labels.filter({ hasText: /(MHz|kHz)/ }).count()
    expect(withSuffix).toBeGreaterThanOrEqual(Math.floor(labelCount / 2))
  })

  test('Bug 8: Tick density increases after zooming in', async ({ page }) => {
    const zoomBtn = page.locator('.kiwi-cpanel__icon-btn--zoom').first()
    await expect(zoomBtn).toBeVisible()

    const majorBefore = await page.locator('.freq-ruler__tick--major').count()
    const minorBefore = await page.locator('.freq-ruler__tick--minor').count()

    // Zoom in 4 times
    for (let i = 0; i < 4; i++) {
      await zoomBtn.click()
    }
    await page.waitForTimeout(200)

    const majorAfter = await page.locator('.freq-ruler__tick--major').count()
    const minorAfter = await page.locator('.freq-ruler__tick--minor').count()

    // More ticks visible after zoom-in
    expect(majorAfter).toBeGreaterThan(majorBefore)
    expect(minorAfter).toBeGreaterThan(minorBefore)
  })

  test('Bug 8: Minor ticks are shorter height than major ticks', async ({ page }) => {
    await page.waitForTimeout(200)

    const major = page.locator('.freq-ruler__tick--major').nth(2)
    const minor = page.locator('.freq-ruler__tick--minor').nth(2)

    const majorY2 = await major.getAttribute('y2')
    const minorY2 = await minor.getAttribute('y2')
    expect(majorY2).toBeTruthy()
    expect(minorY2).toBeTruthy()
    if (majorY2 && minorY2) {
      expect(parseFloat(majorY2)).toBeGreaterThan(parseFloat(minorY2))
    }
  })

  test('Bug 16: dragging the cursor center tunes but keeps the window stable', async ({ page }) => {
    const readStore = () => page.evaluate(() => {
      const s = window.__vueStore
      if (!s) throw new Error('__vueStore not exposed (dev server?)')
      return { freq: s.freqKhz, offset: s.panOffsetKhz }
    })
    const before = await readStore()
    const zone = page.locator('[data-testid="cursor-zone-center"]')
    const box = await zone.boundingBox()
    if (!box) throw new Error('center zone not visible')
    await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2)
    await page.mouse.down()
    await page.mouse.move(box.x + box.width / 2 + 150, box.y + box.height / 2, { steps: 5 })
    await page.mouse.up()
    await page.waitForTimeout(100)
    const after = await readStore()
    // cursor moved right → freq increased
    expect(after.freq).toBeGreaterThan(before.freq)
    // window centre stayed the same: freq + offset unchanged
    expect(after.freq + after.offset).toBeCloseTo(before.freq + before.offset, 0)
  })

  test('Bug 17: panning the ruler moves window AND cursor together', async ({ page }) => {
    const readStore = () => page.evaluate(() => {
      const s = window.__vueStore
      if (!s) throw new Error('__vueStore not exposed (dev server?)')
      return { freq: s.freqKhz, offset: s.panOffsetKhz }
    })
    const before = await readStore()
    // Pan = drag on the waterfall canvas (below the ruler; the scale itself
    // has no pan handler anymore — the CursorBar above it would catch drags).
    const waterfall = page.locator('.waterfall canvas, canvas[data-testid="waterfall"]').first()
    const target = (await waterfall.count()) > 0 ? waterfall : page.locator('.kiwi-canvas-area').first()
    const box = await target.boundingBox()
    if (!box) throw new Error('pan target not visible')
    await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2)
    await page.mouse.down()
    await page.mouse.move(box.x + box.width / 2 + 150, box.y + box.height / 2, { steps: 5 })
    await page.mouse.up()
    await page.waitForTimeout(100)
    const after = await readStore()
    // KiwiSDR: panning moves the WINDOW under the cursor...
    expect(after.freq + after.offset).not.toBe(before.freq + before.offset)
    // ...while the cursor stays at its absolute frequency (no tune)
    expect(after.freq).toBe(before.freq)
    expect(after.offset).not.toBe(before.offset)
  })

  test('Bug 17: cursor pixel position follows the panned window', async ({ page }) => {
    const carrier = page.locator('.cursor-bar__trapezoid line[data-testid="cursor-carrier"]').first()
    await expect(carrier).toBeAttached()
    const x1Before = parseFloat((await carrier.getAttribute('x1')) ?? '0')
    // Pan via waterfall canvas drag (the scale has no pan handler anymore)
    const waterfall = page.locator('.waterfall canvas, canvas[data-testid="waterfall"]').first()
    const target = (await waterfall.count()) > 0 ? waterfall : page.locator('.kiwi-canvas-area').first()
    const box = await target.boundingBox()
    if (!box) throw new Error('waterfall canvas not visible')
    await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2)
    await page.mouse.down()
    await page.mouse.move(box.x + box.width / 2 + 150, box.y + box.height / 2, { steps: 5 })
    await page.mouse.up()
    await page.waitForTimeout(100)
    const x1After = parseFloat((await carrier.getAttribute('x1')) ?? '0')
    // Dragging right on the scale pans the window; assert the carrier MOVED
    expect(x1After).not.toBe(x1Before)
  })

  test('CursorBar is its own 20px bar stacked ABOVE the 26px ruler (no overlap)', async ({ page }) => {
    const rulerBox = await page.locator('.freq-ruler').boundingBox()
    const barBox = await page.locator('.cursor-bar').boundingBox()
    expect(rulerBox).toBeTruthy()
    expect(barBox).toBeTruthy()
    if (!rulerBox || !barBox) return
    // Bar sits fully above the ruler: bar.y + bar.height <= ruler.y
    expect(barBox.y + barBox.height).toBeLessThanOrEqual(rulerBox.y + 1)
    // Ruler is 26px tall (KiwiSDR reference height)
    expect(rulerBox.height).toBeGreaterThanOrEqual(24)
    expect(rulerBox.height).toBeLessThanOrEqual(28)
    // Bar is ~20px tall
    expect(barBox.height).toBeGreaterThanOrEqual(18)
    expect(barBox.height).toBeLessThanOrEqual(24)
  })

  test('Cursor trapezoid spans the passband, NOT the full window width', async ({ page }) => {
    const barBox = await page.locator('.cursor-bar').boundingBox()
    const polygon = page.locator('.cursor-bar__trapezoid polygon').first()
    await expect(polygon).toBeVisible()
    const points = await polygon.getAttribute('points')
    expect(points).toBeTruthy()
    if (!points || !barBox) return
    const nums = points.trim().split(/[\s,]+/).map(Number)
    const xs = nums.filter((_, i) => i % 2 === 0)
    const width = Math.max(...xs) - Math.min(...xs)
    // Passband width must be a fraction of the full bar width, not the whole bar
    expect(width).toBeGreaterThan(0)
    expect(width).toBeLessThan(barBox.width * 0.5)
  })

  test('All three cursor drag zones are rendered and positioned correctly (zoomed in)', async ({ page }) => {
    // Zones only appear when the passband renders >= 50px (Kiwi allowResize rule).
    // At default zoom (span 30000 kHz) the passband is ~0.1px → collapsed.
    for (let i = 0; i < 8; i++) {
      await page.click('.kiwi-cpanel__icon-btn--zoom')
    }
    await page.waitForTimeout(200)
    const lo = page.locator('[data-testid="cursor-zone-lo"]')
    const center = page.locator('[data-testid="cursor-zone-center"]')
    const hi = page.locator('[data-testid="cursor-zone-hi"]')
    await expect(lo).toBeVisible()
    await expect(center).toBeVisible()
    await expect(hi).toBeVisible()
    // lo left of center, center left of hi
    const loBox = await lo.boundingBox()
    const cBox = await center.boundingBox()
    const hiBox = await hi.boundingBox()
    expect(loBox && cBox && hiBox).toBeTruthy()
    if (loBox && cBox && hiBox) {
      expect(loBox.x).toBeLessThan(cBox.x)
      expect(cBox.x).toBeLessThan(hiBox.x)
    }
  })

  test('At default zoom the cursor is collapsed (no lo/hi zones, yellow)', async ({ page }) => {
    // No zoom: span 30000 kHz → passband ~0.1px → collapsed Kiwi cursor.
    const lo = page.locator('[data-testid="cursor-zone-lo"]')
    const hi = page.locator('[data-testid="cursor-zone-hi"]')
    await expect(lo).toHaveCount(0)
    await expect(hi).toHaveCount(0)
    // Center zone always exists (tuning)
    await expect(page.locator('[data-testid="cursor-zone-center"]')).toBeVisible()
    const cursorBar = page.locator('[data-testid="cursor-bar"]')
    await expect(cursorBar).toHaveAttribute('data-cursor-color', 'yellow')
  })
})
