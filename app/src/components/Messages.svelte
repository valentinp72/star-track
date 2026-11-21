<!-- The app's toast and alert dialog (see lib/ui.svelte.js). -->
<script>
  import { Dialog, DialogButton, Toast } from 'konsta/svelte';
  import { ui } from '../lib/ui.svelte.js';
</script>

<Toast position="center" opened={!!ui.toast} class="bottom-[calc(var(--k-safe-area-bottom)+112px)]!">
  {ui.toast ?? ''}
</Toast>

<Dialog opened={!!ui.alert} onBackdropClick={() => (ui.alert = null)}>
  {#snippet title()}{ui.alert?.title ?? ''}{/snippet}
  {ui.alert?.text ?? ''}
  {#snippet buttons()}
    <DialogButton strong onClick={() => (ui.alert = null)}>OK</DialogButton>
  {/snippet}
</Dialog>
