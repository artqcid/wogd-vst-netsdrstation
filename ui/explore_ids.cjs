import { chromium } from 'playwright';
(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  await page.goto('http://kphsdr.com:8072', { waitUntil: 'domcontentloaded', timeout: 30000 });
  await page.waitForTimeout(8000);

  // Get ALL actual DOM IDs (not CSS classes)
  const ids = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('[id]')).map(el => ({
      id: el.id,
      tag: el.tagName,
      type: (el as HTMLInputElement).type || el.tagName,
      text: (el.textContent || '').trim().slice(0, 60)
    }));
  });
  console.log('=== ALL ACTUAL DOM IDS (' + ids.length + ') ===');
  ids.sort((a, b) => a.id.localeCompare(b.id));
  ids.forEach(item => console.log(item.id + ' (' + item.tag + '/' + item.type + '): ' + item.text));

  // Also get ALL elements that have id-* in their class list (the OpenWebRX convention)
  const classIds = await page.evaluate(() => {
    const result = [];
    document.querySelectorAll('[class*="id-"]').forEach(el => {
      const classes = Array.from(el.classList).filter(c => c.startsWith('id-'));
      if (classes.length > 0) {
        result.push({
          id: el.id || '(no id)',
          tag: el.tagName,
          classes: classes.slice(0, 5),
          text: (el.textContent || '').trim().slice(0, 40)
        });
      }
    });
    return result;
  });
  console.log('\n=== ELEMENTS WITH id-* CLASSES (' + classIds.length + ') ===');
  classIds.forEach(item => {
    console.log('#' + item.id + ' ' + item.tag + ' classes: ' + item.classes.join(', ') + ' text: ' + item.text);
  });

  // Button labels
  const btnTexts = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('button, [role=button], .w3-btn, .class-button')).map(el => ({
      id: (el as HTMLElement).id || '(no id)',
      tag: el.tagName,
      text: (el.textContent || '').trim().slice(0, 40),
      cls: Array.from(el.classList).slice(0, 5).join(' ')
    }));
  });
  console.log('\n=== BUTTONS / CLICKABLE (' + btnTexts.length + ') ===');
  btnTexts.forEach(b => console.log('#' + b.id + ' ' + b.tag + ' "' + b.text + '" [' + b.cls + ']'));

  await browser.close();
})();