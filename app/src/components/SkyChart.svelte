<!-- The sky above the site: stereographic projection centred on the zenith,
     north up and east left (as seen looking up). Stars to magnitude 5, deep-sky
     objects, Sun, Moon and planets, the Sun's keep-out zone, the alignment stars,
     the telescope and its target. Pinch to zoom, drag to pan, tap to pick. -->
<script>
  import { onMount, untrack } from 'svelte';
  import { bodies, dsos, stars } from '../lib/catalog.js';
  import { apparent, horizon, quickHorizon } from '../lib/sky.js';

  // `onPick(obj)` when an object is tapped. `alignStars`: [{ra, dec}] of date.
  // `follow`: keep the telescope in the middle (zoomed in).
  let { mount, targets, onPick, alignStars = [], follow = $bindable(false) } = $props();

  const D = Math.PI / 180;
  const BODY_STYLE = {
    Sun: ['#f5c518', 7], Moon: ['#e3e6ee', 6], Mercury: ['#b0a89c', 2.5], Venus: ['#f3e7c0', 3.5],
    Mars: ['#e2704a', 3], Jupiter: ['#e6c9a0', 3.5], Saturn: ['#e3cf8f', 3], Uranus: ['#9fd6e0', 2.5],
    Neptune: ['#7a9cf0', 2.5],
  };

  let canvas;
  let box;
  let width = $state(0);
  // View: zoom ≥ 1, and the dome point (unit disc coordinates) at the centre.
  let zoom = $state(1);
  let panU = $state(0);
  let panV = $state(0);

  // alt/az -> unit disc (stereographic, horizon at radius 1).
  function disc(alt, az) {
    const k = Math.tan(((90 - alt) * D) / 2);
    return [-k * Math.sin(az * D), -k * Math.cos(az * D)];
  }

  // Positions, every 5 s (the sky turns 1.25′ in that time).
  let positions = $state.raw(null);
  function compute() {
    const site = mount.site;
    if (!site) {
      positions = null;
      return;
    }
    const utc = mount.now();
    const q = quickHorizon(utc, site);
    const fixed = (o) => {
      const h = q(o.ra, o.dec);
      return { o, alt: h.alt, az: h.az };
    };
    const solar = bodies.map((b) => {
      const p = apparent(b, utc, site);
      return { o: b, ...horizon(p.ra, p.dec, utc, site), ra: p.ra, dec: p.dec };
    });
    positions = {
      utc,
      stars: stars.map(fixed).filter((p) => p.alt > -1),
      dsos: dsos.map(fixed).filter((p) => p.alt > -1),
      bodies: solar,
    };
  }
  $effect(() => {
    mount.site; // recompute when the site changes
    untrack(compute);
    const t = setInterval(compute, 5000);
    return () => clearInterval(t);
  });

  // Follow the telescope: keep its position in the middle.
  $effect(() => {
    if (follow && untrack(() => zoom) < 3) zoom = 3;
  });
  $effect(() => {
    const p = mount.status?.pos;
    if (!follow || !p || p.alt == null || p.az == null) return;
    const [u, v] = disc(p.alt, p.az);
    panU = u;
    panV = v;
  });

  // ------------------------------------------------------------ drawing

  let hits = []; // [{x, y, o, rank}] for tapping
  let queued = false;
  function redraw() {
    if (queued) return;
    queued = true;
    requestAnimationFrame(() => {
      queued = false;
      draw();
    });
  }
  // Redraw on any change of what is drawn.
  $effect(() => {
    positions;
    mount.status;
    zoom;
    panU;
    panV;
    width;
    alignStars;
    targets.current;
    redraw();
  });

  function draw() {
    if (!canvas || !width) return;
    const dpr = window.devicePixelRatio || 1;
    const w = width;
    const h = width;
    if (canvas.width !== Math.round(w * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const R = (w / 2 - 14) * zoom;
    const cx = w / 2 - panU * R;
    const cy = h / 2 - panV * R;
    const P = (alt, az) => {
      const [u, v] = disc(alt, az);
      return [cx + u * R, cy + v * R];
    };
    const inView = (x, y, m = 20) => x > -m && x < w + m && y > -m && y < h + m;
    hits = [];

    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, w, h);
    // The sky inside the horizon.
    ctx.save();
    ctx.beginPath();
    ctx.arc(cx, cy, R, 0, 2 * Math.PI);
    ctx.fillStyle = '#0a1022';
    ctx.fill();
    ctx.clip();

    // Altitude circles every 30° and the meridian lines every 45°.
    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 1;
    for (const a of [30, 60]) {
      ctx.beginPath();
      ctx.arc(cx, cy, R * Math.tan(((90 - a) * D) / 2), 0, 2 * Math.PI);
      ctx.stroke();
    }
    for (let az = 0; az < 360; az += 45) {
      const [x, y] = P(0, az);
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(x, y);
      ctx.stroke();
    }

    const labelSize = 11;
    ctx.font = `${labelSize}px -apple-system, system-ui, sans-serif`;
    ctx.textBaseline = 'middle';
    // Labels are drawn last, most important first, and skipped where they
    // would overlap one already drawn.
    const labels = []; // {text, x, y, color, rank}
    const label = (text, x, y, color, rank) => labels.push({ text, x, y, color, rank });
    const pos = positions;
    if (pos) {
      // Stars: size by magnitude; names for the bright ones, more when zoomed.
      const nameMag = 1.2 + Math.log2(zoom) * 1.2;
      for (const p of pos.stars) {
        if (p.alt < 0) continue;
        const [x, y] = P(p.alt, p.az);
        if (!inView(x, y)) continue;
        const r = Math.max(0.6, (5.4 - p.o.mag) * 0.55) * Math.min(1.6, Math.sqrt(zoom));
        ctx.fillStyle = '#fff';
        ctx.globalAlpha = Math.min(1, 0.35 + (5 - p.o.mag) * 0.2);
        ctx.beginPath();
        ctx.arc(x, y, r, 0, 2 * Math.PI);
        ctx.fill();
        ctx.globalAlpha = 1;
        if (p.o.name) {
          hits.push({ x, y, o: p.o, rank: 2 });
          if (p.o.mag <= nameMag) label(p.o.name, x + r + 3, y, 'rgba(255,255,255,0.6)', 1 + p.o.mag / 10);
        }
      }
      // Deep sky: open circles; Messier names when zoomed.
      const showDso = zoom >= 1.5;
      for (const p of pos.dsos) {
        if (p.alt < 0 || (!p.o.messier && !showDso)) continue;
        const [x, y] = P(p.alt, p.az);
        if (!inView(x, y)) continue;
        ctx.strokeStyle = 'rgba(143,170,230,0.8)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, 2 * Math.PI);
        ctx.stroke();
        hits.push({ x, y, o: p.o, rank: 1 });
        if ((p.o.messier && zoom >= 2) || zoom >= 6)
          label(p.o.title, x + 5, y, 'rgba(143,170,230,0.85)', p.o.messier ? 2 : 3);
      }
      // The Sun's keep-out zone.
      const sun = pos.bodies.find((b) => b.o.body === 'Sun');
      const sep = mount.config?.['sun.min_sep'] ?? 15;
      if (sun && sun.alt > -sep) drawSunZone(ctx, P, sun.alt, sun.az, sep, mount.status?.solar);
      // Sun, Moon, planets, always named.
      for (const p of pos.bodies) {
        if (p.alt < 0) continue;
        const [x, y] = P(p.alt, p.az);
        if (!inView(x, y)) continue;
        const [color, r] = BODY_STYLE[p.o.body];
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(x, y, r, 0, 2 * Math.PI);
        ctx.fill();
        label(p.o.title, x + r + 4, y, color, 0);
        hits.push({ x, y, o: p.o, rank: 0 });
      }
    }

    labels.sort((a, b) => a.rank - b.rank);
    const placed = [];
    for (const l of labels) {
      const lw = ctx.measureText(l.text).width;
      const box = [l.x - 1, l.y - labelSize / 2 - 1, l.x + lw + 1, l.y + labelSize / 2 + 1];
      if (placed.some((b) => box[0] < b[2] && box[2] > b[0] && box[1] < b[3] && box[3] > b[1])) continue;
      placed.push(box);
      ctx.fillStyle = l.color;
      ctx.fillText(l.text, l.x, l.y);
    }

    // The alignment stars: rings.
    const site = mount.site;
    if (site && pos) {
      ctx.strokeStyle = 'rgba(255,255,255,0.7)';
      ctx.lineWidth = 1;
      for (const a of alignStars) {
        const h2 = horizon(a.ra, a.dec, pos.utc, site);
        if (h2.alt < 0) continue;
        const [x, y] = P(h2.alt, h2.az);
        ctx.beginPath();
        ctx.arc(x, y, 8, 0, 2 * Math.PI);
        ctx.stroke();
      }
    }
    ctx.restore();

    // The horizon and the cardinal points.
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(cx, cy, R, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.font = `600 13px -apple-system, system-ui, sans-serif`;
    ctx.textAlign = 'center';
    ctx.fillStyle = 'rgba(255,255,255,0.7)';
    for (const [l, az] of [['N', 0], ['E', 90], ['S', 180], ['W', 270]]) {
      const [x, y] = P(-4, az);
      if (inView(x, y, -6)) ctx.fillText(l, x, y);
    }
    ctx.textAlign = 'left';

    // The target and the telescope.
    const st = mount.connected ? mount.status : null;
    const primary = getComputedStyle(canvas).getPropertyValue('--k-color-primary').trim() || '#007aff';
    if (st?.target?.alt != null && st.target.az != null) {
      const [x, y] = P(st.target.alt, st.target.az);
      ctx.strokeStyle = '#ffb02e';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(x, y - 9);
      ctx.lineTo(x + 9, y);
      ctx.lineTo(x, y + 9);
      ctx.lineTo(x - 9, y);
      ctx.closePath();
      ctx.stroke();
    }
    if (st?.pos?.alt != null && st.pos.az != null) {
      const [x, y] = P(st.pos.alt, st.pos.az);
      ctx.strokeStyle = primary;
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(x, y, 12, 0, 2 * Math.PI);
      for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
        ctx.moveTo(x + dx * 12, y + dy * 12);
        ctx.lineTo(x + dx * 20, y + dy * 20);
      }
      ctx.stroke();
    }
  }

  function drawSunZone(ctx, P, alt0, az0, sep, solar) {
    const a0 = alt0 * D;
    const z0 = az0 * D;
    const r = sep * D;
    ctx.beginPath();
    for (let i = 0; i <= 72; i++) {
      const b = (i / 72) * 2 * Math.PI;
      const alt = Math.asin(Math.sin(a0) * Math.cos(r) + Math.cos(a0) * Math.sin(r) * Math.cos(b));
      const az = z0 + Math.atan2(Math.sin(b) * Math.sin(r) * Math.cos(a0), Math.cos(r) - Math.sin(a0) * Math.sin(alt));
      const [x, y] = P(Math.max(alt / D, -20), az / D);
      if (i) ctx.lineTo(x, y);
      else ctx.moveTo(x, y);
    }
    ctx.closePath();
    ctx.fillStyle = solar ? 'rgba(245,197,24,0.06)' : 'rgba(245,197,24,0.16)';
    ctx.fill();
    ctx.setLineDash(solar ? [3, 5] : [6, 4]);
    ctx.strokeStyle = '#f5c518';
    ctx.lineWidth = 1.2;
    ctx.stroke();
    ctx.setLineDash([]);
  }

  // ------------------------------------------------------------ touch

  const pointers = new Map(); // id -> {x, y}
  let gesture = null; // {kind, ...} while fingers are down

  function local(e) {
    const r = canvas.getBoundingClientRect();
    return { x: e.clientX - r.left, y: e.clientY - r.top };
  }
  function clampView() {
    zoom = Math.min(12, Math.max(1, zoom));
    const lim = Math.max(0, 1 - 1 / zoom) + 0.05;
    panU = Math.max(-lim, Math.min(lim, panU));
    panV = Math.max(-lim, Math.min(lim, panV));
    if (zoom === 1) panU = panV = 0;
  }
  // Zoom by `f` keeping the screen point (x, y) fixed.
  function zoomAt(x, y, f) {
    const R0 = (width / 2 - 14) * zoom;
    const u = panU + (x - width / 2) / R0;
    const v = panV + (y - width / 2) / R0;
    zoom = Math.min(12, Math.max(1, zoom * f));
    const R1 = (width / 2 - 14) * zoom;
    if (!follow) {
      panU = u - (x - width / 2) / R1;
      panV = v - (y - width / 2) / R1;
    }
    clampView();
  }

  function down(e) {
    canvas.setPointerCapture(e.pointerId);
    pointers.set(e.pointerId, local(e));
    if (pointers.size === 1) {
      const p = local(e);
      gesture = { kind: 'tap', x0: p.x, y0: p.y, t0: performance.now(), u0: panU, v0: panV };
    } else if (pointers.size === 2) {
      const [a, b] = [...pointers.values()];
      gesture = { kind: 'pinch', d0: Math.hypot(a.x - b.x, a.y - b.y), z0: zoom, mx: (a.x + b.x) / 2, my: (a.y + b.y) / 2 };
    }
  }
  function move(e) {
    if (!pointers.has(e.pointerId)) return;
    pointers.set(e.pointerId, local(e));
    if (gesture?.kind === 'pinch' && pointers.size === 2) {
      const [a, b] = [...pointers.values()];
      const d = Math.hypot(a.x - b.x, a.y - b.y);
      const target = gesture.z0 * (d / gesture.d0);
      zoomAt(gesture.mx, gesture.my, target / zoom);
    } else if (gesture && pointers.size === 1) {
      const p = local(e);
      const dx = p.x - gesture.x0;
      const dy = p.y - gesture.y0;
      if (gesture.kind === 'tap' && Math.hypot(dx, dy) > 8) gesture.kind = 'pan';
      if (gesture.kind === 'pan' && zoom > 1) {
        follow = false;
        const R = (width / 2 - 14) * zoom;
        panU = gesture.u0 - dx / R;
        panV = gesture.v0 - dy / R;
        clampView();
      }
    }
  }
  function up(e) {
    if (!pointers.has(e.pointerId)) return;
    pointers.delete(e.pointerId);
    if (gesture?.kind === 'tap' && pointers.size === 0 && performance.now() - gesture.t0 < 500) pick(gesture.x0, gesture.y0);
    if (pointers.size === 0) gesture = null;
    else if (gesture?.kind === 'pinch') gesture = null; // one finger left: wait for a new gesture
  }
  function wheel(e) {
    e.preventDefault();
    const p = local(e);
    zoomAt(p.x, p.y, Math.exp(-e.deltaY * 0.002));
  }

  // The nearest object within 24 px: the Sun, Moon and planets first, then deep
  // sky, then stars.
  function pick(x, y) {
    let best = null;
    for (const h of hits) {
      const d = Math.hypot(h.x - x, h.y - y);
      if (d > 24) continue;
      const score = d + h.rank * 6;
      if (!best || score < best.score) best = { o: h.o, score };
    }
    if (best) onPick?.(best.o);
  }

  onMount(() => {
    // Applied on the next frame: setting the size from inside the observer
    // would trigger it again in the same frame.
    const ro = new ResizeObserver(() => requestAnimationFrame(() => (width = box.clientWidth)));
    ro.observe(box);
    canvas.addEventListener('wheel', wheel, { passive: false });
    return () => {
      ro.disconnect();
      canvas.removeEventListener('wheel', wheel);
    };
  });

  export function resetView() {
    zoom = 1;
    panU = panV = 0;
    follow = false;
  }
</script>

<div bind:this={box} class="relative w-full overflow-hidden rounded-3xl" style="height:{width}px">
  <canvas
    bind:this={canvas}
    class="block touch-none"
    style="width:{width}px;height:{width}px"
    onpointerdown={down}
    onpointermove={move}
    onpointerup={up}
    onpointercancel={up}
  ></canvas>
  {#if !mount.site}
    <div class="absolute inset-0 flex items-center justify-center p-8 text-center text-sm text-white/70">
      The mount does not know its site yet: set it in Setup.
    </div>
  {/if}
</div>
