import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { Mount, MountError } from './mount.svelte.js';

// A fake mount behind a fake WebSocket: answers hello and set_time, and the
// test can reply, send events or stay silent.
let sockets;
class FakeSocket {
  readyState = 0;
  sent = [];
  constructor(url) {
    this.url = url;
    sockets.push(this);
    setTimeout(() => {
      this.readyState = 1;
      this.onopen?.();
    }, 10);
  }
  send(text) {
    const m = JSON.parse(text);
    this.sent.push(m);
    if (this.silent) return;
    if (m.cmd === 'hello') this.reply(m.id, { proto: 1, fw: '0.1.0', board: this.board ?? 'esp32s3', sim: this.board === 'sim' });
    else if (m.cmd === 'set_time') this.reply(m.id, { source: 'phone' });
    else if (m.cmd === 'stream') this.reply(m.id, {});
    else if (m.cmd === 'status') this.reply(m.id, { mode: 'idle', utc: 1 });
    else if (m.cmd === 'get_config') this.reply(m.id, { config: { 'sun.min_sep': 15 } });
  }
  reply(id, fields, ok = true) {
    this.receive({ re: id, ok, ...fields });
  }
  receive(m) {
    setTimeout(() => this.onmessage?.({ data: JSON.stringify(m) }), 1);
  }
  close() {
    if (this.readyState === 3) return;
    this.readyState = 3;
    setTimeout(() => this.onclose?.(), 1);
  }
}

beforeEach(() => {
  sockets = [];
  vi.useFakeTimers();
});
afterEach(() => vi.useRealTimers());

function start() {
  const m = new Mount({ url: 'ws://mount/ws', WebSocket: FakeSocket });
  m.start();
  return m;
}

describe('Mount', () => {
  it('says hello, sends its clock and reads the settings on connecting', async () => {
    const m = start();
    expect(m.link).toBe('connecting');
    await vi.advanceTimersByTimeAsync(50);
    expect(m.link).toBe('open');
    expect(m.info.fw).toBe('0.1.0');
    const cmds = sockets[0].sent.map((s) => s.cmd);
    expect(cmds).toEqual(['hello', 'set_time', 'get_config']);
    expect(m.config['sun.min_sep']).toBe(15);
    expect(sockets[0].sent[0].proto).toBe(1);
    m.stop();
  });

  it('keeps the status events and log events', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    sockets[0].receive({ ev: 'status', mode: 'tracking', utc: 5 });
    sockets[0].receive({ ev: 'log', level: 'warn', msg: 'target set' });
    await vi.advanceTimersByTimeAsync(5);
    expect(m.status).toEqual({ mode: 'tracking', utc: 5 });
    expect(m.logs[0]).toMatchObject({ level: 'warn', msg: 'target set' });
    m.stop();
  });

  it('matches replies to commands and rejects refusals', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    const s = sockets[0];
    s.silent = true;
    const a = expect(m.send('goto', { ra: 1, dec: 2 })).resolves.toMatchObject({ eta: 4 });
    const b = expect(m.send('park')).rejects.toMatchObject({ err: 'estop' });
    const [ga, gb] = s.sent.slice(-2);
    s.reply(gb.id, { err: 'estop', msg: 'E-stop pressed' }, false);
    s.reply(ga.id, { eta: 4 });
    await vi.advanceTimersByTimeAsync(5);
    await a;
    await b;
    m.stop();
  });

  it('times out a command without a reply', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    sockets[0].silent = true;
    const p = m.send('goto', {}, { timeout: 500 });
    const check = expect(p).rejects.toBeInstanceOf(MountError);
    await vi.advanceTimersByTimeAsync(600);
    await check;
    m.stop();
  });

  it('reconnects after the link closes, and fails pending commands', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    sockets[0].silent = true;
    const p = m.send('goto');
    const check = expect(p).rejects.toMatchObject({ err: 'disconnected' });
    sockets[0].close();
    await vi.advanceTimersByTimeAsync(5);
    await check;
    expect(m.link).toBe('closed');
    await vi.advanceTimersByTimeAsync(1100);
    expect(sockets.length).toBe(2);
    expect(m.link).toBe('open');
    m.stop();
  });

  it('drops a link that went silent', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    sockets[0].silent = true;
    await vi.advanceTimersByTimeAsync(4100);
    expect(sockets[0].readyState).toBe(3);
    expect(m.error).toMatch(/stopped answering/);
    m.stop();
  });

  it('polls the status in the simulator instead of streaming', async () => {
    FakeSocket.prototype.board = 'sim';
    try {
      const m = start();
      await vi.advanceTimersByTimeAsync(350);
      const cmds = sockets[0].sent.map((s) => s.cmd);
      expect(cmds.slice(0, 3)).toEqual(['hello', 'set_time', 'stream']);
      expect(sockets[0].sent[2].hz).toBe(0);
      expect(cmds.filter((c) => c === 'status').length).toBeGreaterThanOrEqual(2);
      expect(m.status.mode).toBe('idle');
      m.stop();
    } finally {
      delete FakeSocket.prototype.board;
    }
  });

  it('sends jogs without an id', async () => {
    const m = start();
    await vi.advanceTimersByTimeAsync(50);
    expect(m.post('jog', { x: 0.5, y: 0 })).toBe(true);
    expect(sockets[0].sent.at(-1)).toEqual({ cmd: 'jog', x: 0.5, y: 0 });
    m.stop();
    expect(m.post('jog', { x: 0, y: 0 })).toBe(false);
  });
});
