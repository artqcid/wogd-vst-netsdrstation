import { test, expect } from '@playwright/test'

/**
 * E2E: vectorial (proportional) scaling. The 1280x720 design surface is
 * scaled uniformly to the viewport (contain), so at every size all regions
 * remain visible and the waterfall canvas keeps a non-zero size.
 */
test('resize: all regions visible at 640x400 (scaled down)', async ({ page }) => {
  await page.setViewportSize({ width: 640, height: 400 })
  await page.goto('/')

  for (const id of ['mode-panel', 'band-panel', 'freq-panel', 'audio-panel', 'extension-panel', 'waterfall-panel', 'waterfall']) {
    await expect(page.getByTestId(id)).toBeVisible()
  }
})

test('resize: all regions visible at 1024x600', async ({ page }) => {
  await page.setViewportSize({ width: 1024, height: 600 })
  await page.goto('/')

  for (const id of ['mode-panel', 'freq-panel', 'band-panel', 'audio-panel', 'waterfall-panel', 'waterfall']) {
    await expect(page.getByTestId(id)).toBeVisible()
  }
})

test('resize: waterfall canvas renders with non-zero size at 1920x1080', async ({ page }) => {
  await page.setViewportSize({ width: 1920, height: 1080 })
  await page.goto('/')

  const waterfall = page.getByTestId('waterfall')
  await expect(waterfall).toBeVisible()
  const canvas = waterfall.locator('canvas')
  await expect(canvas).toBeVisible()
  const box = await canvas.boundingBox()
  expect(box).not.toBeNull()
  expect(box!.width).toBeGreaterThan(100)
  expect(box!.height).toBeGreaterThan(50)
})