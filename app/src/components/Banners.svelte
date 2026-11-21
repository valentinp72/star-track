<!-- Warnings above every tab: no connection, E-stop pressed, solar mode on. -->
<script>
  import { Block, Button, Preloader } from 'konsta/svelte';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount } = $props();

  const solar = $derived(mount.connected && mount.status?.solar);

  async function solarOff() {
    try {
      await mount.send('solar', { on: false });
    } catch (e) {
      ui.showAlert('Solar mode', explain(e));
    }
  }

  const estop = $derived(mount.connected && mount.status?.mode === 'estop');
</script>

{#if !mount.connected}
  <Block strong inset class="flex items-start gap-3">
    {#if mount.link === 'connecting'}
      <Preloader class="mt-0.5 h-5 w-5 shrink-0" />
    {/if}
    <div>
      <p class="font-semibold">{mount.link === 'connecting' ? 'Connecting to the mount…' : 'Not connected'}</p>
      <p class="mt-1 text-sm opacity-70">
        Join the mount's Wi-Fi network (<i>star-track</i>) in the iPhone's settings. The app reconnects by itself.
      </p>
      {#if mount.error}
        <p class="mt-1 text-sm opacity-70">{mount.error}</p>
      {/if}
    </div>
  </Block>
{:else if estop}
  <Block strong inset class="bg-red-500! text-white">
    <p class="font-semibold">E-stop pressed</p>
  </Block>
{/if}
{#if solar}
  <Block strong inset class="flex items-center gap-3 bg-yellow-400! text-black">
    <div class="flex-1">
      <p class="font-semibold">Solar mode on</p>
      <p class="mt-1 text-sm">The keep-out zone around the Sun is off. Keep the solar filter on.</p>
    </div>
    <Button small rounded inline class="bg-black! text-yellow-300!" onClick={solarOff}>Turn off</Button>
  </Block>
{/if}
{#if mount.connected && mount.error}
  <Block strong inset>
    <p class="text-sm">{mount.error}</p>
  </Block>
{/if}
