import { describe, expect, it } from 'vitest';
import { explain } from './errors.js';
import { MountError } from './mount.svelte.js';

describe('explain', () => {
  it('turns refusals into sentences', () => {
    expect(explain(new MountError('not_ready', 'aligned is false: send align_add'))).toMatch(/not aligned/);
    expect(explain(new MountError('estop', 'the E-stop is pressed'))).toMatch(/release it/);
    expect(explain(new MountError('estop', 'the E-stop was pressed: send arm'))).toMatch(/re-armed/);
    expect(explain(new MountError('bad_arg', 'points: not in time order'))).toBe('Points: not in time order.');
  });
});
