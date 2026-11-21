// GPIO numbers, as in docs/architecture.md ("Pins") and firmware/board-test.
#pragma once

namespace esp32 {

constexpr int kPinAzDir = 4, kPinAzStep = 5;
constexpr int kPinAltDir = 10, kPinAltStep = 11;
constexpr int kPinEnOut = 9;   // LOW = drivers on, through the E-stop
constexpr int kPinDrvEn = 18;  // reads the drivers' EN line
constexpr int kPinLedRed = 12, kPinLedBlue = 13;
constexpr int kPinGpsTx = 16, kPinGpsRx = 17;  // ESP32 side
constexpr int kPinJoyX = 1, kPinJoyY = 2, kPinJoySw = 42;
constexpr int kPinButton[3] = {41, 40, 39};

}  // namespace esp32
