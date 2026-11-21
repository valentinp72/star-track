// The joystick speed choice (Fine, Medium, Fast), shared by every joystick in
// the app and kept on the phone.
import { SPEEDS } from './jog.js';

const KEY = 'star-track.jog-speed';

function load() {
  try {
    return SPEEDS.find((s) => s.id === localStorage.getItem(KEY))?.id ?? 'medium';
  } catch {
    return 'medium';
  }
}

class JogSpeed {
  id = $state(load());
  get factor() {
    return SPEEDS.find((s) => s.id === this.id).factor;
  }
  set(id) {
    this.id = id;
    try {
      localStorage.setItem(KEY, id);
    } catch {
      // not kept
    }
  }
}

export const jogSpeed = new JogSpeed();
