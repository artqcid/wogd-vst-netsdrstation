/**
 * M4.9 — KiwiSDR LIVE Reference Capture (Phase 1)
 *
 * Öffnet den LIVE-KiwiSDR (kphsdr.com:8072) und erstellt eine
 * vollständige Referenz-Dokumentation:
 *   - Screenshot jedes UI-Zustands (Baseline, jedes Panel, jedes Tab, jeder Mode)
 *   - Vollständiges Inventar aller Elemente (IDs, Texte, Werte, Typen)
 *   - Klick-Protokoll (was passiert wenn man auf jedes Element klickt)
 *   - JSON-Metadaten
 *
 * Alle Ausgaben landen in: e2e/reference/kiwisdr-reference/
 *
 * Ausführung:
 *   npx playwright test e2e/capture-reference.spec.ts
 */
import { test, Page } from '@playwright/test'
import { fileURLToPath } from 'url'
import path from 'path'
import fs from 'fs'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const REF_DIR = path.join(__dirname, 'reference', 'kiwisdr-reference')
const BASE_URL = 'http://kphsdr.com:8072'

/** Ein einzelnes UI-Element in einem bestimmten Zustand */
interface ElementSnapshot {
  id: string
  tag: string
  text: string
  value: string
  type: string
  classes: string[]
  title: string
  ariaLabel: string
  placeholder: string
  min: string
  max: string
  step: string
  checked: boolean | null
  disabled: boolean | null
  visible: boolean
  rect: { x: number; y: number; w: number; h: number } | null
  parentId: string
  parentTag: string
  parentClasses: string[]
  options?: string[]
}

/** Ein vollständiger UI-Zustands-Capture */
interface UiState {
  name: string
  timestamp: string
  title: string
  url: string
  viewport: { w: number; h: number }
  elements: ElementSnapshot[]
  screenshot: string
}

// ============================================================
// HELPER: Discover ALL interactive elements dynamically
// ============================================================
async function captureAllElements(page: Page): Promise<ElementSnapshot[]> {
  // First discover what IDs are actually on the page
  const discoveredIds = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('[id]')).map(el => el.id)
  })
  console.log('Discovered ' + discoveredIds.length + ' DOM IDs on page')

  return page.evaluate(() => {
    const results: any[] = []

    // Helper to snapshot one element
    function snap(el: Element): any {
      const rect = el.getBoundingClientRect()
      const tag = el.tagName.toLowerCase()
      const isInput = tag === 'input' || tag === 'select' || tag === 'textarea'
      const s: any = {
        id: (el as HTMLElement).id || '',
        tag,
        text: (el.textContent || '').trim().slice(0, 120),
        value: isInput ? (el as HTMLInputElement).value ?? '' : '',
        type: (el as HTMLInputElement).type || tag,
        classes: Array.from(el.classList),
        title: (el as HTMLElement).title || '',
        ariaLabel: el.getAttribute('aria-label') || '',
        placeholder: (el as HTMLInputElement).placeholder || '',
        min: (el as HTMLInputElement).min || '',
        max: (el as HTMLInputElement).max || '',
        step: (el as HTMLInputElement).step || '',
        checked: tag === 'input' ? (el as HTMLInputElement).checked ?? null : null,
        disabled: (el as HTMLInputElement).disabled ?? null,
        visible: rect.width > 0 && rect.height > 0,
        rect: rect.width > 0 ? { x: rect.x, y: rect.y, w: rect.width, h: rect.height } : null,
        parentId: el.parentElement?.id || '',
        parentTag: el.parentElement?.tagName.toLowerCase() || '',
        parentClasses: el.parentElement ? Array.from(el.parentElement.classList) : [],
      }
      if (tag === 'select') {
        s.options = Array.from((el as HTMLSelectElement).options).map(o => (o.text || o.value).trim())
      }
      return s
    }

    // 1. Capture ALL elements that have an actual DOM id
    document.querySelectorAll('[id]').forEach(el => {
      results.push(snap(el))
    })

    // 2. Capture all button elements (even those without IDs)
    document.querySelectorAll('button').forEach(el => {
      if (!el.id) results.push(snap(el))
    })

    // 3. Capture all input/select/textarea without IDs
    document.querySelectorAll('input, select, textarea').forEach(el => {
      if (!el.id) results.push(snap(el))
    })

    // 4. Capture elements with specific interactive roles
    document.querySelectorAll('[role="button"], [role="slider"], [role="tab"], [role="switch"], [tabindex]').forEach(el => {
      if (!el.id && !results.some(r => r.tag === el.tagName && r.text === (el.textContent || '').trim().slice(0, 120))) {
        results.push(snap(el))
      }
    })

    return results
  })
}

