<!-- The joystick with its speed choice, sending jog to the mount. -->
<script>
  import { onDestroy } from 'svelte';
  import { Segmented, SegmentedButton } from 'konsta/svelte';
  import Joystick from './Joystick.svelte';
  import { Jogger, SPEEDS } from '../lib/jog.js';
  import { jogSpeed } from '../lib/jogspeed.svelte.js';

  let { mount, size = 240 } = $props();

  const jogger = new Jogger(mount);
  $effect(() => {
    jogger.factor = jogSpeed.factor;
  });
  onDestroy(() => jogger.release());

  // Jogs are refused while parked and pointless during the E-stop.
  const mode = $derived(mount.connected ? mount.status?.mode : null);
  const canJog = $derived(mount.connected && mode !== 'estop' && mode !== 'parked');
  $effect(() => {
    if (!canJog) jogger.release();
  });

  // For the Stop button next to it.
  export function release() {
    jogger.release();
  }
</script>

<div class="space-y-4">
  <Joystick {size} disabled={!canJog} onChange={({ x, y }) => jogger.set(x, y)} />
  <Segmented strong>
    {#each SPEEDS as s (s.id)}
      <SegmentedButton strong active={jogSpeed.id === s.id} onClick={() => jogSpeed.set(s.id)}>{s.label}</SegmentedButton>
    {/each}
  </Segmented>
</div>
