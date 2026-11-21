# Mount firmware

The firmware for the ESP32-S3 on the mount: an Arduino sketch, built with
arduino-cli. The core runs in the [simulator](../sim/) as well. The ESP32 layer
compiles but is not tested on the board yet.

| Path | What it is |
| --- | --- |
| [`src/core/`](src/core/) | Everything that does not touch the hardware: the protocol of [`docs/protocol.md`](../../docs/protocol.md), modes, tracking control, pointing model, time and coordinates, NMEA reader, settings. |
| [`src/esp32/`](src/esp32/) | The platform layer: steppers (FastAccelStepper), E-stop, hand unit, GPS, LEDs, NVS, the Wi-Fi access point, the web server and the serial console. |
| `star-track.ino` | Connects the two and runs the loop. |
| `partitions.csv` | The flash layout of [`docs/architecture.md`](../../docs/architecture.md#flash-layout). |
| `app-fs.sh` | Makes the LittleFS image with the phone app (and its update for `ota.sh`), and writes it to the board over USB. |
| `ota.sh` | Sends a new firmware or app image to the mount over Wi-Fi. |

## Build and upload

The board is an ESP32-S3-N16R8 (16 MB flash, octal PSRAM); `sketch.yaml` sets
these options. One-time setup, besides the ESP32 core of the
[board test](../board-test/README.md):

```sh
arduino-cli lib install ArduinoJson@7.2.1 FastAccelStepper@2.0.1 \
  "Async TCP@3.5.0" "ESP Async WebServer@3.12.1"
```

Connect the board's **UART** USB port, then from the repository's root:

```sh
arduino-cli compile --upload -p /dev/ttyUSB0 firmware/star-track
(cd app && npm run build)
firmware/star-track/app-fs.sh /dev/ttyUSB0
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

The firmware and the app are written to separate partitions: each can be
updated without the other. Without the app's image, the mount still answers on
the WebSocket and the serial console.

## Updates over Wi-Fi

From a computer joined to the mount's network:

```sh
firmware/star-track/ota.sh firmware   # compiles, then sends build/firmware/star-track.ino.bin
firmware/star-track/ota.sh app        # makes build/app-fs.ota, then sends it
```

Both send the file to `POST /update` (`?target=app` for the app's image) as
multipart form data, for example with
`curl -F file=@star-track.ino.bin http://192.168.4.1/update`.

- The mount must be idle, parked or in E-stop, with both axes still; otherwise
  the reply is 409 and nothing is written. While the image is written, the
  drivers are off, the mount does not run, and commands get the error
  `updating`. Then the mount restarts: park it first to keep the alignment.
- The app's update, `build/app-fs.ota`, holds only the blocks of the LittleFS
  image that are not blank (about 400 of 2528), each after its number: 1.6 MB
  to send and to write instead of 9.9 MB. The other blocks keep old data, which
  LittleFS never reads (checked by unpacking an image whose blank blocks were
  filled with random bytes).
- A firmware image starts with the byte 0xE9, the app's update with `STFS`:
  the wrong one for the target is refused.
- A new firmware goes to the other app slot and runs on trial. It confirms
  itself once the access point has been up for 30 s (`new firmware confirmed`
  on the console). If it crashes or is reset before, the bootloader starts the
  previous firmware again. Each boot prints the firmware's version and slot.
- The app's image has one partition only: an interrupted upload leaves the app
  missing until the next one. The firmware, the WebSocket and the serial
  console keep working.
- Anyone on the mount's Wi-Fi can send an update, as they can drive the mount:
  the Wi-Fi password guards both.

## On the board

- **Wi-Fi.** An access point named after `wifi.ssid` (default `star-track`),
  WPA2, at most 4 stations. At the first boot the firmware makes a 12-character
  password and saves it with the settings. Each boot prints the network name
  and password on the serial console, as a `log` line. The app is at
  `http://192.168.4.1/` (or `http://star-track.local/`), the WebSocket at `/ws`.
  A new `wifi.ssid` or `wifi.password` applies at the next power-on.
- **Settings.** A JSON record in NVS, read key by key, so another firmware
  version keeps the settings it knows (the Wi-Fi password among them) and
  skips the others. Firmware 0.1.0's record (the struct as it is) is converted
  at the first boot.
- **Web server.** ESPAsyncWebServer, in AsyncTCP's task. WebSocket events wait
  in a queue until the loop hands them to the core, so the core runs in one
  task only. A fifth WebSocket client is refused. Files under `assets/` are
  cached for a year (their names change with their contents), the others are
  checked again on each load.
- **Serial console.** 115200 baud on the UART port, one JSON object per line.
  The ROM's boot messages come before the first JSON line.
- **Steppers.** FastAccelStepper drives STEP and DIR; the core sets a speed
  on each axis every 50 ms. Below 0.07 steps/s (FastAccelStepper's slowest
  speed is about 0.064) an axis stops, and the core's position correction moves
  it a step at a time. The step counter is FastAccelStepper's plus an offset, so
  a sync never touches FastAccelStepper's own counter while the axis moves.
- **E-stop.** EN_OUT at the strongest drive, DRV_EN read with the internal
  pull-up. Pressed means DRV_EN high on two reads in a row while EN_OUT is low.
- **Hand unit.** The joystick's centre is read at power-on (leave it alone).
  X and Y have the internal pull-downs on: without the hand unit, both read
  about 0 V and count as centred. To check on the board: with the hand unit
  unplugged, `status` must give `"hand": false`.
- **LEDs.** PWM, as set by the core.
- **Restarts.** Each boot logs why the board started, unless it was a
  power-on. After a brown-out, a crash or a watchdog reset, each WebSocket
  client also gets the reason as a `warn` log when it connects (the app shows
  it). After a crash the log gives the task, the program counter and the
  backtrace from the core dump; to find the code:
  `xtensa-esp32s3-elf-addr2line -pfiaC -e build/firmware/star-track.ino.elf 0x…`.

## Rules for `src/core/`

The same files build for the ESP32 (arduino-cli) and for the PC (the simulator's
Makefile), so the core:

- includes no Arduino header and calls no `millis()`, `delay()` or `String`;
- gets time only from `Clock`, and reaches the hardware only through the
  interfaces in [`platform.h`](src/core/platform.h);
- uses ArduinoJson 7, through [`json.h`](src/core/json.h) for the same settings
  on both builds (64-bit integers, doubles).

## Files

`src/core/`:

| File | Contents |
| --- | --- |
| `platform.h` | Clock, Axis, EStop, Inputs, Gps, Leds, Storage, Link. |
| `mount.h`, `mount.cpp` | Modes, the 20 Hz control loop, the commands and events. |
| `align.h`, `align.cpp` | Pointing model: a rotation (Horn's method) and an altitude index, from one star (azimuth offset and index, level base assumed) or two stars and more (best fit); sync. |
| `astro.h`, `astro.cpp` | Sidereal time, RA/Dec ↔ alt/az, refraction, vectors and matrices. |
| `nmea.h`, `nmea.cpp` | RMC and GGA sentences from the GPS. |
| `config.h`, `config.cpp` | The settings and their `get_config` / `set_config` keys. |

`src/esp32/`:

| File | Contents |
| --- | --- |
| `pins.h` | The GPIO numbers of [`docs/architecture.md`](../../docs/architecture.md#pins). |
| `hardware.h`, `hardware.cpp` | Clock, Axis, EStop, Inputs, Gps, Leds and Storage on the board. |
| `network.h`, `network.cpp` | Link: the access point, the web server and WebSocket, the serial console. |
| `ota.h`, `ota.cpp` | Updates over Wi-Fi, and the confirmation of a new firmware. |
| `diag.h`, `diag.cpp` | Why the board restarted, with the core dump's summary after a crash. |

## Control

Every 50 ms, each axis gets a speed: the target's predicted rate (from its
position now and one second later) plus a correction for the position error. The
correction is the smallest of the slew speed, the speed that can still brake
before the target, and `track.gain` × error. Gotos and tracking use the same law;
a goto ends when both axes are within 0.005° (18″).
