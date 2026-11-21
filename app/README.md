# Phone app

The web app that drives the mount from an iPhone. The mount serves it over its own
Wi-Fi access point at `http://192.168.4.1/`, and it is added to the home screen
with Safari's *Share → Add to Home Screen*, so it opens full screen like an app.
It speaks the protocol of [`docs/protocol.md`](../docs/protocol.md) on a
WebSocket at `/ws`. Design choices: [Phone interface](../docs/architecture.md#phone-interface).

Status: the connection and the mount's status. The catalogue, goto, joystick,
alignment and sky chart are still to come.

## Libraries

| Library | Use |
| --- | --- |
| [Svelte](https://svelte.dev) 5 | Components and reactive state. Compiled, no runtime framework to load. |
| [Konsta UI](https://konstaui.com) 5 | iOS-style components (navigation bar, tab bar, lists, segmented controls, sheets), on Tailwind CSS 4. |
| [Astronomy Engine](https://github.com/cosinekitty/astronomy) | Sun, Moon and planet positions, precession (not used yet). |
| [Vite](https://vite.dev) | Development server and build. |
| [Vitest](https://vitest.dev) | Tests of the logic. |

All MIT licensed. Versions are pinned in `package.json`.

## Develop

Needs Node.js 24 (`.nvmrc`; with nvm: `nvm install` then `nvm use` in this folder).

```sh
npm install
npm run dev      # development server on port 5173, reloads on every change
npm test         # tests
npm run build    # the app in dist/, as the mount and the simulator serve it
```

`npm run dev` needs a running simulator ([`firmware/sim/`](../firmware/sim/)): it
forwards the WebSocket to `localhost:8080`, or to another one with
`SIM=localhost:8090 npm run dev`. Open <http://localhost:5173/>.

On the iPhone, on the same network as the PC: open `http://<the PC's address>:5173/`
in Safari. The development server listens on every interface for this; the
simulator itself can stay local.

Without the development server, the simulator serves the last build at
<http://localhost:8080/> (and at `http://<the PC's address>:8080/` with `--any`).

## Layout

| Path | What it is |
| --- | --- |
| `src/App.svelte` | The tab bar (Sky, Objects, Control, Setup), the navigation bar with the mount's mode, the banners. |
| `src/lib/mount.svelte.js` | The connection: `hello` and `set_time` on each connection, replies matched to commands, reconnection, a dead link detected after 3 s without a message. In the simulator, it polls `status` instead of the stream (the simulated clock may run 1000 times faster). |
| `src/lib/appearance.svelte.js` | Auto (follows the phone), dark, or night (red). Kept in the phone's storage. |
| `src/lib/format.js` | Text for coordinates, times and modes. |
| `src/tabs/` | One component per tab. |
| `src/components/` | Pieces shared by the tabs. |
| `public/` | Files copied as they are: the web manifest. |

## Notes for iOS

- Plain HTTP (the mount has no certificate): no service worker, no geolocation,
  no screen wake lock. The app is always loaded from the mount, the phone sends
  its clock, and the site comes from the GPS or a saved site.
- iOS suspends a page in the background and may drop its socket without telling
  it. When the app comes back to the foreground it checks the link, reconnects if
  needed and sends its clock again.
- The home-screen app keeps its own storage, apart from Safari's.
- In the home-screen app (status bar `black-translucent`), iOS 26 blurs a band
  about 24 px below the status bar, which Safari's tab does not. `src/app.css`
  moves everything under the top safe area down by that much, in standalone mode
  only. iOS reads the status bar style when the icon is added: re-add the icon
  after changing it.
