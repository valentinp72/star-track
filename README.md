# star-track

Motorised alt-azimuth mount for a 114/900 Newtonian reflector (Omegon 114/900),
controlled from a phone, with a joystick, buttons and an emergency stop for manual use.

This branch restarts the project. It currently holds design notes, an interactive 3D
model of the mount, the electronics schematic and PCB, a board test sketch, and the
mount firmware's core with a simulator that runs it on a PC (the ESP32 layer is not
written yet), and the start of the phone app.

## Contents

| Path | What it is |
| --- | --- |
| [`docs/architecture.md`](docs/architecture.md) | Planned electronics and software: ESP32-S3 controller, battery power, pins, phone web app, firmware structure, safety. |
| [`docs/protocol.md`](docs/protocol.md) | JSON protocol between the mount and the phone app (WebSocket and serial), shared with the simulator. |
| [`docs/mechanics.md`](docs/mechanics.md) | Current mechanical build (dimensions from the CAD model), bearing redesign, battery placement, base stability, transport. |
| [`docs/3d/index.html`](docs/3d/index.html) | Interactive 3D model of the redesigned mount. Open it in a browser (needs an internet connection to load three.js from a CDN). |
| [`electronics/`](electronics/) | KiCad schematic: power input and battery, ESP32-S3 board, two TMC2209 drivers, GPS, E-stop, LEDs, hand unit (joystick and buttons). [PDF](electronics/star-track.pdf) and PNG exports alongside. Two-layer PCB for the board on the mount (through-hole, plus four SMD parts for hand soldering), and a stripboard alternative (24 × 56 holes) with pictures of both sides. |
| [`firmware/board-test/`](firmware/board-test/) | Arduino sketch (arduino-cli) that checks the soldered board from a serial menu: LEDs, E-stop, GPS, motors, hand unit. |
| [`firmware/star-track/`](firmware/star-track/) | Mount firmware: the hardware-independent core (protocol, modes, tracking, pointing model). The ESP32 platform layer is still to come. |
| [`firmware/sim/`](firmware/sim/) | Simulator: builds the firmware's core for the PC with a model of the mount, speaks the protocol on stdin/stdout and a WebSocket, serves the phone app and a simulator page; `make test` runs unit tests and a scripted night. |
| [`app/`](app/) | Phone web app (Svelte, Konsta UI), served by the mount over its Wi-Fi access point and added to the iPhone's home screen. In progress: connection and status so far. |

## Earlier attempts

| Branch | Period | Approach |
| --- | --- | --- |
| `v1` | 2019–2020 | Single-board computer generating step pulses itself (wiringPi, C and Python), emergency button as a systemd service. |
| `v2` | 2023 | Orange Pi "brain" (Flask REST API + Skyfield) with an Ionic/Vue client packaged for iOS with Capacitor; planned Arduino "muscle" for the motors. |
| `v3` | 2025 | ESP-IDF Wi-Fi access point example on an ESP32 (previously `main`). |
