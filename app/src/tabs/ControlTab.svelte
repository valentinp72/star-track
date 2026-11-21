<!-- Manual control: the joystick, Stop, tracking, park, and the way back after the
     E-stop. -->
<script>
  import { onDestroy } from 'svelte';
  import { Block, BlockTitle, Button, List, ListItem, Segmented, SegmentedButton, Toggle } from 'konsta/svelte';
  import Joystick from '../components/Joystick.svelte';
  import Recovery from '../components/Recovery.svelte';
  import SyncSheet from '../components/SyncSheet.svelte';
  import { Jogger, SPEEDS } from '../lib/jog.js';
  import { formatAz, formatDeg } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, targets } = $props();

  const st = $derived(mount.status);
  const mode = $derived(mount.connected ? st?.mode : null);

  // The speed choice is kept on the phone.
  const KEY = 'star-track.jog-speed';
  let speed = $state(load());
  function load() {
    try {
      return SPEEDS.find((s) => s.id === localStorage.getItem(KEY))?.id ?? 'medium';
    } catch {
      return 'medium';
    }
  }
  function setSpeed(id) {
    speed = id;
    try {
      localStorage.setItem(KEY, id);
    } catch {
      // not kept
    }
  }

  const jogger = new Jogger(mount);
  $effect(() => {
    jogger.factor = SPEEDS.find((s) => s.id === speed).factor;
  });
  onDestroy(() => jogger.release());

  // The app's jogs are ignored while the hand unit's joystick is off centre,
  // refused while parked, and pointless during the E-stop.
  const canJog = $derived(mount.connected && mode !== 'estop' && mode !== 'parked');
  $effect(() => {
    if (!canJog) jogger.release();
  });

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
    jogger.release();
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
    <p class="text-center text-sm opacity-70">Moving now shifts the tracked target: use it to centre the object.</p>
  {/if}
  <Joystick disabled={!canJog} onChange={({ x, y }) => jogger.set(x, y)} />
  <Segmented strong>
    {#each SPEEDS as s (s.id)}
      <SegmentedButton strong active={speed === s.id} onClick={() => setSpeed(s.id)}>{s.label}</SegmentedButton>
    {/each}
  </Segmented>
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

<Block class="my-4! grid grid-cols-3 gap-3">
  <Button rounded tonal disabled={!mount.connected || mode !== 'estop' || st?.estop} onClick={() => run('arm', {}, 'Arm')}>
    Arm
  </Button>
  <Button rounded tonal disabled={!mount.connected || !mount.site || mode === 'estop'} onClick={() => (alignOpen = true)}>
    Align
  </Button>
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
  <ListItem title="Alignment" after={st?.aligned ? 'Aligned' : 'Not aligned'} />
</List>

<SyncSheet {mount} action="align" bind:opened={alignOpen} />