async function captureState(page: Page, stateName: string): Promise<UiState> {
  const screenshot = stateName + '.png'
  await page.screenshot({ path: path.join(REF_DIR, screenshot), fullPage: false })
  return {
    name: stateName,
    timestamp: new Date().toISOString(),
    title: await page.title(),
    url: page.url(),
    viewport: { w: 1280, h: 800 },
    elements: await captureAllElements(page),
    screenshot,
  }
}

// ============================================================
//  REFERENCE CAPTURE TEST
// ============================================================
test('KiwiSDR Live Reference — Complete UI Inventory', async ({ browser }) => {
  const context = await browser.newContext({
    viewport: { width: 1280, height: 800 },
    userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36',
  })
  const page = await context.newPage()
  fs.mkdirSync(REF_DIR, { recursive: true })

  // --- LOAD LIVE SERVER ---
  try {
    await page.goto(BASE_URL, { waitUntil: 'domcontentloaded', timeout: 30_000 })
    await page.waitForTimeout(8000)
  } catch (err) {
    console.warn('Live server unreachable:', err)
    await context.close()
    return
  }

  const inventory: UiState[] = []

  // ------------------------------------------------------------------
  // 0: BASELINE — full page, initial load state
  // ------------------------------------------------------------------
  inventory.push(await captureState(page, '00-baseline'))

  // ------------------------------------------------------------------
  // 1: Navigate optbar (panel tabs): RF, WF, Audio, AGC, User, Stat
  //    Click each optbar button and capture state
  // ------------------------------------------------------------------
  const optbarSections = [
    { id: 'id-optbar-rf', name: 'optbar-rf' },
    { id: 'id-optbar-wf', name: 'optbar-wf' },
    { id: 'id-optbar-audio', name: 'optbar-audio' },
    { id: 'id-optbar-agc', name: 'optbar-agc' },
    { id: 'id-optbar-user', name: 'optbar-user' },
    { id: 'id-optbar-stat', name: 'optbar-stat' },
  ]

  for (const section of optbarSections) {
    const btn = page.locator('#' + section.id)
    if (await btn.isVisible().catch(() => false)) {
      await btn.click()
      await page.waitForTimeout(1000)
      inventory.push(await captureState(page, '01-' + section.name))
    }
  }

  // ------------------------------------------------------------------
  // 2: Mode buttons — click each mode
  // ------------------------------------------------------------------
  const modeButtons = [
    'id-button-am', 'id-button-sam', 'id-button-usb', 'id-button-lsb',
    'id-button-cw', 'id-button-drm', 'id-button-nbfm', 'id-button-iq',
  ]
  // First ensure the optbar-rf tab is active to see mode buttons
  const rfBtn = page.locator('#id-optbar-rf')
  if (await rfBtn.isVisible().catch(() => false)) {
    await rfBtn.click()
    await page.waitForTimeout(500)
  }

  for (const modeId of modeButtons) {
    const btn = page.locator('#' + modeId)
    if (await btn.isVisible().catch(() => false)) {
      await btn.click()
      await page.waitForTimeout(800)
      inventory.push(await captureState(page, '02-mode-' + modeId.replace(/^id-button-/, '')))
    }
  }

  // ------------------------------------------------------------------
  // 3: Zoom buttons
  // ------------------------------------------------------------------
  const zoomIn = page.locator('#id-zoom-in')
  const zoomOut = page.locator('#id-zoom-out')
  if (await zoomIn.isVisible().catch(() => false)) {
    await zoomIn.click()
    await page.waitForTimeout(500)
    inventory.push(await captureState(page, '03-zoom-in'))
    // Zoom back out a couple times
    if (await zoomOut.isVisible().catch(() => false)) {
      await zoomOut.click()
      await page.waitForTimeout(300)
      await zoomOut.click()
      await page.waitForTimeout(300)
      inventory.push(await captureState(page, '04-zoom-out'))
    }
  }

  // ------------------------------------------------------------------
  // 4: Step buttons (frequency steps)
  // ------------------------------------------------------------------
  const stepIds = ['id-step-0', 'id-step-1', 'id-step-2', 'id-step-3', 'id-step-4', 'id-step-5']
  for (const stepId of stepIds) {
    const btn = page.locator('#' + stepId)
    if (await btn.isVisible().catch(() => false)) {
      await btn.click()
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '05-step-' + stepId.replace(/^id-step-/, '')))
      await page.waitForTimeout(200)
    }
  }

  // ------------------------------------------------------------------
  // 5: Toggle buttons — click each one we can find
  // ------------------------------------------------------------------
  const toggleIds = [
    'id-button-agc', 'id-button-hang', 'id-button-spectrum',
    'id-button-spec-color', 'id-button-compression',
    'id-button-wf-autoscale', 'id-button-spec-peak0', 'id-button-spec-peak1',
  ]
  for (const toggleId of toggleIds) {
    const btn = page.locator('#' + toggleId)
    if (await btn.isVisible().catch(() => false)) {
      await btn.click()
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '06-toggle-' + toggleId.replace(/^id-button-/, '')))
      // click back to restore
      await btn.click().catch(() => {})
      await page.waitForTimeout(300)
    }
  }

  // ------------------------------------------------------------------
  // 6: Mute / play button
  // ------------------------------------------------------------------
  const playBtn = page.locator('#id-play-button')
  if (await playBtn.isVisible().catch(() => false)) {
    await playBtn.click()
    await page.waitForTimeout(800)
    inventory.push(await captureState(page, '07-play-toggle'))
    // toggle back
    await playBtn.click().catch(() => {})
    await page.waitForTimeout(500)
  }

  // ------------------------------------------------------------------
  // 7: Band select — cycle through options
  // ------------------------------------------------------------------
  const bandSelect = page.locator('#id-select-band')
  if (await bandSelect.isVisible().catch(() => false)) {
    const opts = await bandSelect.locator('option').allTextContents()
    for (let i = 0; i < Math.min(opts.length, 5); i++) {
      await bandSelect.selectOption(opts[i])
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '08-band-' + opts[i].replace(/[^a-zA-Z0-9]/g, '-')))
    }
    // Restore first option
    await bandSelect.selectOption(opts[0])
    await page.waitForTimeout(300)
  }

  // ------------------------------------------------------------------
  // 8: Extension select — cycle through options
  // ------------------------------------------------------------------
  const extSelect = page.locator('#id-select-ext')
  if (await extSelect.isVisible().catch(() => false)) {
    const opts = await extSelect.locator('option').allTextContents()
    for (let i = 0; i < Math.min(opts.length, 4); i++) {
      await extSelect.selectOption(opts[i])
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '09-ext-' + opts[i].replace(/[^a-zA-Z0-9]/g, '-')))
    }
    await extSelect.selectOption(opts[0])
    await page.waitForTimeout(300)
  }

  // ------------------------------------------------------------------
  // 9: Sliders — move sample positions
  // ------------------------------------------------------------------
  const sliderIds = [
    'id-input-volume', 'id-input-ceildb', 'id-input-floordb',
    'id-input-threshold', 'id-input-decay', 'id-input-slope',
  ]
  for (const sliderId of sliderIds) {
    const slider = page.locator('#' + sliderId)
    if (await slider.isVisible().catch(() => false)) {
      const box = await slider.boundingBox()
      if (box && box.width > 0) {
        // Move to 75%
        await page.mouse.move(box.x + box.width * 0.75, box.y + box.height / 2)
        await page.mouse.down()
        await page.waitForTimeout(150)
        await page.mouse.up()
        await page.waitForTimeout(400)
        inventory.push(await captureState(page, '10-slider-' + sliderId.replace(/^id-input-/, '')))
      }
    }
  }

  // ------------------------------------------------------------------
  // 10: Waterfall colormap dropdown
  // ------------------------------------------------------------------
  const cmapSelect = page.locator('#id-wf\\.cmap')
  if (await cmapSelect.isVisible().catch(() => false)) {
    const opts = await cmapSelect.locator('option').allTextContents()
    for (let i = 0; i < Math.min(opts.length, 5); i++) {
      await cmapSelect.selectOption(opts[i])
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '11-cmap-' + opts[i].replace(/[^a-zA-Z0-9]/g, '-')))
    }
    await cmapSelect.selectOption(opts[0])
    await page.waitForTimeout(300)
  }

  // ------------------------------------------------------------------
  // 11: Aperture dropdown
  // ------------------------------------------------------------------
  const aperSelect = page.locator('#id-wf\\.aper')
  if (await aperSelect.isVisible().catch(() => false)) {
    const opts = await aperSelect.locator('option').allTextContents()
    for (let i = 0; i < Math.min(opts.length, 4); i++) {
      await aperSelect.selectOption(opts[i])
      await page.waitForTimeout(500)
      inventory.push(await captureState(page, '12-aper-' + opts[i].replace(/[^a-zA-Z0-9]/g, '-')))
    }
    await aperSelect.selectOption(opts[0])
    await page.waitForTimeout(300)
  }

  // ------------------------------------------------------------------
  // 12: Squelch toggle + slider
  // ------------------------------------------------------------------
  const squelchBtn = page.locator('#id-squelch')
  if (await squelchBtn.isVisible().catch(() => false)) {
    await squelchBtn.click()
    await page.waitForTimeout(500)
    inventory.push(await captureState(page, '13-squelch-on'))
    const squelchSlider = page.locator('#id-squelch-field')
    if (await squelchSlider.isVisible().catch(() => false)) {
      const box = await squelchSlider.boundingBox()
      if (box && box.width > 0) {
        await page.mouse.move(box.x + box.width * 0.5, box.y + box.height / 2)
        await page.mouse.down()
        await page.waitForTimeout(150)
        await page.mouse.up()
        await page.waitForTimeout(400)
        inventory.push(await captureState(page, '14-squelch-slider'))
      }
    }
    // toggle off
    await squelchBtn.click().catch(() => {})
    await page.waitForTimeout(300)
  }

  // ------------------------------------------------------------------
  // 13: Final — return to baseline state (WF tab)
  // ------------------------------------------------------------------
  const wfSection = page.locator('#id-optbar-wf')
  if (await wfSection.isVisible().catch(() => false)) {
    await wfSection.click()
    await page.waitForTimeout(500)
    inventory.push(await captureState(page, '15-final-wf'))
  }

  // ------------------------------------------------------------------
  // SAVE INVENTORY
  // ------------------------------------------------------------------
  const jsonPath = path.join(REF_DIR, 'inventory.json')
  const summary = {
    capturedAt: new Date().toISOString(),
    source: BASE_URL,
    totalStates: inventory.length,
    states: inventory.map(s => ({
      name: s.name,
      elements: s.elements.length,
      screenshot: s.screenshot,
      title: s.title,
    })),
  }
  fs.writeFileSync(jsonPath, JSON.stringify(summary, null, 2))
  console.log('=== REFERENCE CAPTURE COMPLETE ===')
  console.log('States captured:', inventory.length)
  for (const s of inventory) {
    console.log('  ' + s.screenshot + ': ' + s.elements.length + ' elements')
  }
  console.log('Output dir:', REF_DIR)

  await context.close()
})

// ============================================================
//  SECOND TEST: simple baseline screenshot for visual diff
// ============================================================
test('KiwiSDR Live Reference — Baseline Screenshot', async ({ browser }) => {
  const context = await browser.newContext({
    viewport: { width: 1280, height: 800 },
    userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36',
  })
  const page = await context.newPage()

  try {
    await page.goto(BASE_URL, { waitUntil: 'domcontentloaded', timeout: 30_000 })
    await page.waitForTimeout(6000)
  } catch (err) {
    console.warn('Live server unreachable:', err)
    await context.close()
    return
  }

  const refDir = path.join(__dirname, 'reference')
  fs.mkdirSync(refDir, { recursive: true })
  await page.screenshot({ path: path.join(refDir, 'kiwisdr-reference.png') })
  console.log('Baseline screenshot saved')
  await context.close()
})