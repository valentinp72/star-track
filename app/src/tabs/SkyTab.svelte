<!-- The sky chart, where the tube points and its target. Tapping an object opens
     its sheet, with the goto. -->
<script>
  import { Block, Button, List, ListItem } from 'konsta/svelte';
  import SkyChart from '../components/SkyChart.svelte';
  import ObjectSheet from '../components/ObjectSheet.svelte';
  import { formatAz, formatDeg } from '../lib/format.js';

  let { mount, targets, askSolar } = $props();

  let chart;
  let follow = $state(false);
  let open = $state(null);
  let alignStars = $state([]);

  // The alignment stars, for their rings: read when the mount becomes aligned,
  // then every 30 s (a star added). `aligned` is a plain value: the effect does
  // not run on every status.
  const aligned = $derived(mount.connected && !!mount.status?.aligned);
  $effect(() => {
    if (!aligned) {
      alignStars = [];
      return;
    }
    const read = () =>
      mount
        .send('align_list')
        .then((r) => (alignStars = r.stars ?? []))
        .catch(() => {});
    read();
    const t = setInterval(read, 30000);
    return () => clearInterval(t);
  });

  const pos = $derived(mount.connected ? mount.status?.pos : null);
  const target = $derived(mount.connected ? mount.status?.target : null);
</script>

<div class="px-4 pt-2">
  <SkyChart bind:this={chart} {mount} {targets} {alignStars} bind:follow onPick={(o) => (open = o)} />
</div>

<Block class="my-3! grid grid-cols-2 gap-3">
  <Button rounded tonal onClick={() => chart.resetView()}>Whole sky</Button>
  <Button rounded tonal={!follow} onClick={() => (follow = !follow)} disabled={!pos || pos.az == null}>
    Telescope
  </Button>
</Block>

<List strong inset>
  <ListItem title="Telescope" after={pos ? `${formatDeg(pos.alt)} · ${formatAz(pos.az)}` : '—'} />
  <ListItem
    title={targets.current ? targets.current.obj.title : 'Target'}
    after={target ? `${formatDeg(target.alt)} · ${formatAz(target.az)}` : 'None'}
  />
</List>

<ObjectSheet {mount} {targets} bind:obj={open} {askSolar} />
