<!-- One object: where it is now, when it rises and sets, and the goto buttons. -->
<script>
  import { untrack } from 'svelte';
  import { Sheet, Block, BlockTitle, Button, List, ListItem, Link } from 'konsta/svelte';
  import { apparent, bodyMagnitude, horizon, riseSet, separation, sun as sunAt } from '../lib/sky.js';
  import { formatAz, formatDec, formatDeg, formatRa, formatTime } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  // `obj` is the open object (null: closed). `askSolar(then)` opens the solar
  // filter check.
  let { mount, targets, obj = $bindable(null), askSolar } = $props();

  let busy = $state(false);

  const st = $derived(mount.status);
  const site = $derived(mount.site);
  const altMin = $derived(mount.config?.['alt.min'] ?? 0);
  const sunMinSep = $derived(mount.config?.['sun.min_sep'] ?? 15);
  const isSun = $derived(obj?.body === 'Sun');

  // Recomputed with each status while open (about 10 per second) would be
  // wasteful: once a second is plenty.
  let now = $state(Date.now());
  $effect(() => {
    if (!obj) return;
    now = untrack(() => mount.now());
    const t = setInterval(() => (now = mount.now()), 1000);
    return () => clearInterval(t);
  });

  const pos = $derived.by(() => {
    if (!obj || !site) return null;
    const p = apparent(obj, now, site);
    return { ...p, ...horizon(p.ra, p.dec, now, site) };
  });
  // Rise and set: once per object (the search is slower).
  const times = $derived.by(() => {
    if (!obj || !site) return null;
    return riseSet(obj, untrack(() => mount.now()), site);
  });
  const sunNow = $derived(site ? sunAt(now, site) : null);
  const sunSep = $derived(pos && sunNow && !isSun ? separation(pos.ra, pos.dec, sunNow.ra, sunNow.dec) : null);
  const mag = $derived(obj?.body ? bodyMagnitude(obj.body, now) : obj?.mag);

  // Why a goto cannot be sent now (null: it can). The mount checks again.
  const blocked = $derived.by(() => {
    if (!mount.connected) return 'Not connected to the mount.';
    if (!site) return 'The mount does not know where it is: set the site in Setup.';
    if (pos && pos.alt < altMin) return altMin > 0 ? `Below the ${altMin}° altitude limit.` : 'Below the horizon.';
    if (!isSun && !st?.solar && sunNow && sunNow.alt > -2 && sunSep != null && sunSep < sunMinSep)
      return `${formatDeg(sunSep)} from the Sun: closer than the ${sunMinSep}° safety limit.`;
    return null;
  });

  // `solarOn`: solar mode was just turned on (the status may not show it yet).
  async function go(track, solarOn = false) {
    if (isSun && !st?.solar && !solarOn) {
      askSolar(() => go(track, true));
      return;
    }
    busy = true;
    try {
      const r = await targets.goto(obj, { track });
      const eta = r?.eta != null ? ` · about ${Math.max(1, Math.round(r.eta))} s` : '';
      ui.showToast(`Slewing to ${obj.title}${eta}`);
      obj = null;
    } catch (e) {
      ui.showAlert(`Cannot go to ${obj.title}`, explain(e));
    } finally {
      busy = false;
    }
  }

  const when = (t) => (t == null ? '—' : formatTime(t));
  const sizeText = (s) => (s == null ? null : s >= 60 ? `${(s / 60).toFixed(1)}°` : `${s}′`);
</script>

<Sheet class="pb-safe w-full" opened={!!obj} onBackdropClick={() => (obj = null)}>
  {#if obj}
    <div class="flex items-start justify-between px-4 pt-4">
      <div>
        <div class="text-[22px] font-bold leading-tight">{obj.title}</div>
        {#if obj.subtitle}<div class="mt-0.5 opacity-70">{obj.subtitle}</div>{/if}
      </div>
      <Link onClick={() => (obj = null)} class="pt-1">Done</Link>
    </div>

    <div class="max-h-[55vh] overflow-auto">
      <List strong inset class="my-4!">
        {#if pos}
          <ListItem title="Altitude" after={formatDeg(pos.alt)} />
          <ListItem title="Azimuth" after={formatAz(pos.az)} />
        {/if}
        {#if times}
          {#if times.rise == null && times.set == null}
            <ListItem title={times.transitAlt > 0 ? 'Always up' : 'Never up'} after="" />
          {:else}
            <!-- in the order they come -->
            {#each [['Rises', times.rise], ['Sets', times.set]].sort((a, b) => (a[1] ?? Infinity) - (b[1] ?? Infinity)) as [label, t] (label)}
              <ListItem title={label} after={when(t)} />
            {/each}
          {/if}
          <ListItem title="Highest" after={times.transit ? `${when(times.transit)} · ${formatDeg(times.transitAlt, 0)}` : '—'} />
        {/if}
        {#if sunSep != null && sunNow.alt > -12}
          <ListItem title="From the Sun" after={formatDeg(sunSep)} />
        {/if}
      </List>

      <List strong inset class="my-4!">
        {#if mag != null}<ListItem title="Magnitude" after={mag.toFixed(1)} />{/if}
        {#if obj.size}<ListItem title="Size" after={sizeText(obj.size)} />{/if}
        {#if obj.conName}<ListItem title="Constellation" after={obj.conName} />{/if}
        {#if obj.other}<ListItem title="Also" after={obj.other} />{/if}
        {#if pos}
          <ListItem title="RA (of date)" after={formatRa(pos.ra)} />
          <ListItem title="Dec (of date)" after={formatDec(pos.dec)} />
        {/if}
      </List>
    </div>

    <Block class="my-4! space-y-3">
      {#if blocked}
        <p class="text-center text-sm opacity-70">{blocked}</p>
      {/if}
      {#if isSun && !st?.solar}
        <p class="text-center text-sm opacity-70">The Sun needs solar mode and a solar filter.</p>
      {/if}
      <Button large rounded disabled={!!blocked || busy} onClick={() => go(true)}>
        {isSun && !st?.solar ? 'Solar mode…' : 'Go to and track'}
      </Button>
      {#if !isSun}
        <Button large rounded tonal disabled={!!blocked || busy} onClick={() => go(false)}>Go to without tracking</Button>
      {/if}
    </Block>
  {/if}
</Sheet>
