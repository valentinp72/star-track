<!-- "I have centred this one": bright stars and planets at a useful altitude.
     Sends sync (align_add before any alignment star) with its position of
     date, to find the position again after the E-stop. -->
<script>
  import { untrack } from 'svelte';
  import { Sheet, BlockTitle, BlockFooter, List, ListItem, Link } from 'konsta/svelte';
  import { bodies, stars } from '../lib/catalog.js';
  import { apparent, bodyMagnitude, horizon } from '../lib/sky.js';
  import { formatAz, formatDeg } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, opened = $bindable(false) } = $props();

  // Between 5° and 75°: near the zenith the mount refuses (the azimuth is too
  // uncertain there), near the horizon refraction varies.
  const MIN_ALT = 5;
  const MAX_ALT = 75;
  // Stars to magnitude 2.2 and the planets brighter than that, by brightness;
  // the Moon last (its disc is half a degree wide: a rough reference).
  const brightStars = stars.filter((s) => s.name && s.mag <= 2.2);
  const planets = bodies.filter((b) => b.body !== 'Sun' && b.body !== 'Moon');
  const moon = bodies.find((b) => b.body === 'Moon');

  const list = $derived.by(() => {
    if (!opened || !mount.site) return [];
    const utc = untrack(() => mount.now());
    const withMag = [
      ...brightStars.map((o) => ({ o, mag: o.mag })),
      ...planets.map((o) => ({ o, mag: bodyMagnitude(o.body, utc) })).filter((c) => c.mag <= 2.2),
    ].sort((a, b) => a.mag - b.mag);
    return [...withMag, { o: moon, mag: null }]
      .map(({ o }) => {
        const p = apparent(o, utc, mount.site);
        return { o, p, h: horizon(p.ra, p.dec, utc, mount.site) };
      })
      .filter((c) => c.h.alt > MIN_ALT && c.h.alt < MAX_ALT);
  });

  let busy = $state(false);
  async function pick(c) {
    if (busy) return;
    busy = true;
    // The position now, not when the list was made.
    const p = apparent(c.o, mount.now(), mount.site);
    try {
      if (mount.status?.aligned) {
        await mount.send('sync', { ra: p.ra, dec: p.dec });
        ui.showToast(`Position set on ${c.o.title}`);
      } else {
        const r = await mount.send('align_add', { ra: p.ra, dec: p.dec, name: c.o.title });
        ui.showToast(`Alignment star ${r.n}: ${c.o.title}`);
      }
      opened = false;
    } catch (e) {
      ui.showAlert(c.o.title, explain(e));
    } finally {
      busy = false;
    }
  }
</script>

<Sheet class="pb-safe w-full" {opened} onBackdropClick={() => (opened = false)}>
  <div class="flex items-center justify-between px-4 pt-4">
    <div class="text-[22px] font-bold">Which one is centred?</div>
    <Link onClick={() => (opened = false)}>Cancel</Link>
  </div>
  <BlockFooter class="mt-2!">
    Centre it in the eyepiece with the joystick, then tap it.
  </BlockFooter>
  <div class="max-h-[60vh] overflow-auto">
    {#if !mount.site}
      <BlockFooter>The mount does not know its site: set it in Setup.</BlockFooter>
    {:else if list.length === 0}
      <BlockFooter>No bright star or planet between {MIN_ALT}° and {MAX_ALT}° now.</BlockFooter>
    {:else}
      <BlockTitle>Brightest first</BlockTitle>
      <List strong inset>
        {#each list as c (c.o.key)}
          <ListItem
            link
            title={c.o.title}
            subtitle={c.o.subtitle}
            after={`${formatDeg(c.h.alt, 0)} · ${formatAz(c.h.az).split(' ')[1]}`}
            onClick={() => pick(c)}
          />
        {/each}
      </List>
    {/if}
  </div>
</Sheet>
