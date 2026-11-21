<script>
  import { onMount } from 'svelte';
  import { App, Page, Navbar, Tabbar, TabbarLink } from 'konsta/svelte';
  import Icon from './components/Icon.svelte';
  import ModeBadge from './components/ModeBadge.svelte';
  import Banners from './components/Banners.svelte';
  import SkyTab from './tabs/SkyTab.svelte';
  import ObjectsTab from './tabs/ObjectsTab.svelte';
  import ControlTab from './tabs/ControlTab.svelte';
  import SetupTab from './tabs/SetupTab.svelte';
  import { Mount } from './lib/mount.svelte.js';
  import { appearance } from './lib/appearance.svelte.js';

  const mount = new Mount();

  const TABS = [
    { id: 'sky', label: 'Sky', view: SkyTab },
    { id: 'objects', label: 'Objects', view: ObjectsTab },
    { id: 'control', label: 'Control', view: ControlTab },
    { id: 'setup', label: 'Setup', view: SetupTab },
  ];
  let tab = $state('sky');
  const current = $derived(TABS.find((t) => t.id === tab));

  onMount(() => {
    mount.start();
    const resume = () => document.visibilityState === 'visible' && mount.resume();
    document.addEventListener('visibilitychange', resume);
    window.addEventListener('pageshow', resume);
    return () => {
      document.removeEventListener('visibilitychange', resume);
      window.removeEventListener('pageshow', resume);
      mount.stop();
    };
  });

  // Konsta's dark mode follows the `dark` class on <html>.
  $effect(() => {
    document.documentElement.classList.toggle('dark', appearance.dark);
    document.documentElement.classList.toggle('night', appearance.night);
    document.querySelector('meta[name="theme-color"]')?.setAttribute('content', appearance.dark ? '#000000' : '#f7f7f8');
  });
</script>

<App theme="ios" safeAreas class={appearance.night ? 'k-color-brand-red' : ''}>
  <Page>
    <Navbar title={current.label} rightClass="h-10 px-3.5">
      {#snippet right()}
        <ModeBadge {mount} />
      {/snippet}
    </Navbar>
    <Banners {mount} />
    <div class="pb-[calc(var(--k-safe-area-bottom)+104px)]">
      <current.view {mount} />
    </div>
    <Tabbar
      labels
      icons
      class="left-0 bottom-0 fixed"
      innerClass="rounded-full px-1 backdrop-blur-lg bg-ios-light-glass dark:bg-ios-dark-glass shadow-[0_4px_20px_rgba(0,0,0,0.15)]"
    >
      {#each TABS as t (t.id)}
        <TabbarLink active={tab === t.id} onClick={() => (tab = t.id)} label={t.label}>
          {#snippet icon()}
            <Icon name={t.id} />
          {/snippet}
        </TabbarLink>
      {/each}
    </Tabbar>
  </Page>
  {#if appearance.night}
    <div class="night-filter"></div>
  {/if}
</App>
