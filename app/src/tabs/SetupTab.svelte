<!-- The connection, the mount's state, the app's appearance and the mount's messages. -->
<script>
  import { BlockTitle, BlockFooter, List, ListItem, Segmented, SegmentedButton, Block } from 'konsta/svelte';
  import { appearance } from '../lib/appearance.svelte.js';
  import { formatClock, formatLat, formatLon, MODES, SOURCES } from '../lib/format.js';

  let { mount } = $props();

  const st = $derived(mount.status);
  const yesNo = (v) => (v == null ? '—' : v ? 'Yes' : 'No');
  const gps = $derived(!st ? '—' : !st.gps ? 'No signal' : st.gps.fix ? `Fix, ${st.gps.sats} satellites` : `No fix, ${st.gps.sats} satellites`);
  const LINK = { open: 'Connected', connecting: 'Connecting…', closed: 'Not connected' };
</script>

<BlockTitle>Appearance</BlockTitle>
<Block strong inset>
  <Segmented strong>
    <SegmentedButton strong active={appearance.choice === 'auto'} onClick={() => appearance.set('auto')}>Auto</SegmentedButton>
    <SegmentedButton strong active={appearance.choice === 'dark'} onClick={() => appearance.set('dark')}>Dark</SegmentedButton>
    <SegmentedButton strong active={appearance.choice === 'night'} onClick={() => appearance.set('night')}>Night</SegmentedButton>
  </Segmented>
</Block>
<BlockFooter>Night: red on black, to keep your eyes adapted to the dark.</BlockFooter>

<BlockTitle>Mount</BlockTitle>
<List strong inset>
  <ListItem title="Mode" after={st ? (MODES[st.mode] ?? st.mode) : '—'} />
  <ListItem title="Clock" after={st?.time_ok ? `${formatClock(st.utc)} (${SOURCES[st.time_src]})` : 'Not set'} />
  <ListItem title="Site" after={st?.site ? `${formatLat(st.site.lat)}, ${formatLon(st.site.lon)}` : 'Not set'} footer={st?.site ? SOURCES[st.site_src] : ''} />
  <ListItem title="GPS" after={gps} />
  <ListItem title="Aligned" after={yesNo(st?.aligned)} />
  <ListItem title="Position known" after={yesNo(st?.pos_ok)} />
  <ListItem title="Solar mode" after={st ? (st.solar ? 'On' : 'Off') : '—'} />
</List>

<BlockTitle>Connection</BlockTitle>
<List strong inset>
  <ListItem title="State" after={LINK[mount.link]} />
  <ListItem title="Firmware" after={mount.info?.fw ?? '—'} />
  <ListItem title="Board" after={mount.info ? (mount.info.sim ? 'Simulator' : mount.info.board) : '—'} />
  <ListItem title="Protocol" after={mount.info?.proto ?? '—'} />
</List>

{#if mount.logs.length}
  <BlockTitle>Messages</BlockTitle>
  <List strong inset>
    {#each mount.logs.slice(0, 10) as l (l.at + l.msg)}
      <ListItem title={l.msg} footer={`${l.level} · ${formatClock(l.at)}`} />
    {/each}
  </List>
{/if}
