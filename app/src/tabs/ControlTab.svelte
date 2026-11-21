<!-- Manual control: the joystick, Stop, tracking, park, and the way back after the
     E-stop. -->
<script>
  import { Block, BlockTitle, BlockFooter, Button, List, ListItem, Toggle } from 'konsta/svelte';
  import JogPad from '../components/JogPad.svelte';
  import Recovery from '../components/Recovery.svelte';
  import AlignPopup from '../components/AlignPopup.svelte';
  import { apparent } from '../lib/sky.js';
  import { formatAz, formatDeg } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, targets } = $props();

  const st = $derived(mount.status);
  const mode = $derived(mount.connected ? st?.mode : null);

  let pad;

  const recovering = $derived(mount.connected && st && (mode === 'estop' || !st.pos_ok));

  async function run(cmd, args, what) {
    try {
      await mount.send(cmd, args);
      return true;
    } catch (e) {
      ui.showAlert(what, explain(e));
      return false;
    }
  }

  async function stop() {
    pad?.release();
    await run('stop', {}, 'Stop');
  }

  // The switch shows the mount's tracking; it changes when the mount says so.
  let trackSwitch = $state(false);
  $effect(() => {
    trackSwitch = mode === 'tracking';
  });
  async function toggleTracking() {
    const on = mode !== 'tracking';
    trackSwitch = mode === 'tracking';
    await run('track', { on }, on ? 'Start tracking' : 'Stop tracking');
  }

  let alignOpen = $state(false);

  // Sync: the goto target is now centred (after a goto that landed a little
  // off): corrects the pointing near it.
  const canSync = $derived(mount.connected && !!st?.aligned && !!targets.current && mode !== 'estop' && !!mount.site);
  async function sync() {
    const obj = targets.current.obj;
    const p = apparent(obj, mount.now(), mount.site);
    if (await run('sync', { ra: p.ra, dec: p.dec }, `Sync on ${obj.title}`)) ui.showToast(`Synced on ${obj.title}`);
  }

  async function park() {
    if (await run('park', {}, 'Park')) ui.showToast('Going to the park position');
  }
</script>

{#if recovering}
  <Recovery {mount} />
{/if}

<BlockTitle>Move</BlockTitle>
<Block strong inset class="space-y-4">
  {#if st?.hand}
    <p class="text-center text-sm opacity-70">The hand unit's joystick is in use: it has priority.</p>
  {:else if mode === 'parked'}
    <p class="text-center text-sm opacity-70">Parked: unpark to move.</p>
  {:else if mode === 'tracking' || (mode === 'manual' && targets.current)}
    <p class="text-center text-sm opacity-70">Moving shifts the tracked target.</p>
  {/if}
  <JogPad {mount} bind:this={pad} />
  <div class="flex justify-between text-sm opacity-70">
    <span>Alt {formatDeg(st?.pos?.alt)}</span>
    <span>Az {formatAz(st?.pos?.az)}</span>
  </div>
</Block>

<Block class="my-4!">
  <Button large rounded class="bg-red-600! text-white! active:bg-red-700!" disabled={!mount.connected} onClick={stop}>
    Stop
  </Button>
</Block>

<Block class="my-4! grid grid-cols-4 gap-2">
  <Button rounded tonal disabled={!mount.connected || mode !== 'estop' || st?.estop} onClick={() => run('arm', {}, 'Arm')}>
    Arm
  </Button>
  <Button rounded tonal disabled={!mount.connected || !mount.site || mode === 'estop'} onClick={() => (alignOpen = true)}>
    Align
  </Button>
  <Button rounded tonal disabled={!canSync} onClick={sync}>Sync</Button>
  {#if mode === 'parked'}
    <Button rounded tonal disabled={!mount.connected} onClick={() => run('unpark', {}, 'Unpark')}>Unpark</Button>
  {:else}
    <Button rounded tonal disabled={!mount.connected || mode === 'estop'} onClick={park}>Park</Button>
  {/if}
</Block>

<List strong inset>
  <ListItem title="Tracking" footer={targets.current ? `Target: ${targets.current.obj.title}` : 'Follows the sky'}>
    {#snippet after()}
      <Toggle
        bind:checked={trackSwitch}
        disabled={!mount.connected || !st?.aligned || mode === 'estop' || mode === 'parked'}
        onChange={toggleTracking}
      />
    {/snippet}
  </ListItem>
  <ListItem link title="Alignment" after={st?.aligned ? 'Aligned' : 'Not aligned'} onClick={() => (alignOpen = true)} />
</List>
<BlockFooter>Sync: centre the goto target, then sync.</BlockFooter>

<AlignPopup {mount} {targets} bind:opened={alignOpen} />
