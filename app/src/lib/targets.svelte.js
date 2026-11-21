// Sends gotos, and keeps the Moon's and planets' position tables up to date
// while the mount follows them (the mount has no planet theory: it
// interpolates the table the app sends).

import { untrack } from 'svelte';
import { apparent, trackTable } from './sky.js';

const RENEW_BEFORE = 3600000; // send a new table 1 h before the old one ends

export class Targets {
  // The object of the last goto, while the mount still has a target.
  current = $state.raw(null); // {obj, track, tableEnd, seen}
  #mount;
  #renewing = false;

  constructor(mount) {
    this.#mount = mount;
    // Follow the status: forget the target once the mount has none, renew the
    // table in time. Runs on each new status only, not when `current` changes.
    // The status from before a goto has no target yet, so it is skipped;
    // messages keep their order on the link, so a status received after the
    // goto's reply shows the goto.
    $effect.root(() => {
      $effect(() => {
        const st = mount.status;
        untrack(() => this.#onStatus(st));
      });
    });
  }

  // Goes to `obj` and tracks it (or not). Resolves with the reply (eta) or
  // rejects with a MountError. The Sun needs solar mode (the caller asks for
  // the filter check first).
  async goto(obj, { track = true } = {}) {
    const st = this.#mount.status;
    const seen = st; // older than the goto
    const site = st?.site;
    const utc = st?.utc ?? Date.now();
    let reply;
    if (obj.body === 'Sun') {
      reply = await this.#mount.send('goto', { sun: true, track });
      this.current = { obj, track, tableEnd: null, seen };
    } else if (obj.body && track) {
      if (!site) throw notReady();
      const points = trackTable(obj.body, utc, site);
      reply = await this.#mount.send('track_table', { points });
      this.current = { obj, track, tableEnd: points.at(-1)[0], seen };
    } else {
      if (!site) throw notReady();
      const p = apparent(obj, utc, site);
      reply = await this.#mount.send('goto', { ra: round(p.ra, 6), dec: round(p.dec, 5), track });
      this.current = { obj, track, tableEnd: null, seen };
    }
    return reply;
  }

  #onStatus(st) {
    const cur = this.current;
    if (!cur || !st || st === cur.seen) return;
    if (!st.target && st.mode !== 'goto') {
      this.current = null;
      return;
    }
    const following = st.mode === 'tracking' || st.mode === 'manual' || st.mode === 'goto';
    if (cur.tableEnd && following && st.utc > cur.tableEnd - RENEW_BEFORE && !this.#renewing && st.site) {
      this.#renewing = true;
      const points = trackTable(cur.obj.body, st.utc, st.site);
      this.#mount
        .send('track_table', { points })
        .then(() => {
          if (this.current === cur) this.current = { ...cur, tableEnd: points.at(-1)[0] };
        })
        .catch(() => {})
        .finally(() => (this.#renewing = false));
    }
  }
}

const round = (x, n) => Number(x.toFixed(n));

function notReady() {
  const e = new Error('site_ok is false');
  e.err = 'not_ready';
  return e;
}
