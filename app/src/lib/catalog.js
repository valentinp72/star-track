// The objects the app can point at: the Sun, Moon and planets, deep-sky
// objects and stars. Built from src/data/catalog.json (see
// scripts/build-catalog.mjs). Fixed objects have J2000 RA (hours) and Dec
// (degrees); `sky.apparent` turns them into coordinates of date.

import data from '../data/catalog.json';
import { BODIES } from './sky.js';

export const CONSTELLATIONS = {
  And: 'Andromeda', Ant: 'Antlia', Aps: 'Apus', Aqr: 'Aquarius', Aql: 'Aquila', Ara: 'Ara', Ari: 'Aries',
  Aur: 'Auriga', Boo: 'Boötes', Cae: 'Caelum', Cam: 'Camelopardalis', Cnc: 'Cancer', CVn: 'Canes Venatici',
  CMa: 'Canis Major', CMi: 'Canis Minor', Cap: 'Capricornus', Car: 'Carina', Cas: 'Cassiopeia',
  Cen: 'Centaurus', Cep: 'Cepheus', Cet: 'Cetus', Cha: 'Chamaeleon', Cir: 'Circinus', Col: 'Columba',
  Com: 'Coma Berenices', CrA: 'Corona Australis', CrB: 'Corona Borealis', Crv: 'Corvus', Crt: 'Crater',
  Cru: 'Crux', Cyg: 'Cygnus', Del: 'Delphinus', Dor: 'Dorado', Dra: 'Draco', Equ: 'Equuleus',
  Eri: 'Eridanus', For: 'Fornax', Gem: 'Gemini', Gru: 'Grus', Her: 'Hercules', Hor: 'Horologium',
  Hya: 'Hydra', Hyi: 'Hydrus', Ind: 'Indus', Lac: 'Lacerta', Leo: 'Leo', LMi: 'Leo Minor', Lep: 'Lepus',
  Lib: 'Libra', Lup: 'Lupus', Lyn: 'Lynx', Lyr: 'Lyra', Men: 'Mensa', Mic: 'Microscopium',
  Mon: 'Monoceros', Mus: 'Musca', Nor: 'Norma', Oct: 'Octans', Oph: 'Ophiuchus', Ori: 'Orion',
  Pav: 'Pavo', Peg: 'Pegasus', Per: 'Perseus', Phe: 'Phoenix', Pic: 'Pictor', Psc: 'Pisces',
  PsA: 'Piscis Austrinus', Pup: 'Puppis', Pyx: 'Pyxis', Ret: 'Reticulum', Sge: 'Sagitta',
  Sgr: 'Sagittarius', Sco: 'Scorpius', Scl: 'Sculptor', Sct: 'Scutum', Ser: 'Serpens', Se1: 'Serpens',
  Se2: 'Serpens', Sex: 'Sextans', Tau: 'Taurus', Tel: 'Telescopium', Tri: 'Triangulum',
  TrA: 'Triangulum Australe', Tuc: 'Tucana', UMa: 'Ursa Major', UMi: 'Ursa Minor', Vel: 'Vela',
  Vir: 'Virgo', Vol: 'Volans', Vul: 'Vulpecula',
};

const GREEK_NAMES = {
  α: 'alpha', β: 'beta', γ: 'gamma', δ: 'delta', ε: 'epsilon', ζ: 'zeta', η: 'eta', θ: 'theta',
  ι: 'iota', κ: 'kappa', λ: 'lambda', μ: 'mu', ν: 'nu', ξ: 'xi', ο: 'omicron', π: 'pi', ρ: 'rho',
  σ: 'sigma', τ: 'tau', υ: 'upsilon', φ: 'phi', χ: 'chi', ψ: 'psi', ω: 'omega',
};

// Lower case, no accents, no spaces or punctuation: "M 42" and "m42" match.
export function searchKey(text) {
  return text
    .normalize('NFD')
    .replace(/[̀-ͯ]/g, '')
    .toLowerCase()
    .replace(/[^a-z0-9α-ω]/g, '');
}

function greekSpelled(desig) {
  return desig.replace(/[α-ω]/g, (c) => GREEK_NAMES[c] + ' ');
}

const conName = (c) => CONSTELLATIONS[c] ?? c;

export const bodies = BODIES.map((b) => ({
  key: `body:${b}`,
  kind: 'body',
  body: b,
  title: b,
  subtitle: b === 'Sun' ? 'Star' : b === 'Moon' ? 'Moon' : 'Planet',
  search: searchKey(b),
}));

export const dsos = data.dso.map(([id, other, name, type, con, ra, dec, mag, size]) => ({
  key: `dso:${id}`,
  kind: 'dso',
  title: id,
  name,
  other,
  type,
  con,
  conName: conName(con),
  subtitle: [name, type].filter(Boolean).join(' · '),
  ra,
  dec,
  mag,
  size, // arcminutes
  messier: id.startsWith('M '),
  search: searchKey(`${id} ${other} ${name} ${type} ${conName(con)}`),
}));

export const stars = data.stars.map(([ra, dec, mag, name = '', desig = ''], i) => {
  const con = desig.split(' ')[1] ?? '';
  return {
    key: `star:${i}`,
    kind: 'star',
    title: name || desig,
    name,
    desig,
    con,
    conName: conName(con),
    subtitle: [name ? desig : '', 'Star'].filter(Boolean).join(' · '),
    ra,
    dec,
    mag,
    search: searchKey(`${name} ${desig} ${greekSpelled(desig)} ${conName(con)}`),
  };
});

// The stars worth a row in the list: those with a name, and the bright ones
// with a Bayer letter. All of them (to magnitude 5) are searchable and drawn.
export const listedStars = stars.filter((s) => s.name || (/^[α-ω]/.test(s.desig) && s.mag <= 3.5));

export const about = data.about;
