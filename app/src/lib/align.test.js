import { describe, expect, it } from 'vitest';
import { candidates, MAX_ALT, MIN_ALT, whereToLook } from './align.js';
import { separation } from './sky.js';

const site = { lat: 45, lon: 5, elev: 0 };
const utc = Date.UTC(2026, 10, 21, 21, 0, 0);

describe('alignment stars', () => {
  it('suggests bright stars at a useful altitude', () => {
    const c = candidates(utc, site);
    expect(c.length).toBeGreaterThan(5);
    for (const x of c) {
      expect(x.alt).toBeGreaterThanOrEqual(MIN_ALT);
      expect(x.alt).toBeLessThanOrEqual(MAX_ALT);
      expect(x.mag).toBeLessThanOrEqual(2.2);
    }
  });

  it('then prefers stars far from the ones used', () => {
    const first = candidates(utc, site)[0];
    const next = candidates(utc, site, [{ ra: first.ra, dec: first.dec, name: first.o.title }]);
    expect(next.find((x) => x.o.key === first.o.key)).toBeUndefined(); // within 10°: would replace it
    expect(next[0].minSep).toBeGreaterThan(60);
    expect(next[0].altDiff).toBeGreaterThan(10); // at another height, for the altitude index
    expect(separation(next[0].ra, next[0].dec, first.ra, first.dec)).toBeCloseTo(next[0].minSep, 6);
  });

  it('says where to look', () => {
    expect(whereToLook(32.4, 112)).toBe('32° up, toward ESE');
    expect(whereToLook(10, 359)).toBe('10° up, toward N');
  });
});
