// Sites kept on the phone: [{name, lat, lon, elev}], by name.

const KEY = 'star-track.sites';

function load() {
  try {
    const v = JSON.parse(localStorage.getItem(KEY));
    return Array.isArray(v) ? v.filter((s) => s && typeof s.name === 'string' && Number.isFinite(s.lat) && Number.isFinite(s.lon)) : [];
  } catch {
    return [];
  }
}

class Sites {
  list = $state(load());

  #save() {
    try {
      localStorage.setItem(KEY, JSON.stringify(this.list));
    } catch {
      // not kept
    }
  }

  // Adds the site, or replaces the one with the same name.
  put(site) {
    const name = site.name.trim();
    if (!name) return;
    this.list = [...this.list.filter((s) => s.name !== name), { ...site, name }].sort((a, b) => a.name.localeCompare(b.name));
    this.#save();
  }

  remove(name) {
    this.list = this.list.filter((s) => s.name !== name);
    this.#save();
  }
}

export const sites = new Sites();
