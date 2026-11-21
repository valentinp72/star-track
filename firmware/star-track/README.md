# Mount firmware

The firmware for the ESP32-S3 on the mount. Work in progress: the core is written
and runs in the [simulator](../sim/); the ESP32 platform layer and the sketch are
not written yet.

| Path | What it is |
| --- | --- |
| [`src/core/`](src/core/) | Everything that does not touch the hardware: the protocol of [`docs/protocol.md`](../../docs/protocol.md), modes, tracking control, pointing model, time and coordinates, NMEA reader, settings. |
| `src/esp32/` | Planned: the platform layer, with FastAccelStepper, the Wi-Fi access point, the web server and NVS. |

## Rules for `src/core/`

The same files build for the ESP32 (arduino-cli) and for the PC (the simulator's
Makefile), so the core:

- includes no Arduino header and calls no `millis()`, `delay()` or `String`;
- gets time only from `Clock`, and reaches the hardware only through the
  interfaces in [`platform.h`](src/core/platform.h);
- uses ArduinoJson 7, through [`json.h`](src/core/json.h) for the same settings
  on both builds (64-bit integers, doubles).

## Files

| File | Contents |
| --- | --- |
| `platform.h` | Clock, Axis, EStop, Inputs, Gps, Leds, Storage, Link. |
| `mount.h`, `mount.cpp` | Modes, the 20 Hz control loop, the commands and events. |
| `align.h`, `align.cpp` | Pointing model: a rotation (Horn's method) and an altitude index, from one star (azimuth offset and index, level base assumed) or two stars and more (best fit); sync. |
| `astro.h`, `astro.cpp` | Sidereal time, RA/Dec ↔ alt/az, refraction, vectors and matrices. |
| `nmea.h`, `nmea.cpp` | RMC and GGA sentences from the GPS. |
| `config.h`, `config.cpp` | The settings and their `get_config` / `set_config` keys. |

## Control

Every 50 ms, each axis gets a speed: the target's predicted rate (from its
position now and one second later) plus a correction for the position error. The
correction is the smallest of the slew speed, the speed that can still brake
before the target, and `track.gain` × error. Gotos and tracking use the same law;
a goto ends when both axes are within 0.005° (18″).
