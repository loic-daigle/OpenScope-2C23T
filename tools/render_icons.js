// Rasterise Lucide icons (ISC licence) at their exact on-screen pixel sizes.
// Output: one PNG per icon (white on transparent) for tools/gen_fonts.py.
//
//   node tools/render_icons.js <lucide-static/icons dir> <out dir>
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

// Our own shapes, drawn in Lucide's 24-unit grid and stroke style.
const CUSTOM = {
  // A straight-edged lightning bolt (Lucide's 'zap' is more slanted).
  bolt: '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" ' +
        'stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
        '<path d="M13.5 2 4.5 13.5h6.5L9.5 22l10-12.5h-6.5L13.5 2z"/></svg>',
};

// name in the firmware, icon (Lucide name or CUSTOM key), pixel size, stroke width (24-unit space)
const ICONS = [
  ['check_xl', 'check', 34, 3.0],
  ['zap_xl', 'bolt', 32, 2.0],
  ['alert_sm', 'triangle-alert', 12, 2.4],
  ['beep_sm', 'volume-2', 11, 2.4],
  ['pause_sm', 'pause', 9, 3.0],
  ['plug_sm', 'plug', 12, 2.4],
  ['charge_sm', 'bolt', 11, 2.4],
  ['usb_sm', 'usb', 12, 2.2],
];

(async () => {
  const [src, out] = process.argv.slice(2);
  fs.mkdirSync(out, { recursive: true });
  const browser = await chromium.launch();
  const page = await browser.newPage({ deviceScaleFactor: 1 });
  for (const [name, icon, size, stroke] of ICONS) {
    let svg = CUSTOM[icon] || fs.readFileSync(path.join(src, icon + '.svg'), 'utf8');
    svg = svg.replace(/<!--[\s\S]*?-->/g, '')
      .replace(/width="24"/, `width="${size}"`).replace(/height="24"/, `height="${size}"`)
      .replace(/stroke="currentColor"/, 'stroke="#ffffff"')
      .replace(/stroke-width="[0-9.]+"/, `stroke-width="${stroke}"`);
    await page.setContent(`<html><body style="margin:0;background:transparent">${svg}</body></html>`);
    const el = await page.$('svg');
    await el.screenshot({ path: path.join(out, name + '.png'), omitBackground: true });
  }
  await browser.close();
})();
