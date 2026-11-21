import { describe, expect, it } from 'vitest';
import { apparent, horizon, quickHorizon, riseSet, separation, sun, trackTable } from './sky.js';

const site = { lat: 45, lon: 5, elev: 200 };
const utc = Date.UTC(2026, 10, 21, 21, 0, 0); // 21 Nov 2026, 21:00 UTC

describe('sky', () => {
  it('precesses a J2000 star to the date', () => {
    const vega = { ra: 18.61576, dec: 38.7858 };
    const p = apparent(vega, utc, site);
    const moved = separation(p.ra, p.dec, vega.ra, vega.dec);
    // about 26 years of precession (50″/yr in longitude), plus nutation and aberration
    expect(moved).toBeGreaterThan(0.1);
    expect(moved).toBeLessThan(0.5);
  });

  it('gives the Moon from the site, not from the centre of the Earth', () => {
    const topo = apparent({ body: 'Moon' }, utc, site);
    const geoSite = apparent({ body: 'Moon' }, utc, { lat: 0, lon: 0, elev: -6378000 });
    expect(Number.isFinite(topo.ra)).toBe(true);
    // the Moon's parallax is about 1°: two sites see it in different places
    const other = apparent({ body: 'Moon' }, utc, { lat: -45, lon: 5 });
    expect(separation(topo.ra, topo.dec, other.ra, other.dec)).toBeGreaterThan(0.5);
    expect(geoSite).toBeTruthy();
  });

  it('quick alt/az agrees with the full computation', () => {
    const q = quickHorizon(utc, site);
    for (const o of [{ ra: 18.61576, dec: 38.7858 }, { ra: 5.58791, dec: -5.3897 }, { ra: 2.5303, dec: 89.264 }]) {
      const p = apparent(o, utc, site);
      const full = horizon(p.ra, p.dec, utc, site);
      const fast = q(o.ra, o.dec);
      expect(Math.abs(fast.alt - full.alt)).toBeLessThan(full.alt > 10 ? 0.15 : 0.6);
      const daz = Math.abs(((fast.az - full.az + 540) % 360) - 180) * Math.cos((full.alt * Math.PI) / 180);
      expect(daz).toBeLessThan(0.15);
    }
  });

  it('makes a table for track_table that covers the next hours', () => {
    const pts = trackTable('Jupiter', utc, site);
    expect(pts.length).toBeLessThanOrEqual(64);
    expect(pts[0][0]).toBeLessThan(utc);
    expect(pts.at(-1)[0]).toBeGreaterThanOrEqual(utc + 6 * 3600000);
    for (let i = 1; i < pts.length; i++) expect(pts[i][0] - pts[i - 1][0]).toBe(600000);
    expect(JSON.stringify({ cmd: 'track_table', id: 1, points: pts }).length).toBeLessThan(4096);
  });

  it('finds rise, set and transit', () => {
    const r = riseSet({ body: 'Sun' }, utc, site);
    expect(r.rise).toBeGreaterThan(utc);
    expect(r.set).toBeGreaterThan(utc);
    expect(r.transitAlt).toBeGreaterThan(20);
    expect(r.transitAlt).toBeLessThan(30); // late November at 45° N
    const polaris = riseSet({ ra: 2.5303, dec: 89.264 }, utc, site);
    expect(polaris.rise).toBeNull(); // always up
  });

  it('puts the Sun below the horizon at night', () => {
    expect(sun(utc, site).alt).toBeLessThan(-18);
  });
});
