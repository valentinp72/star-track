<!-- The way back after the E-stop: release it, re-arm (the Arm button under the
     joystick), find the position again (the motors were free, so the step
     counters may be wrong). -->
<script>
  import { Block, BlockTitle, Button } from 'konsta/svelte';
  import SyncSheet from './SyncSheet.svelte';

  let { mount } = $props();

  const st = $derived(mount.status);
  const released = $derived(!st?.estop);
  const armed = $derived(st?.mode !== 'estop');
  const found = $derived(!!st?.pos_ok);
  let syncOpen = $state(false);

  const steps = $derived([
    { n: 1, done: released, now: !released, text: released ? 'E-stop released' : 'Release the E-stop' },
    { n: 2, done: armed, now: released && !armed, text: armed ? 'Armed' : 'Arm the mount' },
    { n: 3, done: found, now: armed && !found, text: found ? 'Position found' : 'Centre a bright star, then choose it' },
  ]);
</script>

<BlockTitle>After the E-stop</BlockTitle>
<Block strong inset class="space-y-3">
  {#each steps as s (s.n)}
    <div class="flex items-start gap-3 {s.done || s.now ? '' : 'opacity-50'}">
      <span
        class="mt-0.5 flex h-6 w-6 shrink-0 items-center justify-center rounded-full text-sm font-semibold
          {s.done ? 'bg-green-600 text-white' : s.now ? 'bg-primary text-white' : 'bg-black/10 dark:bg-white/15'}"
      >
        {s.done ? '✓' : s.n}
      </span>
      <div class="flex-1">
        <p>{s.text}</p>
        {#if s.n === 3 && s.now}
          <Button rounded inline small class="mt-2" onClick={() => (syncOpen = true)}>Choose the centred star…</Button>
        {/if}
      </div>
    </div>
  {/each}
</Block>

<SyncSheet {mount} bind:opened={syncOpen} />
