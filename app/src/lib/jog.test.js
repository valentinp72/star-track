import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { Jogger } from './jog.js';

beforeEach(() => vi.useFakeTimers());
afterEach(() => vi.useRealTimers());

const fakeMount = () => ({ sent: [], post(cmd, a) { this.sent.push({ cmd, ...a }); return true; } });

describe('Jogger', () => {
  it('repeats the jog every 100 ms while held, then stops at once', () => {
    const m = fakeMount();
    const j = new Jogger(m);
    j.set(0.5, -1);
    expect(m.sent).toEqual([{ cmd: 'jog', x: 0.5, y: -1 }]);
    vi.advanceTimersByTime(350);
    expect(m.sent.length).toBe(4);
    j.set(0.2, 0.1); // the finger moves: the next message has the new values
    vi.advanceTimersByTime(100);
    expect(m.sent.at(-1)).toEqual({ cmd: 'jog', x: 0.2, y: 0.1 });
    j.set(0, 0);
    expect(m.sent.at(-1)).toEqual({ cmd: 'jog', x: 0, y: 0 });
    const n = m.sent.length;
    vi.advanceTimersByTime(1000);
    expect(m.sent.length).toBe(n);
  });

  it('scales by the speed choice', () => {
    const m = fakeMount();
    const j = new Jogger(m);
    j.factor = 0.3;
    j.set(1, -0.5);
    expect(m.sent[0]).toEqual({ cmd: 'jog', x: 0.3, y: -0.15 });
    j.release();
  });

  it('sends nothing on release when it was not held', () => {
    const m = fakeMount();
    new Jogger(m).release();
    expect(m.sent).toEqual([]);
  });
});
