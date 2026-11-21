// Choosing alignment stars. The mount fits a rotation to the stars (protocol,
// align_add): two stars far apart fix it, a third checks it. Good stars are
// bright, between 15° and 70° up, and far from the stars already used.

import { bodies, stars } from './catalog.js';
import { apparent, bodyMagnitude, horizon, separation } from './sky.js';

export const MIN_ALT = 15;
export const MAX_ALT = 70; // the mount refuses to reset the axes above 75°
const MAG = 2.2;
const REPLACE = 10; // a star within 10° of an earlier one replaces it (protocol)

const brightStars = stars.filter((s) => s.name && s.mag <= MAG);
const planets = bodies.filter((b) => b.body !== 'Sun' && b.body !== 'Moon');

// Candidates now, each {o, ra, dec (of date), alt, az, mag, minSep, score},
// best first. `used` is the mount's align_list: [{ra, dec, name}].
export function candidates(utc, site, used = []) {
  const list = [];
  const add = (o, mag) => {
    const p = apparent(o, utc, site);
    const h = horizon(p.ra, p.dec, utc, site);
    if (h.alt < MIN_ALT || h.alt > MAX_ALT) return;
    const seps = used.map((u) => separation(p.ra, p.dec, u.ra, u.dec));
    const minSep = seps.length ? Math.min(...seps) : null;
    if (minSep != null && minSep < REPLACE) return; // would replace a star, not add one
    list.push({ o, ra: p.ra, dec: p.dec, alt: h.alt, az: h.az, mag, minSep, score: score(mag, h.alt, minSep) });
  };
  for (const s of brightStars) add(s, s.mag);
  for (const b of planets) {
    const mag = bodyMagnitude(b.body, utc);
    if (mag <= MAG) add(b, mag);
  }
  return list.sort((a, b) => b.score - a.score);
}

// Higher is better. Far from the stars already used matters most (up to
// 90°, beyond which nothing is gained), then brightness, then an altitude
// away from the horizon's refraction and the zenith.
function score(mag, alt, minSep) {
  const spread = minSep == null ? 0 : Math.min(minSep, 90) / 90; // 0..1
  const bright = (MAG - mag) / (MAG + 1.5); // about 0..1
  const height = 1 - Math.abs(alt - 45) / 30; // 1 at 45°, 0 at 15° and 75°
  return (minSep == null ? 0 : 3 * spread) + bright + 0.5 * height;
}

// "32° up, toward ESE": where to look for a star before the mount knows north.
const POINTS = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];
export function whereToLook(alt, az) {
  return `${Math.round(alt)}° up, toward ${POINTS[Math.round((((az % 360) + 360) % 360) / 22.5) % 16]}`;
}
