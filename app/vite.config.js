import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import tailwindcss from '@tailwindcss/vite';

// `npm run dev` forwards the WebSocket to a running simulator, by default the
// one on localhost:8080. Another one: SIM=localhost:8090 npm run dev
const sim = process.env.SIM ?? 'localhost:8080';

export default defineConfig({
  base: './',
  plugins: [tailwindcss(), svelte()],
  server: {
    host: true, // reachable from the phone on the same network
    proxy: { '/ws': { target: `ws://${sim}`, ws: true } },
  },
  build: {
    target: 'safari16',
    assetsInlineLimit: 0,
  },
  test: {
    include: ['src/**/*.test.js'],
  },
});
