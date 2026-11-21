import { beforeEach, describe, expect, it, vi } from 'vitest';

beforeEach(() => {
  localStorage.clear();
  vi.resetModules();
});

describe('saved sites', () => {
  it('keeps sites by name, sorted, on the phone', async () => {
    const { sites } = await import('./sites.svelte.js');
    sites.put({ name: 'Garden', lat: 46.2, lon: 6.1, elev: 400 });
    sites.put({ name: ' Alps ', lat: 45.9, lon: 6.9, elev: 1800 });
    sites.put({ name: 'Garden', lat: 46.3, lon: 6.1, elev: 400 }); // replaced
    expect(sites.list.map((s) => s.name)).toEqual(['Alps', 'Garden']);
    expect(sites.list[1].lat).toBe(46.3);
    vi.resetModules();
    const again = (await import('./sites.svelte.js')).sites;
    expect(again.list.length).toBe(2);
    again.remove('Alps');
    expect(again.list.map((s) => s.name)).toEqual(['Garden']);
  });

  it('ignores a site without a name', async () => {
    const { sites } = await import('./sites.svelte.js');
    sites.put({ name: '  ', lat: 1, lon: 2 });
    expect(sites.list).toEqual([]);
  });
});
