// The connection to the mount (or the simulator): one WebSocket to /ws, JSON
// messages as in docs/protocol.md. Reconnects by itself, and exposes the
// mount's state as Svelte state for the views.

export const PROTO = 1;

// Delays in ms.
const REPLY_TIMEOUT = 5000;
const STALE_AFTER = 3000; // no message for this long: the link is dead
const RETRY_MIN = 1000;
const RETRY_MAX = 5000;
const POLL_EVERY = 100; // status polling in the simulator

// A command refused by the mount (`ok: false`), or not answered.
export class MountError extends Error {
  constructor(err, msg) {
    super(msg || err);
    this.err = err; // a code from the protocol's Errors table, or `timeout`, `disconnected`
  }
}

export class Mount {
  // 'connecting', 'open' (hello answered) or 'closed' (waiting to retry)
  link = $state('connecting');
  info = $state(null); // the hello reply: proto, fw, board, sim
  status = $state(null); // the last status event or reply
  logs = $state([]); // the last log events, newest first
  error = $state(null); // the last connection problem, as text

  #url;
  #WebSocket;
  #ws = null;
  #nextId = 1;
  #pending = new Map(); // id -> {resolve, reject, timer}
  #lastMessage = 0;
  #retryDelay = RETRY_MIN;
  #retryTimer = null;
  #watchdog = null;
  #pollTimer = null;
  #polling = false;
  #stopped = false;

  // `url` defaults to /ws on the server that served the page. `WebSocket` can be
  // replaced for the tests.
  constructor({ url, WebSocket: WS } = {}) {
    this.#url = url ?? `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws`;
    this.#WebSocket = WS ?? globalThis.WebSocket;
  }

  get connected() {
    return this.link === 'open';
  }

  start() {
    this.#stopped = false;
    this.#open();
    this.#watchdog = setInterval(() => this.#checkStale(), 1000);
  }

  stop() {
    this.#stopped = true;
    clearInterval(this.#watchdog);
    clearTimeout(this.#retryTimer);
    this.#close('stopped');
  }

  // When the app comes back to the foreground. iOS suspends a page in the
  // background and its socket may be dead without a close event.
  resume() {
    if (this.#stopped) return;
    if (this.link === 'closed') {
      clearTimeout(this.#retryTimer);
      this.#retryDelay = RETRY_MIN;
      this.#open();
    } else if (this.link === 'open') {
      this.#checkStale();
      if (this.link === 'open') this.send('set_time', { utc: Date.now() }).catch(() => {});
    }
  }

  // Sends a command with an id and resolves with its reply (rejects with a
  // MountError if refused, unanswered or the link is down).
  send(cmd, args = {}, { timeout = REPLY_TIMEOUT } = {}) {
    const ws = this.#ws;
    if (!ws || ws.readyState !== 1) return Promise.reject(new MountError('disconnected', 'not connected'));
    const id = this.#nextId++;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.#pending.delete(id);
        reject(new MountError('timeout', `no reply to ${cmd}`));
      }, timeout);
      this.#pending.set(id, { resolve, reject, timer });
      ws.send(JSON.stringify({ cmd, id, ...args }));
    });
  }

  // Sends a command without an id: no reply (used for jog).
  post(cmd, args = {}) {
    const ws = this.#ws;
    if (!ws || ws.readyState !== 1) return false;
    ws.send(JSON.stringify({ cmd, ...args }));
    return true;
  }

  #open() {
    this.link = 'connecting';
    let ws;
    try {
      ws = new this.#WebSocket(this.#url);
    } catch (e) {
      this.error = String(e.message ?? e);
      this.#scheduleRetry();
      return;
    }
    this.#ws = ws;
    this.#lastMessage = Date.now();
    ws.onopen = () => this.#greet(ws);
    ws.onmessage = (e) => this.#receive(e.data);
    ws.onclose = () => {
      if (this.#ws !== ws) return;
      this.#close('closed');
      this.#scheduleRetry();
    };
    ws.onerror = () => {
      if (this.#ws === ws) this.error = 'Cannot reach the mount.';
    };
  }

  async #greet(ws) {
    try {
      const hello = await this.send('hello', { proto: PROTO });
      if (this.#ws !== ws) return;
      await this.send('set_time', { utc: Date.now() });
      if (this.#ws !== ws) return;
      // The simulator's clock may run 1000 times faster, and so would its
      // status events: poll in real time instead.
      if (hello.sim) {
        await this.send('stream', { hz: 0 });
        this.#startPolling(ws);
      }
      this.info = hello;
      this.error = hello.proto === PROTO ? null : `The mount speaks protocol ${hello.proto}, the app expects ${PROTO}: some commands may fail.`;
      this.link = 'open';
      this.#retryDelay = RETRY_MIN;
    } catch (e) {
      if (this.#ws !== ws) return;
      this.error = `No answer from the mount (${e.message}).`;
      ws.close();
    }
  }

  #startPolling(ws) {
    this.#pollTimer = setInterval(async () => {
      if (this.#polling || this.#ws !== ws) return;
      this.#polling = true;
      try {
        const { re, ok, ...st } = await this.send('status');
        if (this.#ws === ws) this.status = st;
      } catch {
        // the watchdog deals with a dead link
      } finally {
        this.#polling = false;
      }
    }, POLL_EVERY);
  }

  #receive(text) {
    this.#lastMessage = Date.now();
    let m;
    try {
      m = JSON.parse(text);
    } catch {
      return;
    }
    if ('re' in m) {
      const p = this.#pending.get(m.re);
      if (!p) return;
      this.#pending.delete(m.re);
      clearTimeout(p.timer);
      if (m.ok) p.resolve(m);
      else p.reject(new MountError(m.err, m.msg));
    } else if (m.ev === 'status') {
      const { ev, ...st } = m;
      this.status = st;
    } else if (m.ev === 'log') {
      this.logs = [{ level: m.level, msg: m.msg, at: Date.now() }, ...this.logs].slice(0, 50);
    }
  }

  #checkStale() {
    if (this.#ws && this.link !== 'closed' && Date.now() - this.#lastMessage > STALE_AFTER) {
      this.error = 'The mount stopped answering.';
      this.#close('closed');
      this.#scheduleRetry();
    }
  }

  #close(link) {
    const ws = this.#ws;
    this.#ws = null;
    clearInterval(this.#pollTimer);
    this.#polling = false;
    if (ws) {
      ws.onopen = ws.onmessage = ws.onclose = ws.onerror = null;
      try {
        ws.close();
      } catch {
        // already closed
      }
    }
    for (const p of this.#pending.values()) {
      clearTimeout(p.timer);
      p.reject(new MountError('disconnected', 'connection lost'));
    }
    this.#pending.clear();
    this.status = null; // old positions would mislead
    this.link = link === 'stopped' ? 'closed' : link;
  }

  #scheduleRetry() {
    if (this.#stopped) return;
    this.link = 'closed';
    clearTimeout(this.#retryTimer);
    this.#retryTimer = setTimeout(() => this.#open(), this.#retryDelay);
    this.#retryDelay = Math.min(this.#retryDelay * 2, RETRY_MAX);
  }
}
