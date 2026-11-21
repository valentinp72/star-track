<!-- Line icons for the tab bar and lists, 24 × 24, drawn with the current colour. -->
<script>
  let { name, size = 26, class: className = '' } = $props();

  const PATHS = {
    // a four-point star and two small ones
    sky: 'M10 3c.6 3.6 2.4 5.4 6 6-3.6.6-5.4 2.4-6 6-.6-3.6-2.4-5.4-6-6 3.6-.6 5.4-2.4 6-6Z M18 13.5c.3 1.7 1 2.5 2.5 2.8-1.5.3-2.2 1.1-2.5 2.7-.3-1.6-1-2.4-2.5-2.7 1.5-.3 2.2-1.1 2.5-2.8Z M6 17.5c.2 1 .7 1.5 1.5 1.7-.8.2-1.3.7-1.5 1.8-.2-1.1-.7-1.6-1.5-1.8.8-.2 1.3-.7 1.5-1.7Z',
    // a list with bullets
    objects: 'M9 6h11 M9 12h11 M9 18h11 M4.5 6h.01 M4.5 12h.01 M4.5 18h.01',
    // a joystick: a ring and four arrows
    control: 'M12 9.5a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5Z M12 2.5l-2 2.5h4Z M12 21.5l-2-2.5h4Z M2.5 12l2.5-2v4Z M21.5 12l-2.5-2v4Z',
    // a gear
    setup: 'M12 8.5a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7Z M10.3 2.5h3.4l.5 2.6 1.7 1 2.5-.9 1.7 2.9-2 1.7v2l2 1.7-1.7 2.9-2.5-.9-1.7 1-.5 2.6h-3.4l-.5-2.6-1.7-1-2.5.9-1.7-2.9 2-1.7v-2l-2-1.7 1.7-2.9 2.5.9 1.7-1Z',
  };
</script>

<svg
  class={className}
  width={size}
  height={size}
  viewBox="0 0 24 24"
  fill="none"
  stroke="currentColor"
  stroke-width="1.7"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d={PATHS[name]} />
</svg>
