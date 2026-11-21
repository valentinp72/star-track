<!-- The site: a saved one in one tap, or entered by hand (and saved under a
     name if given), sent to the mount (kept in its flash). The GPS, when it has
     a fix, takes precedence on the mount. -->
<script>
  import { Sheet, Block, BlockTitle, BlockFooter, Button, List, ListInput, ListItem, Link, Segmented, SegmentedButton } from 'konsta/svelte';
  import { sites } from '../lib/sites.svelte.js';
  import { formatLat, formatLon } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, opened = $bindable(false) } = $props();

  const KEY = 'star-track.site';
  function lastSite() {
    try {
      return JSON.parse(localStorage.getItem(KEY)) ?? null;
    } catch {
      return null;
    }
  }

  // Numbers as typed (positive), and the hemispheres.
  let lat = $state('');
  let lon = $state('');
  let elev = $state('');
  let south = $state(false);
  let west = $state(false);
  let name = $state('');
  let editing = $state(false);
  let busy = $state(false);

  // Fill in from the mount's site, or the last one sent, each time it opens.
  $effect(() => {
    if (!opened) return;
    const s = mount.site ?? lastSite();
    lat = s ? String(Math.abs(s.lat)) : '';
    lon = s ? String(Math.abs(s.lon)) : '';
    elev = s ? String(Math.round(s.elev ?? 0)) : '';
    south = s ? s.lat < 0 : false;
    west = s ? s.lon < 0 : false;
    name = '';
    editing = false;
  });

  const num = (t) => Number(String(t).trim().replace(',', '.'));
  const latOk = $derived(lat !== '' && num(lat) >= 0 && num(lat) <= 90);
  const lonOk = $derived(lon !== '' && num(lon) >= 0 && num(lon) <= 180);
  const elevOk = $derived(elev === '' || (Number.isFinite(num(elev)) && num(elev) > -500 && num(elev) < 9000));

  function typed() {
    return { lat: num(lat) * (south ? -1 : 1), lon: num(lon) * (west ? -1 : 1), elev: elev === '' ? 0 : num(elev) };
  }

  async function send(site) {
    busy = true;
    try {
      const r = await mount.send('set_site', { ...site, save: true });
      try {
        localStorage.setItem(KEY, JSON.stringify(site));
      } catch {
        // not kept
      }
      if (!site.name && name.trim()) sites.put({ ...site, name });
      ui.showToast(r.source === 'gps' ? 'The mount uses its GPS site' : `Site sent to the mount${site.name ? `: ${site.name}` : ''}`);
      opened = false;
    } catch (e) {
      ui.showAlert('Site', explain(e));
    } finally {
      busy = false;
    }
  }
</script>

<Sheet class="pb-safe w-full" {opened} onBackdropClick={() => (opened = false)}>
  <div class="flex items-center justify-between px-4 pt-4">
    <div class="text-[22px] font-bold">Site</div>
    <Link onClick={() => (opened = false)}>Cancel</Link>
  </div>
  <div class="max-h-[75vh] overflow-auto">
  {#if sites.list.length}
    <BlockTitle class="flex justify-between">
      <span>Saved</span>
      <Link onClick={() => (editing = !editing)}>{editing ? 'Done' : 'Edit'}</Link>
    </BlockTitle>
    <List strong inset>
      {#each sites.list as s (s.name)}
        {#if editing}
          <ListItem title={s.name}>
            {#snippet media()}
              <button
                class="flex h-6 w-6 items-center justify-center rounded-full bg-red-500 text-white"
                onclick={() => sites.remove(s.name)}
                aria-label={`Delete ${s.name}`}>−</button
              >
            {/snippet}
          </ListItem>
        {:else}
          <ListItem
            link
            title={s.name}
            after={`${formatLat(s.lat)}, ${formatLon(s.lon)}`}
            onClick={() => !busy && mount.connected && send(s)}
          />
        {/if}
      {/each}
    </List>
    <BlockTitle>New</BlockTitle>
  {/if}
  <List strong inset class="mt-4!">
    <ListInput label="Name (to save it)" type="text" placeholder="Garden" value={name} onInput={(e) => (name = e.target.value)} />
    <ListInput
      label="Latitude (°)"
      type="text"
      inputmode="decimal"
      placeholder="45.0"
      value={lat}
      onInput={(e) => (lat = e.target.value)}
      error={lat !== '' && !latOk ? '0 to 90' : ''}
    />
    <ListInput
      label="Longitude (°)"
      type="text"
      inputmode="decimal"
      placeholder="5.0"
      value={lon}
      onInput={(e) => (lon = e.target.value)}
      error={lon !== '' && !lonOk ? '0 to 180' : ''}
    />
    <ListInput
      label="Elevation (m)"
      type="text"
      inputmode="numeric"
      placeholder="0"
      value={elev}
      onInput={(e) => (elev = e.target.value)}
      error={!elevOk ? 'Metres above sea level' : ''}
    />
  </List>
  <Block class="grid grid-cols-2 gap-3">
    <Segmented strong>
      <SegmentedButton strong active={!south} onClick={() => (south = false)}>N</SegmentedButton>
      <SegmentedButton strong active={south} onClick={() => (south = true)}>S</SegmentedButton>
    </Segmented>
    <Segmented strong>
      <SegmentedButton strong active={!west} onClick={() => (west = false)}>E</SegmentedButton>
      <SegmentedButton strong active={west} onClick={() => (west = true)}>W</SegmentedButton>
    </Segmented>
  </Block>
  <BlockFooter>Degrees with decimals. The town is close enough: the star alignment absorbs a few kilometres.</BlockFooter>
  <Block class="my-4!">
    <Button large rounded disabled={!latOk || !lonOk || !elevOk || busy || !mount.connected} onClick={() => send(typed())}>
      {name.trim() ? 'Save and send to the mount' : 'Send to the mount'}
    </Button>
  </Block>
  </div>
</Sheet>
