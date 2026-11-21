// Builds src/data/catalog.json from the HYG star database and OpenNGC, with the
// title of each object's English Wikipedia article (looked up online).
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
//   Messier objects, and NGC/IC objects of magnitude 10 or brighter (the
//   brighter of V and B minus 0.5), without single stars, duplicates and
//   non-existent entries.
//
// Wikipedia titles: the first of these that is an article (after redirects)
// and not a disambiguation page, or none:
//   deep sky: "Messier 42", "NGC 869", "IC 434";
//   stars: "Alpha Lyrae" (Bayer), "61 Cygni" (Flamsteed), "Vega (star)", "Vega".

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
  row.wikiInfo = [0, 0, 0, name, desig, s.bayer, s.flam, s.con];
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
// The brighter of V and B − 0.5: a few V values in OpenNGC are far too faint
// (NGC 253: V 11.1, B 7.9).
function magnitude(r) {
  const v = r['V-Mag'] ? Number(r['V-Mag']) : null;
  const b = r['B-Mag'] ? Number(r['B-Mag']) - 0.5 : null;
  if (v == null) return b;
  return b == null ? v : Math.min(v, b);
}

// Names used in English for some Messier objects, where OpenNGC has none or
// another one.
const MESSIER_NAMES = {
  11: 'Wild Duck Cluster', 17: 'Omega Nebula', 24: 'Sagittarius Star Cloud', 40: 'Winnecke 4',
  42: 'Orion Nebula', 44: 'Beehive Cluster', 76: 'Little Dumbbell Nebula', 101: 'Pinwheel Galaxy',
};

// The same for other objects, by catalogue id.
const OTHER_NAMES = { 'NGC 253': 'Sculptor Galaxy' };

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
    name: MESSIER_NAMES[messier] ?? OTHER_NAMES[id] ?? (r['Common names'].split(',')[0] || ''),
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

// ------------------------------------------------------------------ Wikipedia

const GENITIVE = {
  And: 'Andromedae', Ant: 'Antliae', Aps: 'Apodis', Aqr: 'Aquarii', Aql: 'Aquilae', Ara: 'Arae', Ari: 'Arietis',
  Aur: 'Aurigae', Boo: 'Boötis', Cae: 'Caeli', Cam: 'Camelopardalis', Cnc: 'Cancri', CVn: 'Canum Venaticorum',
  CMa: 'Canis Majoris', CMi: 'Canis Minoris', Cap: 'Capricorni', Car: 'Carinae', Cas: 'Cassiopeiae',
  Cen: 'Centauri', Cep: 'Cephei', Cet: 'Ceti', Cha: 'Chamaeleontis', Cir: 'Circini', Col: 'Columbae',
  Com: 'Comae Berenices', CrA: 'Coronae Australis', CrB: 'Coronae Borealis', Crv: 'Corvi', Crt: 'Crateris',
  Cru: 'Crucis', Cyg: 'Cygni', Del: 'Delphini', Dor: 'Doradus', Dra: 'Draconis', Equ: 'Equulei', Eri: 'Eridani',
  For: 'Fornacis', Gem: 'Geminorum', Gru: 'Gruis', Her: 'Herculis', Hor: 'Horologii', Hya: 'Hydrae', Hyi: 'Hydri',
  Ind: 'Indi', Lac: 'Lacertae', Leo: 'Leonis', LMi: 'Leonis Minoris', Lep: 'Leporis', Lib: 'Librae', Lup: 'Lupi',
  Lyn: 'Lyncis', Lyr: 'Lyrae', Men: 'Mensae', Mic: 'Microscopii', Mon: 'Monocerotis', Mus: 'Muscae',
  Nor: 'Normae', Oct: 'Octantis', Oph: 'Ophiuchi', Ori: 'Orionis', Pav: 'Pavonis', Peg: 'Pegasi', Per: 'Persei',
  Phe: 'Phoenicis', Pic: 'Pictoris', Psc: 'Piscium', PsA: 'Piscis Austrini', Pup: 'Puppis', Pyx: 'Pyxidis',
  Ret: 'Reticuli', Sge: 'Sagittae', Sgr: 'Sagittarii', Sco: 'Scorpii', Scl: 'Sculptoris', Sct: 'Scuti',
  Ser: 'Serpentis', Sex: 'Sextantis', Tau: 'Tauri', Tel: 'Telescopii', Tri: 'Trianguli', TrA: 'Trianguli Australis',
  Tuc: 'Tucanae', UMa: 'Ursae Majoris', UMi: 'Ursae Minoris', Vel: 'Velorum', Vir: 'Virginis', Vol: 'Volantis',
  Vul: 'Vulpeculae',
};
const GREEK_WORDS = {
  Alp: 'Alpha', Bet: 'Beta', Gam: 'Gamma', Del: 'Delta', Eps: 'Epsilon', Zet: 'Zeta', Eta: 'Eta', The: 'Theta',
  Iot: 'Iota', Kap: 'Kappa', Lam: 'Lambda', Mu: 'Mu', Nu: 'Nu', Xi: 'Xi', Omi: 'Omicron', Pi: 'Pi', Rho: 'Rho',
  Sig: 'Sigma', Tau: 'Tau', Ups: 'Upsilon', Phi: 'Phi', Chi: 'Chi', Psi: 'Psi', Ome: 'Omega',
};

