# star-track

Motorised alt-azimuth mount for a 114/900 Newtonian reflector (Omegon 114/900),
controlled from a phone, with a joystick, buttons and an emergency stop for manual use.

This branch restarts the project. It currently holds design notes and an interactive
3D model of the mount; no firmware yet.

## Contents

| Path | What it is |
| --- | --- |
| [`docs/architecture.md`](docs/architecture.md) | Planned electronics and software: ESP32-S3 controller, phone web app, firmware structure, safety. |
| [`docs/mechanics.md`](docs/mechanics.md) | Current mechanical build (dimensions from the CAD model), bearing redesign, base stability, transport. |
| [`docs/3d/index.html`](docs/3d/index.html) | Interactive 3D model of the redesigned mount. Open it in a browser (needs an internet connection to load three.js from a CDN). |

## Earlier attempts

| Branch | Period | Approach |
| --- | --- | --- |
| `v1` | 2019–2020 | Single-board computer generating step pulses itself (wiringPi, C and Python), emergency button as a systemd service. |
| `v2` | 2023 | Orange Pi "brain" (Flask REST API + Skyfield) with an Ionic/Vue client packaged for iOS with Capacitor; planned Arduino "muscle" for the motors. |
| `v3` | 2025 | ESP-IDF Wi-Fi access point example on an ESP32 (previously `main`). |
