// Positions for the mount, from Astronomy Engine. The protocol wants
// topocentric apparent RA/Dec of date: the app applies precession, nutation,
// aberration and, for the Moon, parallax; the mount applies none of them.

import * as A from 'astronomy-engine';

export const BODIES = ['Sun', 'Moon', 'Mercury', 'Venus', 'Mars', 'Jupiter', 'Saturn', 'Uranus', 'Neptune'];

const D = Math.PI / 180;
const observer = (site) => new A.Observer(site.lat, site.lon, site.elev ?? 0);

// The object's RA (hours) and Dec (degrees) of date as seen from the site.
// `obj` is a body ({body: 'Moon'}) or a fixed object with J2000 coordinates
// ({ra, dec}).
export function apparent(obj, utc, site) {
  const date = new Date(utc);
  let body = obj.body;
  if (!body) {
    // A user-defined star gets the same corrections as the planets. 1000 light
    // years: no parallax.
    A.DefineStar(A.Body.Star1, obj.ra, obj.dec, 1000);
    body = A.Body.Star1;
  }
  const e = A.Equator(body, date, observer(site), true, true);
  return { ra: e.ra, dec: e.dec };
}

// Altitude (with refraction, as the mount reports it) and azimuth of an
// apparent RA/Dec of date.
export function horizon(ra, dec, utc, site) {
  const h = A.Horizon(new Date(utc), observer(site), ra, dec, 'normal');
  return { alt: h.altitude, az: h.azimuth };
}

// For long lists: J2000 positions -> rough alt/az (precession, no nutation or
// aberration, no refraction; a few arcminutes at most). Returns a function.
export function quickHorizon(utc, site) {
  const time = A.MakeTime(new Date(utc));
  const rot = A.Rotation_EQJ_EQD(time);
  const lst = ((A.SiderealTime(time) + site.lon / 15) % 24) * 15 * D;
  const sl = Math.sin(site.lat * D);
  const cl = Math.cos(site.lat * D);
  return (ra, dec) => {
    const a = ra * 15 * D;
    const d = dec * D;
    const x = Math.cos(d) * Math.cos(a);
    const y = Math.cos(d) * Math.sin(a);
    const z = Math.sin(d);
    const m = rot.rot;
    const X = m[0][0] * x + m[1][0] * y + m[2][0] * z;
    const Y = m[0][1] * x + m[1][1] * y + m[2][1] * z;
    const Z = m[0][2] * x + m[1][2] * y + m[2][2] * z;
    const raD = Math.atan2(Y, X);
    const decD = Math.asin(Math.max(-1, Math.min(1, Z)));
    const ha = lst - raD;
    const alt = Math.asin(sl * Math.sin(decD) + cl * Math.cos(decD) * Math.cos(ha)) / D;
    const az = Math.atan2(-Math.cos(decD) * Math.sin(ha), Math.sin(decD) * cl - Math.cos(decD) * Math.cos(ha) * sl) / D;
    return { alt, az: (az + 360) % 360 };
  };
}

// Angle between two RA/Dec directions, degrees.
export function separation(ra1, dec1, ra2, dec2) {
  const a1 = ra1 * 15 * D;
  const a2 = ra2 * 15 * D;
  const d1 = dec1 * D;
  const d2 = dec2 * D;
  const c = Math.sin(d1) * Math.sin(d2) + Math.cos(d1) * Math.cos(d2) * Math.cos(a1 - a2);
  return Math.acos(Math.max(-1, Math.min(1, c))) / D;
}

// The table of positions sent with track_table for the Moon and planets:
// every `stepMin` minutes from 10 minutes ago, `hours` ahead. At most 64 points
// (protocol).
export function trackTable(body, utc, site, { stepMin = 10, hours = 6 } = {}) {
  const step = stepMin * 60000;
  const n = Math.round((hours * 60) / stepMin) + 1;
  if (n + 1 > 64) throw new Error('track_table: at most 64 points');
  const t0 = utc - step;
  const points = [];
  for (let k = 0; k <= n; k++) {
    const t = Math.round(t0 + k * step);
    const p = apparent({ body }, t, site);
    points.push([t, Number(p.ra.toFixed(6)), Number(p.dec.toFixed(5))]);
  }
  return points;
}

// Next rise, set and highest point within a day, as UTC ms (null when there
// is none: always up, or never up).
export function riseSet(obj, utc, site) {
  const date = new Date(utc);
  const obs = observer(site);
  let body = obj.body;
  if (!body) {
    A.DefineStar(A.Body.Star1, obj.ra, obj.dec, 1000);
    body = A.Body.Star1;
  }
  const ms = (t) => (t ? t.date.getTime() : null);
  const transit = A.SearchHourAngle(body, obs, 0, date, +1);
  return {
    rise: ms(A.SearchRiseSet(body, obs, +1, date, 1)),
    set: ms(A.SearchRiseSet(body, obs, -1, date, 1)),
    transit: transit ? transit.time.date.getTime() : null,
    transitAlt: transit ? transit.hor.altitude : null,
  };
}

// The Sun's apparent RA/Dec and alt/az.
export function sun(utc, site) {
  const p = apparent({ body: 'Sun' }, utc, site);
  return { ...p, ...horizon(p.ra, p.dec, utc, site) };
}

// Visual magnitude of a body now (Astronomy Engine; none for the Sun).
export function bodyMagnitude(body, utc) {
  if (body === 'Sun') return -26.7;
  return A.Illumination(body, new Date(utc)).mag;
}
