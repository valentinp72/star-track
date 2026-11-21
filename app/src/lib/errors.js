// A sentence for the user from a refused or failed command (MountError).

const FLAGS = {
  time_ok: 'The mount does not know the time yet.',
  site_ok: 'The mount does not know where it is: set the site in Setup.',
  aligned: 'The mount is not aligned yet: align it on a star first.',
  pos_ok: 'The axis positions are uncertain (after the E-stop): centre a known star and sync on it.',
};

export function explain(e) {
  const msg = e?.message ?? String(e);
  switch (e?.err) {
    case 'estop':
      return /still pressed|is pressed/.test(msg) ? 'The E-stop is pressed: release it first.' : 'The mount must be re-armed after the E-stop.';
    case 'parked':
      return 'The mount is parked: unpark it first.';
    case 'not_ready': {
      const flag = Object.keys(FLAGS).find((f) => msg.startsWith(f));
      return flag ? FLAGS[flag] : `Not ready: ${msg}.`;
    }
    case 'below_limit':
      return 'The object is outside the altitude limits (below the horizon or the set minimum).';
    case 'sun':
      return `Too close to the Sun: ${msg}.`;
    case 'align_mismatch':
      return `This star does not fit the earlier ones: ${msg}.`;
    case 'timeout':
      return 'The mount did not answer.';
    case 'disconnected':
      return 'Not connected to the mount.';
    default:
      return msg.charAt(0).toUpperCase() + msg.slice(1) + (/[.!?]$/.test(msg) ? '' : '.');
  }
}
