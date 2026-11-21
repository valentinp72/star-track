<!-- Alignment: pick a star, bring it into the eyepiece (by goto once the mount
     has a first star, by eye before), centre it with the joystick, add it. -->
<script>
  import { untrack } from 'svelte';
  import {
    Popup, Page, Navbar, Link, Block, BlockTitle, BlockFooter, Button, List, ListItem, Dialog, DialogButton,
  } from 'konsta/svelte';
  import JogPad from './JogPad.svelte';
  import { candidates, whereToLook } from '../lib/align.js';
  import { apparent, horizon } from '../lib/sky.js';
  import { formatAngle, formatDeg } from '../lib/format.js';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, targets, opened = $bindable(false) } = $props();

  const st = $derived(mount.status);
  const aligned = $derived(!!st?.aligned);
  let used = $state([]); // align_list: [{name, ra, dec, err}]
  let index = $state(0); // align_list's alt_index: the altitude counter's reading with the tube square to the base
  let picked = $state(null); // a candidate
  let busy = $state(false);
  let confirmClear = $state(false);
  let now = $state(Date.now());

  async function refresh() {
    now = mount.now();
    try {
      const r = await mount.send('align_list');
      used = r.stars ?? [];
      index = r.alt_index ?? 0;
    } catch {
      used = [];
      index = 0;
    }
  }

  // On opening, and every 30 s while open (the stars move).
  $effect(() => {
    if (!opened) return;
    untrack(refresh);
    const t = setInterval(() => (now = mount.now()), 30000);
    return () => clearInterval(t);
  });

  const list = $derived(opened && mount.site ? candidates(now, mount.site, used) : []);
  const rms = $derived.by(() => {
    if (used.length < 3) return null;
    return Math.sqrt(used.reduce((s, u) => s + u.err * u.err, 0) / used.length);
  });

  // Where the picked star is now (it moves while you centre it).
  let tick = $state(0);
  $effect(() => {
    if (!picked) return;
    const t = setInterval(() => tick++, 2000);
    return () => clearInterval(t);
  });
  const pickedNow = $derived.by(() => {
    tick;
    if (!picked || !mount.site) return null;
    const utc = mount.now();
    const p = apparent(picked.o, utc, mount.site);
    return { ...p, ...horizon(p.ra, p.dec, utc, mount.site) };
  });

  async function goto() {
    busy = true;
    try {
      await targets.goto(picked.o, { track: true });
    } catch (e) {
      ui.showAlert(`Go to ${picked.o.title}`, explain(e));
    } finally {
      busy = false;
    }
  }

  async function add() {
    busy = true;
    const p = apparent(picked.o, mount.now(), mount.site);
    try {
      const r = await mount.send('align_add', { ra: p.ra, dec: p.dec, name: picked.o.title });
      ui.showToast(`Star ${r.n} added: ${picked.o.title}${r.n >= 3 ? ` · fit ${formatAngle(r.rms)}` : ''}`);
      picked = null;
      await refresh();
    } catch (e) {
      ui.showAlert(picked.o.title, explain(e));
    } finally {
      busy = false;
    }
  }

  async function clearAll() {
    confirmClear = false;
    try {
      await mount.send('align_clear');
      await refresh();
    } catch (e) {
      ui.showAlert('Clear the alignment', explain(e));
    }
  }

  function pick(c) {
    picked = c;
    document.querySelector('.align-page')?.scrollTo({ top: 0, behavior: 'smooth' });
  }
</script>

<Popup {opened} onBackdropClick={() => (opened = false)}>
  <Page class="align-page">
    <Navbar title="Alignment">
      {#snippet right()}
        <Link onClick={() => (opened = false)}>Done</Link>
      {/snippet}
    </Navbar>

    <Block strong inset class="text-center">
      {#if used.length === 0}
        <p class="font-semibold">Not aligned</p>
        <p class="mt-1 text-sm opacity-70">Two stars far apart for an exact pointing, a third to check it.</p>
      {:else}
        <p class="font-semibold">{used.length} star{used.length > 1 ? 's' : ''}{rms != null ? ` · fit ${formatAngle(rms)}` : ''}</p>
        <p class="mt-1 text-sm opacity-70">
          {used.length === 1 ? 'Add a second star, far from the first and at another height.' : used.length === 2 ? 'A third star checks the fit.' : 'Aligned.'}
        </p>
        {#if Math.abs(index) >= 0.1}
          <!-- index < 0: the counter read less than the true altitude, the tube was up at power-on -->
          <p class="mt-1 text-sm opacity-70">
            The tube was {formatDeg(Math.abs(index))} {index < 0 ? 'above' : 'below'} level at power-on.
          </p>
        {/if}
      {/if}
    </Block>

    {#if picked}
      <BlockTitle>{picked.o.title}</BlockTitle>
      <Block strong inset class="space-y-4">
        {#if pickedNow}
          <p class="text-center">{whereToLook(pickedNow.alt, pickedNow.az)}</p>
        {/if}
        {#if aligned}
          <Button rounded tonal disabled={busy || !mount.connected} onClick={goto}>Go to {picked.o.title}</Button>
        {:else}
          <p class="text-center text-sm opacity-70">Point the tube at it with the joystick.</p>
        {/if}
        <JogPad {mount} size={200} />
        <Button large rounded disabled={busy || !mount.connected} onClick={add}>Centred: add it</Button>
        <Button clear small onClick={() => (picked = null)}>Another star</Button>
      </Block>
    {/if}

    <BlockTitle>{picked ? 'Other stars' : 'Pick a star'}</BlockTitle>
    {#if !mount.site}
      <BlockFooter>The mount does not know its site: set it in Setup.</BlockFooter>
    {:else if list.length === 0}
      <BlockFooter>No bright star between 15° and 70° altitude now.</BlockFooter>
    {:else}
      <List strong inset>
        {#each list.slice(0, 12) as c, i (c.o.key)}
          <ListItem
            link
            title={c.o.title}
            subtitle={whereToLook(c.alt, c.az)}
            after={i < 3 ? 'Suggested' : ''}
            class={picked?.o.key === c.o.key ? 'font-semibold' : ''}
            onClick={() => pick(c)}
          />
        {/each}
      </List>
    {/if}

    {#if used.length}
      <BlockTitle>Stars used</BlockTitle>
      <List strong inset>
        {#each used as u (u.name + u.ra)}
          <ListItem title={u.name} after={used.length >= 3 ? formatAngle(u.err) : ''} />
        {/each}
      </List>
      <BlockFooter>
        {used.length >= 3 ? 'The error of each star against the fit. ' : ''}A star centred again replaces the one near it.
      </BlockFooter>
      <Block>
        <Button clear class="text-red-500!" onClick={() => (confirmClear = true)}>Clear the alignment</Button>
      </Block>
    {/if}
    <div class="h-[calc(var(--k-safe-area-bottom)+24px)]"></div>
  </Page>
</Popup>

<Dialog opened={confirmClear} onBackdropClick={() => (confirmClear = false)}>
  {#snippet title()}Clear the alignment?{/snippet}
  All the alignment stars are removed: gotos need a new alignment.
  {#snippet buttons()}
    <DialogButton onClick={() => (confirmClear = false)}>Cancel</DialogButton>
    <DialogButton strong class="text-red-500!" onClick={clearAll}>Clear</DialogButton>
  {/snippet}
</Dialog>
