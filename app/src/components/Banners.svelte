<!-- Warnings above every tab: no connection, E-stop pressed. -->
<script>
  import { Block, Preloader } from 'konsta/svelte';

  let { mount } = $props();

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
    <p class="mt-1 text-sm">The motors are off. Release the E-stop to move again.</p>
  </Block>
{:else if mount.error}
  <Block strong inset>
    <p class="text-sm">{mount.error}</p>
  </Block>
{/if}
