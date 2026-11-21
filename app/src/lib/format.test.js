import { describe, expect, it } from 'vitest';
import { formatAz, formatDec, formatDeg, formatLat, formatLon, formatRa } from './format.js';

describe('format', () => {
  it('right ascension', () => {
    expect(formatRa(5.5881)).toBe('05h 35m 17s');
    expect(formatRa(23.99999)).toBe('00h 00m 00s');
    expect(formatRa(-1)).toBe('23h 00m 00s');
    expect(formatRa(null)).toBe('—');
  });
  it('declination', () => {
    expect(formatDec(-5.391)).toBe('−05° 23′ 28″');
    expect(formatDec(89.99999)).toBe('+90° 00′ 00″');
    expect(formatDec(0)).toBe('+00° 00′ 00″');
  });
  it('degrees', () => {
    expect(formatDeg(23.41)).toBe('23.4°');
    expect(formatDeg(-1.25)).toBe('−1.3°');
    expect(formatDeg(-0.01)).toBe('0.0°');
  });
  it('azimuth', () => {
    expect(formatAz(162.03)).toBe('162.0° SSE');
    expect(formatAz(359.9)).toBe('359.9° N');
    expect(formatAz(-90)).toBe('270.0° W');
  });
  it('site', () => {
    expect(formatLat(-33.8688)).toBe('33.869° S');
    expect(formatLon(5)).toBe('5.000° E');
  });
});
