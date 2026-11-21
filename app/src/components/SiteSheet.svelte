<!-- Enter the site by hand and send it to the mount (saved in its flash). The
     GPS, when it has a fix, takes precedence on the mount. -->
<script>
  import { Sheet, Block, BlockTitle, BlockFooter, Button, List, ListInput, Link, Segmented, SegmentedButton } from 'konsta/svelte';
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
  });

  const num = (t) => Number(String(t).trim().replace(',', '.'));
  const latOk = $derived(lat !== '' && num(lat) >= 0 && num(lat) <= 90);
  const lonOk = $derived(lon !== '' && num(lon) >= 0 && num(lon) <= 180);
  const elevOk = $derived(elev === '' || (Number.isFinite(num(elev)) && num(elev) > -500 && num(elev) < 9000));

  async function send() {
    const site = { lat: num(lat) * (south ? -1 : 1), lon: num(lon) * (west ? -1 : 1), elev: elev === '' ? 0 : num(elev) };
    busy = true;
    try {
      const r = await mount.send('set_site', { ...site, save: true });
      try {
        localStorage.setItem(KEY, JSON.stringify(site));
      } catch {
        // not kept
      }
      ui.showToast(r.source === 'gps' ? 'The mount uses its GPS site' : 'Site sent to the mount');
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
  <BlockTitle>Latitude</BlockTitle>
  <List strong inset class="my-2!">
    <ListInput type="text" inputmode="decimal" placeholder="45.0" value={lat} onInput={(e) => (lat = e.target.value)} error={lat !== '' && !latOk ? '0 to 90°' : ''} />
  </List>
  <Block class="my-2!">
    <Segmented strong>
      <SegmentedButton strong active={!south} onClick={() => (south = false)}>North</SegmentedButton>
      <SegmentedButton strong active={south} onClick={() => (south = true)}>South</SegmentedButton>
    </Segmented>
  </Block>
  <BlockTitle>Longitude</BlockTitle>
  <List strong inset class="my-2!">
    <ListInput type="text" inputmode="decimal" placeholder="5.0" value={lon} onInput={(e) => (lon = e.target.value)} error={lon !== '' && !lonOk ? '0 to 180°' : ''} />
  </List>
  <Block class="my-2!">
    <Segmented strong>
      <SegmentedButton strong active={!west} onClick={() => (west = false)}>East</SegmentedButton>
      <SegmentedButton strong active={west} onClick={() => (west = true)}>West</SegmentedButton>
    </Segmented>
  </Block>
  <BlockTitle>Elevation (m)</BlockTitle>
  <List strong inset class="my-2!">
    <ListInput type="text" inputmode="numeric" placeholder="0" value={elev} onInput={(e) => (elev = e.target.value)} error={!elevOk ? 'Metres above sea level' : ''} />
  </List>
  <BlockFooter>Degrees with decimals. The town is close enough: the star alignment absorbs a few kilometres.</BlockFooter>
  <Block class="my-4!">
    <Button large rounded disabled={!latOk || !lonOk || !elevOk || busy || !mount.connected} onClick={send}>Send to the mount</Button>
  </Block>
</Sheet>
