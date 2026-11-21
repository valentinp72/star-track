# Simulator

Runs the mount firmware's core ([`../star-track/src/core/`](../star-track/src/core/))
on the PC, with a model of the mount in place of the ESP32 and its peripherals. It
speaks the protocol of [`docs/protocol.md`](../../docs/protocol.md) on a WebSocket
and on stdin/stdout, adds the `sim_*` commands described there, and serves the
phone app ([`app/`](../../app/)) and a page that shows what the simulated mount
does.

The core is the same code the ESP32 will run: only the platform layer differs
(`sim.cpp` here, `src/esp32/` on the mount, not written yet).

## Build and test

Needs g++ (C++17), make, Python 3 and ArduinoJson 7. ArduinoJson is the library
the ESP32 build uses; the Makefile looks for it in the Arduino libraries folder:

```sh
arduino-cli lib install ArduinoJson@7.2.1
```

Then, in `firmware/sim/`:

```sh
make          # builds build/star-track-sim
make test     # unit tests, then a scripted night on the simulator
```

Another ArduinoJson copy can be given with `make ARDUINOJSON=/path/to/ArduinoJson/src`.

`make test` runs:

- `tests/test_core.cpp`: sidereal time against Meeus's examples, alt/az and
  refraction round trips, the NMEA reader, and the pointing model on a tilted base.
- `tests/test_fuzz.py`: random sequences of user actions (gotos, jogs, time jumps,
  tracking, park, tables, syncs, alignment stars, E-stops and their recovery,
  setting changes) on bases with a random tilt and north, and a tube up to 5° off
  level at power-on. After every action the
  firmware's position must be within 1′ of where the tube really points, and the
  mode, target, speeds and replies must be valid. `make test` runs 20 seeds of 200
  actions; `make fuzz` runs 200 seeds of 300 (about a minute).
- `tests/test_session.py`: the simulator with a paused clock, a base tilted by
  1.5° with north at 140° on the azimuth axis, and a tube 4° above level at
  power-on (and first 32°, which the first alignment star must refuse). It aligns
  on Capella, Aldebaran and Deneb, goes to M42 and tracks it for 4 hours, then checks jogs, the hand unit, the
  E-stop, park, a position table, the GPS and the error replies. It takes about
  half a second.

## Run

```sh
./build/star-track-sim
```

Then open <http://localhost:8080/sim> in a browser for the simulator page, and
<http://localhost:8080/> for the phone app (its build, `app/dist/`: run
`npm run build` in `app/` first; see [`app/README.md`](../../app/README.md)). The
simulator page shows:

- a sky chart with where the tube really points (`sim_truth`), where the firmware
  thinks it points (`status`) and the target, plus an eyepiece view (1.4°, the
  114/900 with a 25 mm eyepiece) and the two axes;
- a 3D view of the mount at its true axis angles, with the base's tilt and north,
  and rays to where the tube points and to the target. It is a simplified copy of
  [`docs/3d/`](../../docs/3d/) (same dimensions) and loads three.js from a CDN, so
  it needs an internet connection; the rest of the page works without one;
- the readouts, with the pointing error (truth to target) and the model error
  (truth to the firmware's belief);
- buttons for the mount commands, a catalogue (Sun, Moon and planets, bright stars,
  Messier objects; or click one on the chart), and *Quick start*: time, site and a
  three-star alignment in one click. The Moon and planets are sent as `track_table`
  positions every 10 minutes for 6 hours, renewed while they are tracked, as the
  phone app will do; their positions come from Astronomy Engine (loaded from a CDN);
- the Sun's keep-out zone on the sky chart, and solar mode behind a solar-filter
  check (the page asks before sending `solar`);
- two joysticks: the phone app's (`jog` every 100 ms) and the hand unit's
  (`sim_hand`), and the hand unit's buttons;
- the simulator's controls: clock speed, time jumps, E-stop, GPS, the base's tilt
  and north, axis slips;
- every message sent and received, and a box to send any command.

After an E-stop, a panel under the header lists the way back, with a button for
each step: release the E-stop, send `arm`, then centre a known object and sync on
it (the position is uncertain because the motors were free).

The page polls `status` and `sim_truth` ten times per real second, so it keeps up
at any clock speed. Gotos and jogs run at ×10 at most (the *slews at most* choice:
×1, ×10 or no limit), so they can be watched; the clock goes back to its speed once
the mount tracks or stops. Actions made of several messages (*Quick
start*, *Centre*, *Align*, *Sync*, a goto to the Moon or a planet) pause the clock
while they run, so that no time passes between them.

Options:

```sh
./build/star-track-sim --rate 1000     # 1000 times faster
./build/star-track-sim --paused        # time moves only with sim_run
./build/star-track-sim --state mount.bin   # keep the flash records between runs
./build/star-track-sim --utc 1795294800000 # start at a given UTC (ms since 1970)
./build/star-track-sim --port 8081     # another port; --port 0: no server
./build/star-track-sim --any           # reachable from other devices, e.g. a phone
./build/star-track-sim --app DIR       # serve the phone app from DIR (default: app/dist)
```

The protocol is also on stdin/stdout: type JSON lines, and replies and events come
back as lines. For example:

```
{"cmd":"set_time","id":1,"utc":1795294800000}
{"cmd":"set_site","id":2,"lat":45,"lon":5}
{"cmd":"sim_point","id":3,"ra":5.2782,"dec":45.998}
{"cmd":"align_add","id":4,"ra":5.2782,"dec":45.998,"name":"Capella"}
{"cmd":"goto","id":5,"ra":5.5881,"dec":-5.391}
{"cmd":"stream","id":6,"hz":1}
```

The HTTP and WebSocket server (`server.cpp`) is a small single-threaded one: the
main loop polls its sockets next to stdin, so the firmware's core never runs on
two threads. It serves the phone app's build at `/`, the simulator page
(`web/sim.html`) at `/sim` and the protocol on `/ws`, up to 4 WebSocket clients.

## The model

- Each axis is a stepper counter with a speed ramp, like FastAccelStepper. The
  mechanics follow the counter while the drivers are on. With the E-stop pressed,
  pulses still count but nothing moves, so the position becomes wrong, as on the
  mount.
- Both axes have 160 000 microsteps per turn. A different `*.steps_per_rev` in the
  firmware's configuration shows up as a pointing error.
- `sim_mount` tilts the base and rotates north relative to the azimuth axis;
  `sim_truth` returns where the tube really points. The true site is the GPS site
  (`sim_gps`, 45° N 5° E by default).
- The GPS sends RMC and GGA sentences once per simulated second when `sim_gps` has
  turned it on (it is off at start).
- The simulator runs the firmware's `loop()` every 5 ms of simulated time.
