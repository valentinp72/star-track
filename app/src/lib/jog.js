// The joystick's messages: `jog` every 100 ms while a finger is on it (the
// mount stops by itself when they stop: dead-man, 200 ms), and `jog 0, 0` at
// once when it lifts. No id, no reply (protocol).

export const JOG_EVERY = 100;

// Speed choices: a factor on the joystick's deflection. The mount maps the
// round joystick onto the square, then squares each axis's value
// (speed = jog.max_speed × |value|²), so 0.3 is 9 % of full speed.
export const SPEEDS = [
  { id: 'fine', label: 'Fine', factor: 0.3 },
  { id: 'medium', label: 'Medium', factor: 0.6 },
  { id: 'fast', label: 'Fast', factor: 1 },
];

export class Jogger {
  #mount;
  #timer = null;
  #x = 0;
  #y = 0;
  factor = 1;

  constructor(mount) {
    this.#mount = mount;
  }

  // From the joystick: x, y from −1 to 1; both 0 when released.
  set(x, y) {
    this.#x = x;
    this.#y = y;
    if (x === 0 && y === 0) {
      this.release();
      return;
    }
    if (this.#timer == null) {
      this.#send();
      this.#timer = setInterval(() => this.#send(), JOG_EVERY);
    }
  }

  release() {
    const was = this.#timer != null;
    clearInterval(this.#timer);
    this.#timer = null;
    this.#x = this.#y = 0;
    if (was) this.#mount.post('jog', { x: 0, y: 0 });
  }

  #send() {
    const r = (v) => Math.round(v * this.factor * 100) / 100;
    this.#mount.post('jog', { x: r(this.#x), y: r(this.#y) });
  }
}
