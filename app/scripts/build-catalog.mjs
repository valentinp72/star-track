// Builds src/data/catalog.json from the HYG star database and OpenNGC.
//
//   node scripts/build-catalog.mjs HYG.csv[.gz] NGC.csv addendum.csv
//
// Sources (both CC BY-SA 4.0, so is the generated file):
//   HYG 4.4    https://codeberg.org/astronexus/hyg  data/hyg/CURRENT/hyg_v44.csv.gz
//   OpenNGC    https://github.com/mattiaverga/OpenNGC  database_files/NGC.csv, addendum.csv
//
// Kept:
//   stars to magnitude 5.0, with their proper motion applied to EPOCH (the
//   positions stay J2000 equinox; the app precesses them to the date);
//   Messier objects, and NGC/IC objects of magnitude 10 or brighter (V, or B
//   minus 0.5 when V is missing), without single stars, duplicates and
//   non-existent entries.

import { readFileSync, writeFileSync } from 'node:fs';
import { gunzipSync } from 'node:zlib';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const STAR_MAG = 5.0;
const DSO_MAG = 10;
const EPOCH = 2026.0;

const [hygPath, ngcPath, addPath] = process.argv.slice(2);
if (!addPath) {
  console.error('usage: node scripts/build-catalog.mjs HYG.csv[.gz] NGC.csv addendum.csv');
  process.exit(2);
}

function read(path) {
  const buf = readFileSync(path);
  return (path.endsWith('.gz') ? gunzipSync(buf) : buf).toString('utf8');
}

// A CSV line with quoted fields (HYG) or a plain ';'-separated one (OpenNGC).
function splitLine(line, sep) {
  const out = [];
  let cur = '';
  let quoted = false;
  for (let i = 0; i < line.length; i++) {
    const c = line[i];
    if (quoted) {
      if (c === '"' && line[i + 1] === '"') {
        cur += '"';
        i++;
      } else if (c === '"') quoted = false;
      else cur += c;
    } else if (c === '"') quoted = true;
    else if (c === sep) {
      out.push(cur);
      cur = '';
    } else cur += c;
  }
  out.push(cur);
  return out;
}

function table(text, sep) {
  const lines = text.split(/\r?\n/).filter((l) => l.length);
  const head = splitLine(lines[0], sep);
  return lines.slice(1).map((l) => {
    const f = splitLine(l, sep);
    return Object.fromEntries(head.map((h, i) => [h, f[i] ?? '']));
  });
}

const round = (x, n) => Number(x.toFixed(n));

// ------------------------------------------------------------------ stars

const GREEK = {
  Alp: 'α', Bet: 'β', Gam: 'γ', Del: 'δ', Eps: 'ε', Zet: 'ζ', Eta: 'η', The: 'θ',
  Iot: 'ι', Kap: 'κ', Lam: 'λ', Mu: 'μ', Nu: 'ν', Xi: 'ξ', Omi: 'ο', Pi: 'π',
  Rho: 'ρ', Sig: 'σ', Tau: 'τ', Ups: 'υ', Phi: 'φ', Chi: 'χ', Psi: 'ψ', Ome: 'ω',
};
const SUPER = { 1: '¹', 2: '²', 3: '³', 4: '⁴', 5: '⁵', 6: '⁶', 7: '⁷', 8: '⁸', 9: '⁹' };

function designation(s) {
  if (s.bayer) {
    const [letter, n] = s.bayer.split('-');
    if (!GREEK[letter]) throw new Error(`unknown Bayer letter ${s.bayer}`);
    return `${GREEK[letter]}${n ? SUPER[n] : ''} ${s.con}`;
  }
  return s.flam ? `${s.flam} ${s.con}` : '';
}

const stars = [];
for (const s of table(read(hygPath), ',')) {
  const mag = Number(s.mag);
  if (s.proper === 'Sol' || !(mag <= STAR_MAG)) continue;
  const years = EPOCH - 2000;
  const dec = Number(s.dec) + (Number(s.pmdec) * years) / 3.6e6;
  const cosd = Math.cos((Number(s.dec) * Math.PI) / 180);
  let ra = Number(s.ra) + (Number(s.pmra) * years) / 3.6e6 / 15 / Math.max(cosd, 1e-6);
  ra = ((ra % 24) + 24) % 24;
  const row = [round(ra, 5), round(dec, 4), round(mag, 2)];
  const name = s.proper;
  const desig = designation(s);
  if (name || desig) row.push(name, desig);
  stars.push(row);
}
stars.sort((a, b) => a[2] - b[2]);

// ------------------------------------------------------------------ deep sky

