import { test, expect } from '@playwright/test'

/**
 * M4.9 E2E: responsive layout (Grundbedingung). At the minimum editor size
 * all control panels must be visible (they wrap, not clip); at a large
 * size the waterfall canvas is present and fills the panel.
 */
test('resize: all panels visible at 640x400', async ({ page }) => {
  await page.setViewportSize({ width: 640, height: 400 })
  await page.goto('/')

  const panels = [
    'mode-panel',
    'freq-panel',
    'band-panel',
    'audio-panel',
    'extension-panel',
    'waterfall-panel',
  ]
  for (const id of panels) {
    await expect(page.getByTestId(id)).toBeVisible()
  }
})

test('resize: all panels visible at 1024x600', async ({ page }) => {
  await page.setViewportSize({ width: 1024, height: 600 })
  await page.goto('/')

  for (const id of ['mode-panel', 'freq-panel', 'band-panel', 'audio-panel', 'waterfall-panel']) {
    await expect(page.getByTestId(id)).toBeVisible()
  }
})

test('resize: waterfall canvas renders at 1920x1080', async ({ page }) => {
  await page.setViewportSize({ width: 1920, height: 1080 })
  await page.goto('/')

  const waterfall = page.getByTestId('waterfall-panel')
  await expect(waterfall).toBeVisible()
  const canvas = waterfall.locator('canvas')
  await expect(canvas).toBeVisible()
  // The canvas element must have a non-zero rendered size.
  const box = await canvas.boundingBox()
  expect(box).not.toBeNull()
  expect(box!.width).toBeGreaterThan(100)
  expect(box!.height).toBeGreaterThan(50)
})