# Board test

A serial menu that checks the soldered PCB, one function at a time. It is
an Arduino sketch, built with [arduino-cli](https://arduino.github.io/arduino-cli/)
and the ESP32 Arduino core 3.x, the same tools as the mount's firmware.

| Key | Test | What it checks |
| --- | --- | --- |
| 1 | LEDs | Red, blue, then both fading (PWM). Asks whether each lit. |
| 2 | E-stop | R3 pulls DRV_EN high; EN_OUT reaches DRV_EN through the E-stop; a press and a release are seen. |
| 3 | GPS | NMEA sentences at 9600 baud with valid checksums; prints the UTC time and the fix (a fix is not needed to pass). |
| 4 | Motors | Each axis a quarter turn and back (1/64 microsteps), holding torque, free shafts once the drivers are off. Needs 12 V. |
| 5 | Hand unit | Live joystick and button readings, for when the hand unit is wired. |
| 6 | E-stop live | Drivers on; red LED when DRV_EN is high (drivers off), blue when low; prints the level and its voltage. |
| a | 1–4 | In order, then a summary. The motor test runs only if the E-stop test passes. |

## Before powering

- Without the ESP32 board and the driver modules, measure with a multimeter:
  no short between +12 V, +5 V, +3.3 V and GND; then, with 12 V on, the DC-DC
  output at 5 V (P5 VOUT).
- E-stop not wired yet: bridge P7 with a wire, or tests 2 and 4 fail (the
  drivers stay off). The press check in test 2 can be skipped with any key.
- Set each driver's Vref before the motor test. Never plug or unplug a motor
  with 12 V on.

## Build and upload

One-time setup:

```sh
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

Connect the ESP32 board's **UART** USB port (not the one marked USB), then:

```sh
arduino-cli board list          # finds the port, for example /dev/ttyUSB0 or /dev/ttyACM0
arduino-cli compile --upload -p /dev/ttyUSB0 firmware/board-test
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Type a menu key, then Enter. On Linux, the user needs to be in the `dialout`
group to open the port.

USB and the 12 V supply can be connected at the same time on the PCB (D5).
On USB alone, the GPS gets no power: test 3 needs the 12 V supply.
