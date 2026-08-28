const { chromium } = require('playwright');
(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  await page.goto('http://kphsdr.com:8072', { waitUntil: 'domcontentloaded', timeout: 30000 });
  await page.waitForTimeout(8000);

  // ALL IDs
  const allIDs = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('[id]')).map(el => ({
      id: el.id,
      tag: el.tagName,
      text: (el.textContent || '').trim().slice(0, 100)
    }));
  });
  console.log('=== ALL IDS (' + allIDs.length + ') ===');
  allIDs.forEach(item => console.log(item.id + ' (' + item.tag + '): ' + item.text));

  // ALL SELECTS
  const selectInfo = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('select')).map(sel => ({
      id: sel.id || '(no id)',
      cls: Array.from(sel.classList).join(' '),
      options: Array.from(sel.options).map(o => (o.text || o.value).trim())
    }));
  });
  console.log('\n=== ALL SELECTS (' + selectInfo.length + ') ===');
  selectInfo.forEach(s => {
    console.log('Select #' + s.id + ' [' + s.cls + ']:');
    s.options.forEach(o => console.log('  -> ' + o));
  });

  // ALL INPUTS
  const inputInfo = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('input')).map(inp => ({
      id: inp.id || '(no id)',
      type: inp.type,
      placeholder: inp.placeholder || '',
      value: inp.value || '',
      min: inp.min || '',
      max: inp.max || '',
      step: inp.step || ''
    }));
  });
  console.log('\n=== ALL INPUTS (' + inputInfo.length + ') ===');
  inputInfo.forEach(i => {
    console.log(i.id + ' type=' + i.type + ' val=' + i.value + ' min=' + i.min + ' max=' + i.max + ' step=' + i.step);
  });

  // ALL BUTTONS
  const buttonInfo = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('button')).map(btn => ({
      id: btn.id || '(no id)',
      text: (btn.textContent || '').trim().slice(0, 80),
      cls: Array.from(btn.classList).join(' ')
    }));
  });
  console.log('\n=== ALL BUTTONS (' + buttonInfo.length + ') ===');
  buttonInfo.forEach(b => console.log('#' + b.id + ': "' + b.text + '" [' + b.cls + ']'));

  // OPTBAR structure
  const optbarInfo = await page.evaluate(() => {
    const result = {};
    document.querySelectorAll('[id^=id-optbar]').forEach(el => {
      const id = el.id;
      const visible = el.offsetParent !== null;
      const text = (el.textContent || '').trim().slice(0, 100);
      const children = Array.from(el.children).slice(0, 15).map(c => ({
        tag: c.tagName,
        id: c.id || '',
        cls: Array.from(c.classList).slice(0, 5).join(' '),
        text: (c.textContent || '').trim().slice(0, 80)
      }));
      result[id] = { visible, text, childCount: children.length, children };
    });
    return result;
  });
  console.log('\n=== OPTBAR SECTIONS ===');
  for (const [id, info] of Object.entries(optbarInfo)) {
    console.log(id + ' visible=' + info.visible + ' children=' + info.childCount);
    console.log('  text: ' + info.text);
    info.children.forEach(c => console.log('  ' + c.tag + ' #' + c.id + ' [' + c.cls + ']: ' + c.text));
  }

  await browser.close();
})();