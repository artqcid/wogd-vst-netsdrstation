// Build SOLL-Matrix from reference capture data
import fs from 'fs'

const REF = 'e2e/reference/kiwisdr-reference'
const explore = JSON.parse(fs.readFileSync(`${REF}/explore-8074.json`, 'utf-8'))
const subtabs = JSON.parse(fs.readFileSync(`${REF}/subtabs.json`, 'utf-8'))
const header = JSON.parse(fs.readFileSync(`${REF}/header-topbar.json`, 'utf-8'))
const freqCanvas = JSON.parse(fs.readFileSync(`${REF}/freq-canvas.json`, 'utf-8'))
const dxSelects = JSON.parse(fs.readFileSync(`${REF}/dx-selects-smeter.json`, 'utf-8'))

// Collect key interactive elements from explore-8074
const interactive = explore.elements.filter(e => 
  (e.tag === 'button' || e.tag === 'input' || e.tag === 'select') &&
  e.visible &&
  e.rect.w > 10
)

console.log('=== KEY INTERACTIVE ELEMENTS FROM LIVE ===')
console.log(`Total: ${interactive.length}`)

// Group by region
const freqEls = interactive.filter(e => e.id.includes('freq') || e.id.includes('step') || e.id.includes('zoom') || e.id.includes('input'))
const modeEls = interactive.filter(e => e.id.includes('mode-col') || e.id.includes('id-mode-'))
const selectEls = interactive.filter(e => e.tag === 'select')
const buttonEls = interactive.filter(e => e.tag === 'button' && !e.id.includes('dx-label') && !e.id.includes('mode-col'))
const dxEls = interactive.filter(e => e.id.includes('dx-label'))
const canvasEls = explore.elements.filter(e => e.tag === 'canvas')

console.log(`\n=== BY REGION ===`)
console.log(`Frequency controls: ${freqEls.length}`)
freqEls.forEach(e => console.log(`  ${e.id} (${e.tag}): value="${e.value}" text="${e.text.slice(0,40)}"`))

console.log(`\nMode buttons: ${modeEls.length}`)
modeEls.forEach(e => console.log(`  ${e.id}: "${e.text}"`))

console.log(`\nSelects: ${selectEls.length}`)
selectEls.forEach(e => console.log(`  ${e.id} (${e.tag}): value="${e.value}" options=${e.options ? e.options.length : 0}`))

console.log(`\nNon-DX buttons: ${buttonEls.length}`)
buttonEls.forEach(e => console.log(`  ${e.id}: "${e.text.slice(0,60)}"`))

console.log(`\nCanvas: ${canvasEls.length}`)
canvasEls.forEach(e => console.log(`  ${e.id}: ${e.rect.w}x${e.rect.h} at (${e.rect.x},${e.rect.y})`))

console.log(`\nDX tags: ${dxEls.length}`)
dxEls.slice(0,5).forEach(e => console.log(`  ${e.id}: "${e.text}"`))
console.log(`  ... and ${dxEls.length - 5} more`)

// Print header elements
console.log('\n=== HEADER ELEMENTS ===')
const headerIds = header.elements?.filter(e => e.visible) || []
headerIds.slice(0,20).forEach(e => console.log(`  ${e.id} (${e.tag}): "${e.text.slice(0,60)}"`))

// Print canvas inventory from freqCanvas
console.log('\n=== CANVAS INVENTORY ===')
const canvases = freqCanvas.canvases || []
canvases.forEach(c => console.log(`  ${c.id}: ${c.w}x${c.h} at (${c.x},${c.y})`))

// Print subtab tab structure
console.log('\n=== SUB-TABS ===')
const tabs = subtabs.tabs || []
tabs.forEach(t => console.log(`  ${t.id}: ${t.label || ''} (${t.elements?.length || 0} elements)`))

// Print DX select info
console.log('\n=== DX/BAND/EXT INFO ===')
console.log(`Band options: ${dxSelects.bandOptions?.length || 0}`)
if (dxSelects.bandOptions) console.log(`  First 5: ${dxSelects.bandOptions.slice(0,5).join(', ')}`)
console.log(`Extension options: ${dxSelects.extOptions?.length || 0}`)
if (dxSelects.extOptions) console.log(`  First 5: ${dxSelects.extOptions.slice(0,5).join(', ')}`)

console.log('\n=== FREQ INFO ===')
console.log(`Baseline freq: "${freqCanvas.baseline?.freq || 'unknown'}"`)
console.log(`Mode active: "${freqCanvas.baseline?.modeActive || 'none'}"`)