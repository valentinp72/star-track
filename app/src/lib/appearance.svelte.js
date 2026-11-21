// The app's look: 'auto' follows the phone (light or dark), 'dark', or 'night'
// (dark, red accents, and a red filter over the whole screen to keep dark
// adaptation). Kept in the phone's storage for this site.

const KEY = 'star-track.appearance';
const CHOICES = ['auto', 'dark', 'night'];

function load() {
  try {
    const v = localStorage.getItem(KEY);
    return CHOICES.includes(v) ? v : 'auto';
  } catch {
    return 'auto';
  }
}

const media = typeof matchMedia === 'function' ? matchMedia('(prefers-color-scheme: dark)') : null;

class Appearance {
  choice = $state(load());
  #systemDark = $state(media?.matches ?? true);

  constructor() {
    media?.addEventListener('change', (e) => (this.#systemDark = e.matches));
  }

  get dark() {
    return this.choice !== 'auto' || this.#systemDark;
  }

  get night() {
    return this.choice === 'night';
  }

  set(choice) {
    if (!CHOICES.includes(choice)) return;
    this.choice = choice;
    try {
      localStorage.setItem(KEY, choice);
    } catch {
      // private mode: not kept
    }
  }
}

export const appearance = new Appearance();
