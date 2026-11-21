<!-- The solar filter check before solar mode (protocol: solar needs filter: true). -->
<script>
  import { Dialog, DialogButton } from 'konsta/svelte';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  // `request` is {then?: function} while the dialog is open, null otherwise.
  let { mount, request = $bindable(null) } = $props();

  const sep = $derived(mount.config?.['sun.min_sep'] ?? 15);

  async function confirm() {
    const then = request?.then;
    request = null;
    try {
      await mount.send('solar', { on: true, filter: true });
      await then?.();
    } catch (e) {
      ui.showAlert('Solar mode', explain(e));
    }
  }
</script>

<Dialog opened={!!request} onBackdropClick={() => (request = null)}>
  {#snippet title()}Solar filter check{/snippet}
  <div class="space-y-2 text-left">
    <p>
      Pointed at the Sun without a filter, the telescope blinds at once and its own parts can burn. Solar mode turns off
      the {sep}° keep-out zone around the Sun. Before turning it on:
    </p>
    <ul class="list-disc space-y-1 ps-5">
      <li>a certified solar filter (ISO 12312-2) covers the whole front of the tube, fixed so that it cannot fall off;</li>
      <li>the finder is capped or removed;</li>
      <li>nobody looks through the eyepiece while the mount moves.</li>
    </ul>
    <p class="opacity-70">Solar mode turns off again at park and at power-on.</p>
  </div>
  {#snippet buttons()}
    <DialogButton onClick={() => (request = null)}>Cancel</DialogButton>
    <DialogButton strong onClick={confirm}>Filter is on</DialogButton>
  {/snippet}
</Dialog>
