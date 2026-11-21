<!-- The catalogue: search, what is up now, and the object sheet with the goto. -->
<script>
  import { untrack } from 'svelte';
  import { Searchbar, Segmented, SegmentedButton, BlockTitle, Block, List, ListItem, Button } from 'konsta/svelte';
  import ObjectSheet from '../components/ObjectSheet.svelte';
  import { bodies, dsos, listedStars, stars, searchKey } from '../lib/catalog.js';
  import { apparent, horizon, quickHorizon } from '../lib/sky.js';
  import { formatDeg } from '../lib/format.js';

  let { mount, targets, askSolar } = $props();

  let query = $state('');
  let upOnly = $state(true);
  let open = $state(null); // the object in the sheet
  const SHOW = 40; // rows per group before "Show all"
  let expanded = $state({});

  const site = $derived(mount.site);

  // Altitudes: recomputed every 20 s, not on every status.
  let now = $state(Date.now());
  $effect(() => {
    now = untrack(() => mount.now());
    const t = setInterval(() => (now = mount.now()), 20000);
    return () => clearInterval(t);
  });

  const altitudes = $derived.by(() => {
    const alt = new Map();
    if (!site) return alt;
    const q = quickHorizon(now, site);
    for (const o of [...dsos, ...stars]) alt.set(o.key, q(o.ra, o.dec).alt);
    for (const b of bodies) {
      const p = apparent(b, now, site);
      alt.set(b.key, horizon(p.ra, p.dec, now, site).alt);
    }
    return alt;
  });

  const altMin = $derived(Math.max(0, mount.config?.['alt.min'] ?? 0));

  function pick(list) {
    const k = searchKey(query);
    let out = k ? list.filter((o) => o.search.includes(k)) : list;
    // A search looks through everything: a name typed is the object wanted.
    if (upOnly && site && !k) out = out.filter((o) => (altitudes.get(o.key) ?? -90) > altMin);
    return out;
  }

  const groups = $derived([
    { id: 'bodies', title: 'Solar system', items: pick(bodies) },
    { id: 'dso', title: 'Deep sky', items: pick(dsos) },
    // Searching looks through every star; the list shows the named and bright ones.
    { id: 'stars', title: 'Stars', items: pick(searchKey(query) ? stars : listedStars) },
  ]);
  const total = $derived(groups.reduce((n, g) => n + g.items.length, 0));

  function altText(o) {
    const a = altitudes.get(o.key);
    return a == null ? '' : formatDeg(a, 0);
  }
  const below = (o) => (altitudes.get(o.key) ?? 90) <= altMin;
  const targetKey = $derived(targets.current?.obj?.key);
</script>

<div class="px-4 pt-2">
  <Searchbar
    value={query}
    onInput={(e) => (query = e.target.value)}
    onClear={() => (query = '')}
    placeholder="M 42, Vega, Saturn, Orion…"
    disableButton={false}
  />
</div>

{#if !query}
  <Block class="my-3!">
    <Segmented strong>
      <SegmentedButton strong active={upOnly} onClick={() => (upOnly = true)}>Up now</SegmentedButton>
      <SegmentedButton strong active={!upOnly} onClick={() => (upOnly = false)}>All</SegmentedButton>
    </Segmented>
  </Block>
{/if}

{#if !site}
  <Block class="text-center text-sm opacity-70">
    The mount does not know its site yet: altitudes and "Up now" need it. Set it in Setup.
  </Block>
{/if}

{#each groups as g (g.id)}
  {#if g.items.length}
    <BlockTitle>{g.title} <span class="ms-1 font-normal opacity-60">{g.items.length}</span></BlockTitle>
    <List strong inset>
      {#each expanded[g.id] ? g.items : g.items.slice(0, SHOW) as o (o.key)}
        <ListItem
          link
          title={o.title}
          subtitle={o.subtitle}
          after={altText(o)}
          onClick={() => (open = o)}
          class="{o.key === targetKey ? 'font-semibold' : ''} {below(o) ? 'opacity-50' : ''}"
        />
      {/each}
    </List>
    {#if g.items.length > SHOW && !expanded[g.id]}
      <Block class="-mt-4!">
        <Button clear onClick={() => (expanded = { ...expanded, [g.id]: true })}>Show all {g.items.length}</Button>
      </Block>
    {/if}
  {/if}
{/each}

{#if total === 0}
  <Block class="text-center opacity-60">
    {query ? 'Nothing found.' : 'Nothing above the horizon.'}
    {#if upOnly && !query}<br /><button class="text-primary" onclick={() => (upOnly = false)}>Show all objects</button>{/if}
  </Block>
{/if}

<ObjectSheet {mount} {targets} bind:obj={open} {askSolar} />
