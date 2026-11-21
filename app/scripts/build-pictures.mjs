// Builds the pictures shown in the object sheet: public/img/*.webp and
// src/data/pictures.json (credits, licence, a short summary).
//
//   node scripts/build-pictures.mjs [--cached]
//
// --cached: no image download; objects whose image is not in the cache are
// left without a picture (when Wikimedia keeps refusing a few downloads).
//
// For each object of OBJECTS: the main image of its English Wikipedia article
// (or the Commons file in OVERRIDES), kept only if it is on Wikimedia Commons
// under a free licence (public domain, CC0, CC BY, CC BY-SA), resized to 400 px
// at most and saved as WebP (ImageMagick's convert); and the first sentences
// of the article (CC BY-SA 4.0). Needs internet; run again to refresh.
// Downloaded images are kept in .cache/pictures/ (not in git), so a run that
// stops (Wikimedia limits the request rate) goes on where it stopped.

import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const app = join(here, '..');
const catalog = JSON.parse(readFileSync(join(app, 'src/data/catalog.json'), 'utf8'));

// The objects, by the app's titles (catalog.js): the Sun, Moon and planets,
// showpiece deep-sky objects, a few double stars.
const OBJECTS = [
  'Sun', 'Moon', 'Mercury', 'Venus', 'Mars', 'Jupiter', 'Saturn', 'Uranus', 'Neptune',
  'M 1', 'M 3', 'M 4', 'M 5', 'M 6', 'M 7', 'M 8', 'M 11', 'M 13', 'M 15', 'M 16', 'M 17', 'M 20', 'M 22',
  'M 27', 'M 31', 'M 33', 'M 35', 'M 36', 'M 37', 'M 38', 'M 42', 'M 44', 'M 45', 'M 46', 'M 51', 'M 57', 'M 63',
  'M 64', 'M 65', 'M 66', 'M 81', 'M 82', 'M 92', 'M 97', 'M 101', 'M 104', 'M 106',
  'NGC 869', 'NGC 7000', 'NGC 6960', 'NGC 6992', 'NGC 7293', 'NGC 253', 'NGC 4565', 'NGC 7009', 'NGC 6543',
  'NGC 2392', 'NGC 7662', 'NGC 457', 'NGC 4631',
  'Albireo', 'Mizar',
];
const BODY_WIKI = { Mercury: 'Mercury (planet)' };

// Commons files to use instead of the article's main image (a chart, a map, a
// drawing, or a picture under another licence).
const OVERRIDES = {
  Albireo: 'Albireo_tel114.png', // as seen in a 114 mm telescope
  Mizar: 'Mizar_and_Alcor.jpg',
  'NGC 4631': 'NGC_4631-_The_Whale_(noao-n4631vickery).jpg',
};

const CACHED_ONLY = process.argv.includes('--cached');
const UA = 'star-track-catalogue-builder/1.0 (non-commercial; object pictures)';
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
let lastRequest = 0;
async function get(url, binary = false) {
  for (let attempt = 0; ; attempt++) {
    await sleep(Math.max(0, lastRequest + 1000 - Date.now()));
    lastRequest = Date.now();
    const r = await fetch(url, { headers: { 'User-Agent': UA } });
    if (r.ok) return binary ? Buffer.from(await r.arrayBuffer()) : r.json();
    if (attempt >= 10 || (r.status !== 429 && r.status < 500)) throw new Error(`${url}: ${r.status}`);
    const wait = Number(r.headers.get('retry-after')) || 5 * 2 ** attempt;
    console.error(`  ${r.status}, waiting ${wait} s`);
    await sleep(wait * 1000);
  }
}

function wikiTitle(name) {
  if (/^(M|NGC|IC) /.test(name)) return catalog.dso.find((d) => d[0] === name)?.[9];
  const star = catalog.stars.find((s) => s[3] === name);
  if (star) return star[5];
  return BODY_WIKI[name] ?? name;
}

