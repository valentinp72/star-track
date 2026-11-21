<!-- A round joystick. `onChange({x, y})` with x, y from −1 to 1 (right and up
     positive) while a finger is on it, and {x: 0, y: 0} when it lifts. Right is
     the protocol's positive x: clockwise seen from above. -->
<script>
  let { onChange, disabled = false, size = 240 } = $props();

  let pad;
  let x = $state(0);
  let y = $state(0);
  let pointer = $state(null);

  function update(e) {
    const r = pad.getBoundingClientRect();
    let nx = ((e.clientX - r.left) / r.width) * 2 - 1;
    let ny = -(((e.clientY - r.top) / r.height) * 2 - 1);
    const n = Math.hypot(nx, ny);
    if (n > 1) {
      nx /= n;
      ny /= n;
    }
    x = nx;
    y = ny;
    onChange?.({ x, y });
  }

  function down(e) {
    if (disabled || pointer != null) return;
    pointer = e.pointerId;
    pad.setPointerCapture(e.pointerId);
    update(e);
  }
  function move(e) {
    if (e.pointerId === pointer) update(e);
  }
  function up(e) {
    if (e.pointerId !== pointer) return;
    pointer = null;
    x = 0;
    y = 0;
    onChange?.({ x: 0, y: 0 });
  }

  // Disabled while held (E-stop, parked): let go.
  $effect(() => {
    if (disabled && pointer != null) up({ pointerId: pointer });
  });

  const knob = $derived(size * 0.36);
</script>

<div
  bind:this={pad}
  class="relative mx-auto touch-none select-none rounded-full bg-black/5 dark:bg-white/10 {disabled ? 'opacity-40' : ''}"
  style="width:{size}px;height:{size}px"
  onpointerdown={down}
  onpointermove={move}
  onpointerup={up}
  onpointercancel={up}
  role="slider"
  aria-label="Joystick"
  aria-valuenow={0}
  tabindex="-1"
>
  <!-- cross hairs and the arrows -->
  <div class="pointer-events-none absolute inset-0">
    <div class="absolute left-1/2 top-3 h-[calc(100%-24px)] w-px bg-current opacity-15"></div>
    <div class="absolute left-3 top-1/2 h-px w-[calc(100%-24px)] bg-current opacity-15"></div>
    <!-- right turns the mount clockwise seen from above (azimuth up), whichever way it points -->
    {#each [['top-2.5 left-1/2 -translate-x-1/2', 0], ['bottom-2.5 left-1/2 -translate-x-1/2', 180], ['left-2.5 top-1/2 -translate-y-1/2', 270], ['right-2.5 top-1/2 -translate-y-1/2', 90]] as [where, turn] (turn)}
      <svg class="absolute {where} opacity-50" width="12" height="12" viewBox="0 0 12 12" style="transform:rotate({turn}deg)">
        <path d="M6 1 11 10H1Z" fill="currentColor" />
      </svg>
    {/each}
  </div>
  <div
    class="bg-primary pointer-events-none absolute rounded-full shadow-lg {pointer == null ? 'transition-transform duration-150' : ''}"
    style="width:{knob}px;height:{knob}px;left:{(size - knob) / 2}px;top:{(size - knob) / 2}px;transform:translate({(x * (size - knob)) / 2}px,{(-y * (size - knob)) / 2}px)"
  ></div>
</div>
