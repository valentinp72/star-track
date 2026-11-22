// Text for coordinates, times and mount states.

const pad = (n, w = 2) => String(n).padStart(w, '0');

// Right ascension in hours -> "05h 35m 17s".
export function formatRa(h) {
  if (h == null) return '—';
  let s = Math.round((((h % 24) + 24) % 24) * 3600);
  if (s >= 86400) s -= 86400;
  return `${pad(Math.floor(s / 3600))}h ${pad(Math.floor(s / 60) % 60)}m ${pad(s % 60)}s`;
}

// Declination (or any signed angle) in degrees -> "−05° 23′ 28″".
export function formatDec(d) {
  if (d == null) return '—';
  const sign = d < 0 ? '−' : '+';
  const s = Math.round(Math.abs(d) * 3600);
  return `${sign}${pad(Math.floor(s / 3600))}° ${pad(Math.floor(s / 60) % 60)}′ ${pad(s % 60)}″`;
}

// An angle in degrees with one decimal: "23.4°", "−1.2°".
export function formatDeg(d, digits = 1) {
  if (d == null) return '—';
  const t = d.toFixed(digits);
  return `${t.startsWith('-') && Number(t) !== 0 ? '−' + t.slice(1) : t.replace(/^-/, '')}°`;
}

// A small angle in the unit that suits it: "42″", "3.1′", "1.25°".
export function formatAngle(d) {
  if (d == null) return '—';
  const a = Math.abs(d);
  if (a < 1 / 60) return `${Math.round(a * 3600)}″`;
  if (a < 1) return `${(a * 60).toFixed(1)}′`;
  return `${a.toFixed(2)}°`;
}

// Azimuth with the compass point: "162.0° SSE".
const POINTS = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];
export function formatAz(a) {
  if (a == null) return '—';
  const n = ((a % 360) + 360) % 360;
  return `${formatDeg(n)} ${POINTS[Math.round(n / 22.5) % 16]}`;
}

// Latitude and longitude: "45.000° N", "5.000° E".
export function formatLat(d) {
  return d == null ? '—' : `${Math.abs(d).toFixed(3)}° ${d < 0 ? 'S' : 'N'}`;
}
export function formatLon(d) {
  return d == null ? '—' : `${Math.abs(d).toFixed(3)}° ${d < 0 ? 'W' : 'E'}`;
}

// The mount's clock (UTC ms) in the phone's time zone: "22:41:07".
export function formatClock(utc) {
  if (utc == null) return '—';
  const t = new Date(utc);
  return `${pad(t.getHours())}:${pad(t.getMinutes())}:${pad(t.getSeconds())}`;
}

// Hours and minutes in the phone's time zone: "22:41".
export function formatTime(utc) {
  if (utc == null) return '—';
  const t = new Date(utc);
  return `${pad(t.getHours())}:${pad(t.getMinutes())}`;
}

export const MODES = {
  estop: 'E-stop',
  idle: 'Idle',
  manual: 'Manual',
  goto: 'Slewing',
  tracking: 'Tracking',
  parked: 'Parked',
};

// Seconds since the mount started: "45 s", "12 min", "3 h 05 min", "2 d 4 h".
export function formatUptime(s) {
  if (s == null) return '—';
  if (s < 60) return `${s} s`;
  const m = Math.floor(s / 60);
  if (m < 60) return `${m} min`;
  const h = Math.floor(m / 60);
  if (h < 24) return `${h} h ${String(m % 60).padStart(2, '0')} min`;
  return `${Math.floor(h / 24)} d ${h % 24} h`;
}

// Why the mount last started (hello's `reset`).
export const RESETS = {
  poweron: 'Power on',
  sw: 'Restart after an update',
  ext: 'Reset button',
  brownout: 'Brown-out (supply too low)',
  panic: 'Crash',
  wdt: 'Watchdog (firmware hung)',
  other: 'Other',
};

export const SOURCES = { gps: 'GPS', phone: 'Phone', saved: 'Saved', none: 'Not set' };
