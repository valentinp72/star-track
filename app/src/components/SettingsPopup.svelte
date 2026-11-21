<!-- The mount's settings (get_config / set_config, saved in its flash). Only the
     changed keys are sent; the mount checks them all before changing any. -->
<script>
  import { untrack } from 'svelte';
  import {
    Popup, Page, Navbar, Link, Block, BlockTitle, BlockFooter, Button, List, ListInput, ListItem, Toggle, Dialog,
    DialogButton,
  } from 'konsta/svelte';
  import { explain } from '../lib/errors.js';
  import { ui } from '../lib/ui.svelte.js';

  let { mount, opened = $bindable(false) } = $props();

  // The keys of docs/protocol.md (Configuration), with the ranges of
  // firmware/star-track/src/core/config.cpp.
  const GROUPS = [
    {
      title: 'Altitude limits',
      footer: 'Gotos and jogs stop there.',
      keys: [
        { key: 'alt.min', label: 'Lowest (°)', min: -10, max: 90 },
        { key: 'alt.max', label: 'Highest (°)', min: 0, max: 95 },
      ],
    },
    {
      title: 'Speeds',
      footer: 'Goto speed and acceleration on the sky; jog speed at full deflection; tracking correction gain.',
      keys: [
        { key: 'slew.speed', label: 'Goto (°/s)', min: 0.1, max: 20 },
        { key: 'slew.accel', label: 'Acceleration (°/s²)', min: 0.1, max: 50 },
        { key: 'jog.max_speed', label: 'Joystick (°/s)', min: 0.01, max: 20 },
        { key: 'track.gain', label: 'Tracking gain (1/s)', min: 0.05, max: 10 },
      ],
    },
    {
      title: 'Sun',
      footer: 'Kept away from the Sun outside solar mode.',
      keys: [{ key: 'sun.min_sep', label: 'Keep-out radius (°)', min: 5, max: 45 }],
    },
    {
      title: 'Park position',
      footer: 'Axis angles from the power-on position.',
      keys: [
        { key: 'park.alt', label: 'Altitude axis (°)', min: -10, max: 95 },
        { key: 'park.az', label: 'Azimuth axis (°)', min: -360, max: 360 },
      ],
      park: true,
    },
    {
      title: 'Drives',
      footer: 'Microsteps per turn of each axis (200 × 64 × 250/20 = 160 000), and the motor directions.',
      keys: [
        { key: 'az.steps_per_rev', label: 'Azimuth steps per turn', min: 1000, max: 10000000, int: true },
        { key: 'alt.steps_per_rev', label: 'Altitude steps per turn', min: 1000, max: 10000000, int: true },
        { key: 'az.reverse', label: 'Reverse azimuth', bool: true },
        { key: 'alt.reverse', label: 'Reverse altitude', bool: true },
      ],
    },
    {
      title: 'Wi-Fi',
      footer: 'Applied at the next power-on. The password is never shown: leave it empty to keep it.',
      keys: [
        { key: 'wifi.ssid', label: 'Network name', text: true, minLen: 1, maxLen: 32 },
        { key: 'wifi.password', label: 'New password (8 to 63 characters)', text: true, minLen: 8, maxLen: 63, secret: true },
      ],
    },
  ];
  const KEYS = GROUPS.flatMap((g) => g.keys);

  // The fields as typed (strings, or booleans).
  let draft = $state({});
  let busy = $state(false);
  let confirmLevel = $state(false);

  function load() {
    const c = mount.config ?? {};
    const d = {};
    for (const k of KEYS) d[k.key] = k.bool ? !!c[k.key] : k.secret ? '' : c[k.key] == null ? '' : String(c[k.key]);
    draft = d;
  }
  $effect(() => {
    if (!opened) return;
    untrack(async () => {
      await mount.refreshConfig();
      load();
    });
  });

  const num = (t) => Number(String(t).trim().replace(',', '.'));
  function problem(k) {
    const v = draft[k.key] ?? ''; // not loaded yet
    if (k.bool) return '';
    if (k.text) {
      if (k.secret && v === '') return '';
      return v.length < k.minLen || v.length > k.maxLen ? `${k.minLen} to ${k.maxLen} characters` : '';
    }
    const n = num(v);
    if (v === '' || !Number.isFinite(n)) return 'A number';
    if (k.int && !Number.isInteger(n)) return 'A whole number';
    return n < k.min || n > k.max ? `${k.min} to ${k.max}` : '';
  }
  const problems = $derived(Object.fromEntries(KEYS.map((k) => [k.key, problem(k)])));
  const valid = $derived(KEYS.every((k) => !problems[k.key]));

  // The keys whose value differs from the mount's.
  const changes = $derived.by(() => {
    const c = mount.config ?? {};
    const out = {};
    for (const k of KEYS) {
      const v = draft[k.key];
      if (v === undefined) continue; // not loaded yet
      if (k.bool) {
        if (v !== !!c[k.key]) out[k.key] = v;
      } else if (k.text) {
        if (k.secret ? v !== '' : v !== c[k.key]) out[k.key] = v;
      } else if (v !== '' && num(v) !== c[k.key]) out[k.key] = num(v);
    }
    return out;
  });
  const changed = $derived(Object.keys(changes).length > 0);

  async function save() {
    busy = true;
    try {
      await mount.send('set_config', { config: changes });
      await mount.refreshConfig();
      load();
      ui.showToast('Settings saved on the mount');
    } catch (e) {
      ui.showAlert('Settings', explain(e));
    } finally {
      busy = false;
    }
  }

  // The park position from where the axes are now (axis degrees, with the
  // motor direction setting, as the firmware counts them).
  function parkHere() {
    const st = mount.status;
    const c = mount.config;
    if (!st || !c) return;
    const deg = (steps, spr, rev) => ((steps * 360) / spr) * (rev ? -1 : 1);
    draft['park.alt'] = String(Math.round(deg(st.axes.alt.steps, c['alt.steps_per_rev'], c['alt.reverse']) * 100) / 100);
    let az = deg(st.axes.az.steps, c['az.steps_per_rev'], c['az.reverse']);
    az = ((((az + 180) % 360) + 360) % 360) - 180;
    draft['park.az'] = String(Math.round(az * 100) / 100);
  }

  async function levelNow() {
    confirmLevel = false;
    try {
      await mount.send('set_alt_zero');
      ui.showToast('Altitude zero set; the alignment was cleared');
    } catch (e) {
      ui.showAlert('Tube level', explain(e));
    }
  }