const UA = 'star-track-catalogue-builder/1.0 (non-commercial; HYG and OpenNGC catalogue)';
const API = 'https://en.wikipedia.org/w/api.php';

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
// One request a second at most; on 429 or 5xx, waits (Retry-After, or longer
// each time) and tries again.
let lastRequest = 0;
export async function getJson(url) {
  for (let attempt = 0; ; attempt++) {
    await sleep(Math.max(0, lastRequest + 1000 - Date.now()));
    lastRequest = Date.now();
    const r = await fetch(url, { headers: { 'User-Agent': UA } });
    if (r.ok) return r.json();
    if (attempt >= 6 || (r.status !== 429 && r.status < 500)) throw new Error(`${url}: ${r.status}`);
    const wait = Number(r.headers.get('retry-after')) || 5 * 2 ** attempt;
    console.error(`  ${r.status}, waiting ${wait} s`);
    await sleep(wait * 1000);
  }
}

// title -> the article it leads to (redirects followed), or null (missing or a
// disambiguation page). 50 titles per request.
async function resolveTitles(titles) {
  const result = new Map();
  const list = [...new Set(titles)];
  for (let i = 0; i < list.length; i += 50) {
    const batch = list.slice(i, i + 50);
    const url = `${API}?action=query&format=json&redirects=1&prop=pageprops&ppprop=disambiguation&titles=${encodeURIComponent(batch.join('|'))}`;
    const data = await getJson(url);
    const q = data.query ?? {};
    const norm = new Map((q.normalized ?? []).map((n) => [n.from, n.to]));
    const redir = new Map((q.redirects ?? []).map((n) => [n.from, n.to]));
    const pages = new Map(Object.values(q.pages ?? {}).map((p) => [p.title, p]));
    for (const t of batch) {
      let x = norm.get(t) ?? t;
      x = redir.get(x) ?? x;
      const p = pages.get(x);
      result.set(t, p && !('missing' in p) && !('invalid' in p) && !p.pageprops?.disambiguation ? x : null);
    }
  }
  return result;
}

function starCandidates(s) {
  const [, , , name, desig, bayer, flam, con] = s.wikiInfo;
  const out = [];
  if (bayer && GENITIVE[con]) {
    const [letter, n] = bayer.split('-');
    out.push(`${GREEK_WORDS[letter]}${n ?? ''} ${GENITIVE[con]}`);
    if (n) out.push(`${GREEK_WORDS[letter]} ${GENITIVE[con]}`);
  }
  if (flam && GENITIVE[con]) out.push(`${flam} ${GENITIVE[con]}`);
  if (name) out.push(`${name} (star)`, name);
  return out;
}
function dsoCandidates(d) {
  const ids = [d.id, ...d.other];
  return ids.map((id) => id.replace(/^M (\d+)$/, 'Messier $1')).filter((t) => /^(Messier|NGC|IC) \d+$/.test(t));
}

const starCands = stars.map(starCandidates);
const dsoCands = dsos.map(dsoCandidates);
const resolved = await resolveTitles([...starCands.flat(), ...dsoCands.flat()]);
// A star must not land on a constellation or another general page.
const pick = (cands, ok = () => true) => cands.map((c) => resolved.get(c)).find((t) => t && ok(t)) ?? '';
const notConstellation = (t) => !/constellation/i.test(t) && !Object.values(GENITIVE).includes(t);
stars.forEach((s, i) => (s.wiki = pick(starCands[i], notConstellation)));
dsos.forEach((d, i) => (d.wiki = pick(dsoCands[i])));

const out = {
  about:
    'Stars: HYG database 4.4 (codeberg.org/astronexus/hyg), proper motion applied to ' +
    `${EPOCH}. Deep sky: OpenNGC (github.com/mattiaverga/OpenNGC). Both CC BY-SA 4.0; ` +
    'so is this file. RA in hours, Dec in degrees, J2000 equinox. Article titles: English Wikipedia.',
  // [ra, dec, mag, name?, designation?, wikipedia?]
  stars: stars.map((s) => (s.wiki ? [...s.slice(0, 3), s[3] ?? '', s[4] ?? '', s.wiki] : [...s])),
  dso: dsos.map((d) => [d.id, d.other.join(', '), d.name, d.type, d.con, d.ra, d.dec, d.mag, d.size, d.wiki]),
};

const here = dirname(fileURLToPath(import.meta.url));
const path = join(here, '..', 'src', 'data', 'catalog.json');
writeFileSync(path, JSON.stringify(out) + '\n');
console.log(
  `${stars.length} stars (${stars.filter((s) => s[3]).length} named, ${stars.filter((s) => s.wiki).length} with an article), ` +
    `${dsos.length} deep-sky objects (${dsos.filter((d) => d.m).length} Messier, ${dsos.filter((d) => d.wiki).length} with an article) -> ${path}`,
);
