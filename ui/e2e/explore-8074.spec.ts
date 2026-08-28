import { test } from '@playwright/test'
import path from 'path'
import fs from 'fs'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const REF_DIR = path.join(__dirname, 'reference', 'kiwisdr-reference')

test('Explore KiwiSDR 8074 — click to dismiss splash', async ({ page }) => {
  fs.mkdirSync(REF_DIR, { recursive: true })

  await page.setViewportSize({ width: 1280, height: 800 })
  await page.goto('http://kphsdr.com:8074', { waitUntil: 'domcontentloaded', timeout: 30000 })
  await page.waitForTimeout(4000)

  console.log('Title:', await page.title())
  await page.screenshot({ path: path.join(REF_DIR, '01-splash.png') })

  // Splash state
  const splashBtns = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('button, [role=button]')).map(b => ({
      id: b.id || '(no id)',
      text: (b.textContent || '').trim().slice(0, 100),
      cls: Array.from(b.classList).join(' '),
      visible: b.offsetParent !== null,
    }))
  })
  console.log('=== SPLASH BUTTONS ===')
  splashBtns.forEach(b => console.log(`  #${b.id} "${b.text}" [${b.cls}]`))

  // Click center of viewport
  console.log('\n=== CLICKING CENTER ===')
  await page.mouse.click(640, 400)
  await page.waitForTimeout(6000)

  await page.screenshot({ path: path.join(REF_DIR, '02-after-click.png') })

  // ALL visible interactive elements now
  const elements = await page.evaluate(() => {
    const result = []
    document.querySelectorAll('button, input, select, canvas, [role=button]').forEach(el => {
      const rect = el.getBoundingClientRect()
      if (rect.width > 0 && rect.height > 0) {
        result.push({
          id: el.id || '(no id)',
          tag: el.tagName,
          text: (el.textContent || '').trim().slice(0, 100),
          cls: Array.from(el.classList).slice(0, 6).join(' '),
          type: el.tagName === 'INPUT' ? el.getAttribute('type') : el.tagName,
          rect: { x: Math.round(rect.x), y: Math.round(rect.y), w: Math.round(rect.width), h: Math.round(rect.height) },
          title: el.getAttribute('title') || '',
        })
      }
    })
    return result
  })
  console.log('\n=== ELEMENTS AFTER CLICK (' + elements.length + ') ===')
  elements.forEach(e => console.log(`  #${e.id} ${e.tag} "${e.text}" [${e.cls}] type=${e.type} rect=${e.rect.w}x${e.rect.h}`))

  // All IDs
  const allIds = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('[id]')).map(el => ({
      id: el.id,
      tag: el.tagName,
      text: (el.textContent || '').trim().slice(0, 60),
    }))
  })
  console.log('\n=== ALL IDS (' + allIds.length + ') ===')
  allIds.sort((a, b) => a.id.localeCompare(b.id))
  allIds.forEach(i => console.log(i.id + ' (' + i.tag + '): ' + i.text))

  // Body text summary
  const bodyText = await page.evaluate(() => (document.body.innerText || '').trim().slice(0, 2000))
  console.log('\n=== BODY TEXT ===')
  console.log(bodyText)

  // Save element inventory
  const inventory = {
    timestamp: new Date().toISOString(),
    url: page.url(),
    title: await page.title(),
    totalElements: elements.length,
    totalIds: allIds.length,
    elements,
    ids: allIds,
    bodyText,
  }
  fs.writeFileSync(path.join(REF_DIR, 'explore-8074.json'), JSON.stringify(inventory, null, 2))
  console.log('\nSaved to explore-8074.json')
})