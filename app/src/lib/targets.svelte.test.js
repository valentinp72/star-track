import { describe, expect, it } from 'vitest';
import { Targets } from './targets.svelte.js';
import { MountError } from './mount.svelte.js';

const site = { lat: 45, lon: 5, elev: 0 };
const T0 = Date.UTC(2026, 10, 21, 21, 0, 0);

// Effects created outside a component run on the next tick (flushSync does not
// reach them here).
const tick = () => new Promise((r) => setTimeout(r, 0));

// A mount that records what it is sent and answers from `answer`.
class FakeMount {
  status = $state({ utc: T0, mode: 'idle', site, target: null });
  sent = [];
  answer = () => ({ ok: true, eta: 5 });
  async send(cmd, args = {}) {
    this.sent.push({ cmd, ...args });
    const r = this.answer(cmd, args);
    if (!r.ok) throw new MountError(r.err, r.msg);
    return r;
  }
}

describe('Targets', () => {
  it('sends a fixed object as a goto of date', async () => {
    const m = new FakeMount();
    const t = new Targets(m);
    await t.goto({ ra: 5.58791, dec: -5.3897, title: 'M 42' });
    expect(m.sent[0].cmd).toBe('goto');
    expect(m.sent[0].track).toBe(true);
    expect(Math.abs(m.sent[0].ra - 5.58791)).toBeGreaterThan(0.001); // precessed
    expect(t.current.obj.title).toBe('M 42');
  });

  it('sends a planet as a table, and renews it before it ends', async () => {
    const m = new FakeMount();
    const t = new Targets(m);
    await t.goto({ body: 'Mars' });
    expect(m.sent[0].cmd).toBe('track_table');
    const end = t.current.tableEnd;
    m.status = { ...m.status, mode: 'tracking', target: { ra: 1, dec: 1 }, utc: T0 + 3600000 };
    await tick();
    expect(m.sent.length).toBe(1); // not yet
    m.status = { ...m.status, utc: end - 3000000 };
    await tick();
    expect(m.sent.length).toBe(2);
    expect(m.sent[1].cmd).toBe('track_table');
    expect(m.sent[1].points[0][0]).toBeLessThan(end - 3000000);
    expect(t.current.tableEnd).toBeGreaterThan(end);
  });

  it('goes to the Moon once without tracking', async () => {
    const m = new FakeMount();
    const t = new Targets(m);
    await t.goto({ body: 'Moon' }, { track: false });
    expect(m.sent[0]).toMatchObject({ cmd: 'goto', track: false });
  });

  it('goes to the Sun with the mount own position', async () => {
    const m = new FakeMount();
    const t = new Targets(m);
    await t.goto({ body: 'Sun' });
    expect(m.sent[0]).toEqual({ cmd: 'goto', sun: true, track: true });
  });

  it('forgets the target when the mount has none', async () => {
    const m = new FakeMount();
    const t = new Targets(m);
    await t.goto({ ra: 1, dec: 2 });
    m.status = { ...m.status, mode: 'idle', target: null };
    await tick();
    expect(t.current).toBeNull();
  });

  it('passes refusals on', async () => {
    const m = new FakeMount();
    m.answer = () => ({ ok: false, err: 'not_ready', msg: 'aligned is false' });
    const t = new Targets(m);
    await expect(t.goto({ ra: 1, dec: 2 })).rejects.toMatchObject({ err: 'not_ready' });
    expect(t.current).toBeNull();
  });
});
