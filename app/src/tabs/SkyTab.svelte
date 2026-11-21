<!-- The sky chart (to come). For now, where the tube points and its target. -->
<script>
  import { BlockTitle, List, ListItem } from 'konsta/svelte';
  import { formatAz, formatDec, formatDeg, formatRa } from '../lib/format.js';

  let { mount, targets } = $props();

  const pos = $derived(mount.status?.pos);
  const target = $derived(mount.status?.target);
</script>

<BlockTitle>Pointing</BlockTitle>
<List strong inset>
  <ListItem title="Right ascension" after={formatRa(pos?.ra)} />
  <ListItem title="Declination" after={formatDec(pos?.dec)} />
  <ListItem title="Altitude" after={formatDeg(pos?.alt)} />
  <ListItem title="Azimuth" after={formatAz(pos?.az)} />
</List>

{#if target}
  <BlockTitle>Target{targets.current ? ` · ${targets.current.obj.title}` : ''}</BlockTitle>
  <List strong inset>
    <ListItem title="Right ascension" after={formatRa(target.ra)} />
    <ListItem title="Declination" after={formatDec(target.dec)} />
    <ListItem title="Altitude" after={formatDeg(target.alt)} />
    <ListItem title="Azimuth" after={formatAz(target.az)} />
  </List>
{/if}