const FREE = /^(public domain|pd|cc0|cc by(-sa)? \d)/i;
const NOT_A_PICTURE = /map|chart|finder|constellation|location|diagram|orbit|symbol|sketch|drawing/i;
const strip = (html) =>
  (html ?? '')
    .replace(/<[^>]*>/g, '')
    .replace(/&amp;/g, '&')
    .replace(/&quot;/g, '"')
    .replace(/&#0?39;/g, "'")
    .replace(/\s+/g, ' ')
    .trim();
// The thumbnail to download. Wikimedia serves thumbnails at standard widths
// (120, 250, 330, 500 px…); for an image narrower than 500 px the API gives
// the original, which upload.wikimedia.org limits much more: take the largest
// standard thumbnail below its width instead.
function thumbnail(ii) {
  if (!/thumbnail_unscaled/.test(ii.thumburl) && ii.thumbwidth < ii.width) return ii.thumburl;
  const w = [330, 250, 120].find((x) => x < ii.width);
  if (!w) return ii.url;
  const url = ii.url.split('?')[0];
  const m = /^(https:\/\/upload\.wikimedia\.org\/wikipedia\/commons)\/(.+?)\/([^/]+)$/.exec(url);
  if (!m) return ii.url;
  // TIFF thumbnails are JPEG, named after the first page.
  const name = /\.tiff?$/i.test(m[3]) ? `lossy-page1-${w}px-${m[3]}.jpg` : `${w}px-${m[3]}`;
  return `${m[1]}/thumb/${m[2]}/${m[3]}/${name}`;
}

// At most n characters, cut at a word, with "…".
function shorten(text, n) {
  if (text.length <= n) return text;
  return text.slice(0, text.lastIndexOf(' ', n - 1)).replace(/[\s,;:(]+$/, '') + '…';
}

// The first n sentences: a sentence ends at . ! or ? followed by a space and a
// capital letter (not inside "0.7 pc" or "St. ").
function sentences(text, n) {
  const parts = text.replace(/\s+/g, ' ').trim().split(/(?<=[.!?])\s+(?=[A-Z"(])/);
  return parts.slice(0, n).join(' ');
}

const API = 'https://en.wikipedia.org/w/api.php';
const COMMONS = 'https://commons.wikimedia.org/w/api.php';

const titles = new Map(OBJECTS.map((o) => [o, wikiTitle(o)]));
const missing = OBJECTS.filter((o) => !titles.get(o));
if (missing.length) throw new Error(`no Wikipedia article for ${missing.join(', ')}`);

// Page images and introductions, 20 articles per request (the extracts limit).
const pages = new Map();
const list = [...new Set(titles.values())];
for (let i = 0; i < list.length; i += 20) {
  const batch = list.slice(i, i + 20);
  const url =
    `${API}?action=query&format=json&redirects=1&prop=pageimages|extracts&piprop=name&exintro=1&explaintext=1` +
    `&exlimit=20&titles=${encodeURIComponent(batch.join('|'))}`;
  const d = await get(url);
  for (const p of Object.values(d.query.pages)) pages.set(p.title, p);
}

const files = new Map(OBJECTS.map((o) => [o, OVERRIDES[o] ?? pages.get(titles.get(o))?.pageimage]));
const info = new Map();
const fileList = [...new Set([...files.values()].filter(Boolean))];
for (let i = 0; i < fileList.length; i += 50) {
  const batch = fileList.slice(i, i + 50).map((f) => `File:${f}`);
  const url =
    `${COMMONS}?action=query&format=json&prop=imageinfo&iiprop=url|extmetadata|mime|size&iiurlwidth=500` +
    `&titles=${encodeURIComponent(batch.join('|'))}`;
  const d = await get(url);
  const norm = new Map((d.query.normalized ?? []).map((n) => [n.to, n.from]));
  for (const p of Object.values(d.query.pages)) {
    const name = (norm.get(p.title) ?? p.title).replace(/^File:/, '');
    info.set(name.replace(/ /g, '_'), p);
    info.set(name.replace(/_/g, ' '), p);
  }
}

const imgDir = join(app, 'public/img');
mkdirSync(imgDir, { recursive: true });
const cache = join(app, '.cache/pictures');
mkdirSync(cache, { recursive: true });
const out = {};
const problems = [];
for (const o of OBJECTS) {
  const title = titles.get(o);
  const page = pages.get(title);
  const file = files.get(o);
  const p = file && info.get(file);
  const ii = p?.imageinfo?.[0];
  const meta = ii?.extmetadata ?? {};
  const license = meta.LicenseShortName?.value ?? '';
  let why = '';
  if (!file) why = 'no page image';
  else if (!ii || 'missing' in p) why = `${file}: not on Commons`; // a non-free file kept on Wikipedia only
  else if (!FREE.test(license)) why = `${file}: licence ${license || 'unknown'}`;
  else if (!/^image\/(jpeg|png|webp|tiff)$/.test(ii.mime)) why = `${file}: ${ii.mime}`;
  else if (!OVERRIDES[o] && NOT_A_PICTURE.test(file)) why = `${file}: looks like a chart or drawing`;
  const entry = {
    wiki: title,
    summary: sentences(page?.extract ?? '', 3),
  };
  if (why) {
    problems.push(`${o}: ${why}`);
  } else {
    const slug = o.toLowerCase().replace(/[^a-z0-9]/g, '');
    const url = thumbnail(ii);
    const src = join(cache, createHash('sha1').update(url).digest('hex').slice(0, 16));
    if (!existsSync(src)) {
      if (CACHED_ONLY) {
        problems.push(`${o}: ${file}: not downloaded (--cached)`);
        out[o] = entry;
        continue;
      }
      writeFileSync(src, await get(url, true));
    }
    const dest = join(imgDir, `${slug}.webp`);
    execFileSync('convert', [src, '-resize', '400x400>', '-strip', '-quality', '65', dest]);
    const [w, h] = execFileSync('identify', ['-format', '%w %h', dest]).toString().split(' ').map(Number);
    Object.assign(entry, {
      img: `img/${slug}.webp`,
      w,
      h,
      artist: shorten(strip(meta.Artist?.value), 100) || 'Unknown',
      license,
      licenseUrl: meta.LicenseUrl?.value ?? '',
      source: ii.descriptionurl,
    });
  }
  out[o] = entry;
}
// Pictures of objects no longer in the list.
const kept = new Set(Object.values(out).map((e) => e.img?.slice(4)));
for (const f of readdirSync(imgDir)) if (!kept.has(f)) rmSync(join(imgDir, f));

writeFileSync(
  join(app, 'src/data/pictures.json'),
  JSON.stringify(
    {
      about:
        'Pictures from Wikimedia Commons (each with its author and licence); summaries from English Wikipedia ' +
        '(CC BY-SA 4.0). Made by scripts/build-pictures.mjs.',
      objects: out,
    },
    null,
    1,
  ) + '\n',
);
const n = Object.values(out).filter((e) => e.img).length;
console.log(`${n} pictures, ${OBJECTS.length} summaries`);
if (problems.length) console.log('Without a picture:\n  ' + problems.join('\n  '));
