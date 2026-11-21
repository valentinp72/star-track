<!-- The mount's mode in the navigation bar, or the link state when not connected.
     The navigation bar draws the glass capsule around it: a dot and a word. -->
<script>
  import { MODES } from '../lib/format.js';

  let { mount } = $props();

  const DOTS = {
    estop: 'bg-red-500',
    goto: 'bg-orange-500',
    manual: 'bg-primary',
    tracking: 'bg-green-500',
    idle: 'bg-gray-400',
    parked: 'bg-gray-400',
    offline: 'bg-gray-400 opacity-50',
  };

  const mode = $derived(mount.connected && mount.status ? mount.status.mode : 'offline');
  const label = $derived(mode === 'offline' ? (mount.link === 'connecting' ? 'Connecting' : 'Offline') : MODES[mode] ?? mode);
</script>

<span class="flex items-center gap-1.5 text-[15px] font-semibold {mode === 'estop' ? 'text-red-500' : ''} {mode === 'offline' ? 'opacity-60' : ''}">
  <span class="h-2 w-2 shrink-0 rounded-full {DOTS[mode] ?? DOTS.idle}"></span>
  {label}
</span>
