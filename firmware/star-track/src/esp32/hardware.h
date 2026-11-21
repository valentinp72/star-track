// The core's platform interfaces on the ESP32-S3 board: steppers through
// FastAccelStepper, the E-stop line, the hand unit, the GPS UART, the LEDs and
// NVS. Everything here runs in the Arduino loop task.
#pragma once

#include <FastAccelStepper.h>
#include <Preferences.h>

#include "../core/platform.h"

namespace esp32 {

class EspClock : public core::Clock {
 public:
  int64_t monoMs() override;
};

// The step counter is FastAccelStepper's plus an offset: setPosition() changes
// only the offset, so it is safe while the axis moves (FastAccelStepper's own
// setCurrentPosition() is meant for standstill).
class EspAxis : public core::Axis {
 public:
  // False if no step generator is left for this pin.
  bool begin(FastAccelStepperEngine &engine, int stepPin, int dirPin);
  int32_t position() override;
  void setPosition(int32_t steps) override;
  void runAt(float speed, float accel) override;
  void stop(float accel) override;
  void forceStop() override;
  float speed() override;

 private:
  void setAccel(float accel);
  FastAccelStepper *s_ = nullptr;
  int32_t offset_ = 0;
};

class EspEStop : public core::EStop {
 public:
  void begin();
  void enableDrivers(bool on) override;
  bool pressed() override;

 private:
  bool on_ = false;
  bool lastHigh_ = false;
};

// Joystick on ADC1 with the internal pull-downs on: without the hand unit, both
// axes read 0 V and count as centred. Full deflection down and left at once
// reads the same and is ignored too.
class EspInputs : public core::Inputs {
 public:
  void begin();  // takes the joystick's centre, so leave it alone at power-on
  float joyX() override { return axis(0); }
  float joyY() override { return axis(1); }
  bool joySwitch() override;
  bool button(int i) override;
  bool connected();  // the hand unit's joystick is plugged in

 private:
  float axis(int i);
  int read(int pin);
  int centre_[2] = {2048, 2048};
};

class EspGps : public core::Gps {
 public:
  void begin();
  int read() override;
};

class EspLeds : public core::Leds {
 public:
  void begin();
  void set(uint8_t red, uint8_t blue) override;

 private:
  int red_ = -1, blue_ = -1;
};

class EspStorage : public core::Storage {
 public:
  void begin();
  bool load(const char *key, void *data, size_t len) override;
  void save(const char *key, const void *data, size_t len) override;
  void erase(const char *key) override;

 private:
  Preferences prefs_;
};

}  // namespace esp32
