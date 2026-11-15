// Rasterise Lucide icons (ISC licence) at their exact on-screen pixel sizes.
// Output: one PNG per icon (white on transparent) for tools/gen_fonts.py.
//
//   node tools/render_icons.js <lucide-static/icons dir> <out dir>
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

// name in the firmware, Lucide icon, pixel size, stroke width (in 24-unit space)
const ICONS = [
  ['check_xl', 'check', 34, 3.0],
  ['zap_xl', 'zap', 32, 2.2],
  ['alert_sm', 'triangle-alert', 12, 2.4],
  ['beep_sm', 'volume-2', 11, 2.4],
  ['pause_sm', 'pause', 9, 3.0],
  ['plug_sm', 'plug', 12, 2.4],
  ['charge_sm', 'zap', 11, 2.4],
  ['usb_sm', 'usb', 12, 2.2],
];

(async () => {
  const [src, out] = process.argv.slice(2);
  fs.mkdirSync(out, { recursive: true });
  const browser = await chromium.launch();
  const page = await browser.newPage({ deviceScaleFactor: 1 });
  for (const [name, icon, size, stroke] of ICONS) {
    let svg = fs.readFileSync(path.join(src, icon + '.svg'), 'utf8');
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
