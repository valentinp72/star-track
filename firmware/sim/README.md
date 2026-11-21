# Simulator

Runs the mount firmware's core ([`../star-track/src/core/`](../star-track/src/core/))
on the PC, with a model of the mount in place of the ESP32 and its peripherals. It
speaks the protocol of [`docs/protocol.md`](../../docs/protocol.md) on stdin/stdout,
one JSON object per line, and adds the `sim_*` commands described there.

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
- `tests/test_session.py`: the simulator with a paused clock and a base tilted by
  1.5° with north at 140° on the azimuth axis. It aligns on Capella, Aldebaran and
  Deneb, goes to M42 and tracks it for 4 hours, then checks jogs, the hand unit, the
  E-stop, park, a position table, the GPS and the error replies. It takes about
  half a second.

## Run

```sh
./build/star-track-sim                 # real time, clock from the PC
./build/star-track-sim --rate 1000     # 1000 times faster
./build/star-track-sim --paused        # time moves only with sim_run
./build/star-track-sim --state mount.bin   # keep the flash records between runs
./build/star-track-sim --utc 1795294800000 # start at a given UTC (ms since 1970)
```

Type JSON lines; replies and events come back as lines. For example:

```
{"cmd":"set_time","id":1,"utc":1795294800000}
{"cmd":"set_site","id":2,"lat":45,"lon":5}
{"cmd":"sim_point","id":3,"ra":5.2782,"dec":45.998}
{"cmd":"align_add","id":4,"ra":5.2782,"dec":45.998,"name":"Capella"}
{"cmd":"goto","id":5,"ra":5.5881,"dec":-5.391}
{"cmd":"stream","id":6,"hz":1}
```

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