const TYPES = {
  G: 'Galaxy', GPair: 'Galaxy pair', GTrpl: 'Galaxy triplet', GGroup: 'Galaxy group',
  OCl: 'Open cluster', GCl: 'Globular cluster', 'Cl+N': 'Cluster with nebula',
  PN: 'Planetary nebula', Neb: 'Nebula', HII: 'Emission nebula', EmN: 'Emission nebula',
  RfN: 'Reflection nebula', SNR: 'Supernova remnant', DrkN: 'Dark nebula',
  '*Ass': 'Star cloud', '**': 'Double star', Other: 'Other',
};

function hms(t) {
  const [h, m, s] = t.split(':').map(Number);
  return h + m / 60 + s / 3600;
}
function dms(t) {
  const sign = t.startsWith('-') ? -1 : 1;
  const [d, m, s] = t.replace(/^[+-]/, '').split(':').map(Number);
  return sign * (d + m / 60 + s / 3600);
}
// "NGC1976" -> "NGC 1976", "IC0434" -> "IC 434", "M040" -> "M 40"
function catalogueId(name) {
  const m = /^(NGC|IC|M|Mel)0*(\d+)(.*)$/.exec(name);
  return m ? `${m[1]} ${m[2]}${m[3]}` : name;
}
function magnitude(r) {
  if (r['V-Mag']) return Number(r['V-Mag']);
  if (r['B-Mag']) return Number(r['B-Mag']) - 0.5;
  return null;
}

// Names used in English for some Messier objects, where OpenNGC has none or
// another one.
const MESSIER_NAMES = {
  11: 'Wild Duck Cluster', 17: 'Omega Nebula', 24: 'Sagittarius Star Cloud', 40: 'Winnecke 4',
  42: 'Orion Nebula', 44: 'Beehive Cluster', 76: 'Little Dumbbell Nebula', 101: 'Pinwheel Galaxy',
};

const dsos = [];
const ngcRows = table(read(ngcPath), ';');
const addRows = table(read(addPath), ';');
for (const r of [...ngcRows, ...addRows]) {
  const messier = r.M ? Number(r.M) : null;
  const mag = magnitude(r);
  if (['Dup', 'NonEx', 'Nova', '*'].includes(r.Type)) continue;
  if (!messier && !(TYPES[r.Type] && !['**', 'Other', '*Ass'].includes(r.Type) && mag != null && mag <= DSO_MAG)) continue;
  const id = catalogueId(r.Name);
  const other = [];
  if (messier) {
    if (id !== `M ${messier}`) other.push(id);
  }
  if (r.NGC) other.push(...r.NGC.split(',').map((n) => `NGC ${Number(n) || n}`));
  if (r.IC) other.push(...r.IC.split(',').map((n) => `IC ${Number(n) || n}`));
  dsos.push({
    id: messier ? `M ${messier}` : id,
    other: [...new Set(other)].filter((o) => o !== id || messier),
    name: MESSIER_NAMES[messier] ?? (r['Common names'].split(',')[0] || ''),
    type: TYPES[r.Type] ?? r.Type,
    con: r.Const,
    ra: round(hms(r.RA), 5),
    dec: round(dms(r.Dec), 4),
    mag: mag == null ? null : round(mag, 1),
    size: r.MajAx ? round(Number(r.MajAx), 1) : null,
    m: messier,
  });
}
// Messier first, by number, then NGC, then IC, by number.
const order = (d) => (d.m ? [0, d.m] : d.id.startsWith('NGC') ? [1, parseInt(d.id.slice(4))] : [2, parseInt(d.id.slice(3)) || 0]);
dsos.sort((a, b) => {
  const [ga, na] = order(a);
  const [gb, nb] = order(b);
  return ga - gb || na - nb;
});

const out = {
  about:
    'Stars: HYG database 4.4 (codeberg.org/astronexus/hyg), proper motion applied to ' +
    `${EPOCH}. Deep sky: OpenNGC (github.com/mattiaverga/OpenNGC). Both CC BY-SA 4.0; ` +
    'so is this file. RA in hours, Dec in degrees, J2000 equinox.',
  stars, // [ra, dec, mag, name?, designation?]
  dso: dsos.map((d) => [d.id, d.other.join(', '), d.name, d.type, d.con, d.ra, d.dec, d.mag, d.size]),
};

const here = dirname(fileURLToPath(import.meta.url));
const path = join(here, '..', 'src', 'data', 'catalog.json');
writeFileSync(path, JSON.stringify(out) + '\n');
console.log(`${stars.length} stars (${stars.filter((s) => s[3]).length} named), ${dsos.length} deep-sky objects (${dsos.filter((d) => d.m).length} Messier) -> ${path}`);
