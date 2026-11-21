import { describe, expect, it } from 'vitest';
import { bodies, dsos, listedStars, searchKey, stars, wikipediaUrl } from './catalog.js';

describe('catalog', () => {
  it('has the Messier objects and the bright stars', () => {
    expect(dsos.filter((d) => d.messier).length).toBe(109);
    expect(stars.length).toBeGreaterThan(1500);
    expect(bodies.map((b) => b.body)).toContain('Moon');
    const m42 = dsos.find((d) => d.title === 'M 42');
    expect(m42.name).toBe('Orion Nebula');
    expect(m42.conName).toBe('Orion');
    const vega = stars.find((s) => s.name === 'Vega');
    expect(vega.desig).toBe('α Lyr');
    expect(vega.conName).toBe('Lyra');
    expect(listedStars).toContain(vega);
  });

  it('links to Wikipedia', () => {
    expect(dsos.find((d) => d.title === 'M 42').wiki).toBe('Orion Nebula');
    expect(stars.find((s) => s.name === 'Vega').wiki).toBe('Vega');
    expect(stars.find((s) => s.name === 'Mirfak').wiki).toBe('Alpha Persei');
    expect(bodies.find((b) => b.body === 'Mercury').wiki).toBe('Mercury (planet)');
    expect(wikipediaUrl('Alpha Persei')).toBe('https://en.wikipedia.org/wiki/Alpha_Persei');
  });

  it('has sane coordinates', () => {
    for (const o of [...dsos, ...stars]) {
      expect(o.ra).toBeGreaterThanOrEqual(0);
      expect(o.ra).toBeLessThan(24);
      expect(Math.abs(o.dec)).toBeLessThanOrEqual(90);
    }
  });

  it('searches without spaces, case or accents', () => {
    expect(searchKey('M 42')).toBe('m42');
    const find = (q) => [...dsos, ...stars].filter((o) => o.search.includes(searchKey(q))).map((o) => o.title);
    expect(find('m42')).toContain('M 42');
    expect(find('ngc 1976')).toContain('M 42');
    expect(find('alpha lyr')).toContain('Vega');
    expect(find('bootes')).toContain('Arcturus');
    expect(find('andromeda galaxy')).toContain('M 31');
  });
});