</script>

<Popup {opened} onBackdropClick={() => (opened = false)}>
  <Page>
    <Navbar title="Mount settings">
      {#snippet right()}
        <Link onClick={() => (opened = false)}>Done</Link>
      {/snippet}
    </Navbar>

    {#if !mount.config}
      <Block class="text-center text-sm opacity-70">Not connected to the mount.</Block>
    {:else}
      {#each GROUPS as g (g.title)}
        <BlockTitle>{g.title}</BlockTitle>
        <List strong inset>
          {#each g.keys as k (k.key)}
            {#if k.bool}
              <ListItem title={k.label}>
                {#snippet after()}
                  <Toggle checked={draft[k.key]} onChange={() => (draft[k.key] = !draft[k.key])} />
                {/snippet}
              </ListItem>
            {:else}
              <ListInput
                label={k.label}
                type={k.secret ? 'password' : 'text'}
                inputmode={k.text ? 'text' : k.min < 0 ? 'text' : 'decimal'}
                autocapitalize="off"
                autocorrect="off"
                value={draft[k.key]}
                onInput={(e) => (draft[k.key] = e.target.value)}
                error={problems[k.key]}
              />
            {/if}
          {/each}
        </List>
        {#if g.park}
          <Block class="-mt-4!">
            <Button clear onClick={parkHere} disabled={!mount.connected}>Park where the tube is now</Button>
          </Block>
        {/if}
        <BlockFooter>{g.footer}</BlockFooter>
      {/each}

      <Block class="my-6!">
        <Button large rounded disabled={!changed || !valid || busy || !mount.connected} onClick={save}>
          {changed ? `Save ${Object.keys(changes).length} change${Object.keys(changes).length > 1 ? 's' : ''}` : 'No change'}
        </Button>
      </Block>

      <BlockTitle>Tube level</BlockTitle>
      <List strong inset>
        <ListItem link title="The tube is level now" onClick={() => (confirmLevel = true)} />
      </List>
      <BlockFooter>
        Sets the altitude axis to 0°. Only needed when the tube was far from level at power-on (more than 10°): the
        alignment finds smaller errors. Clears the alignment.
      </BlockFooter>
    {/if}
    <div class="h-[calc(var(--k-safe-area-bottom)+24px)]"></div>
  </Page>
</Popup>

<Dialog opened={confirmLevel} onBackdropClick={() => (confirmLevel = false)}>
  {#snippet title()}The tube is level?{/snippet}
  The altitude axis is set to 0° and the alignment stars are removed.
  {#snippet buttons()}
    <DialogButton onClick={() => (confirmLevel = false)}>Cancel</DialogButton>
    <DialogButton strong onClick={levelNow}>Set level</DialogButton>
  {/snippet}
</Dialog>
