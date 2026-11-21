// Interfaces between the mount logic (core/) and the hardware. The ESP32 build
// implements them with the real peripherals (src/esp32/), the simulator with a
// model of the mount (firmware/sim/). The core uses nothing else: no Arduino.h,
// millis(), delay() or String; time only through Clock.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace core {

// Monotonic time since boot. The simulator can pause it or speed it up.
class Clock {
 public:
  virtual ~Clock() = default;
  virtual int64_t monoMs() = 0;
};

// One stepper axis, as FastAccelStepper drives it. Positions in microsteps,
// speeds in microsteps/s, accelerations in microsteps/s².
class Axis {
 public:
  virtual ~Axis() = default;
  virtual int32_t position() = 0;
  virtual void setPosition(int32_t steps) = 0;  // redefines the counter, no motion
  // Runs at `speed` (signed), reaching it with `accel`.
  virtual void runAt(float speed, float accel) = 0;
  virtual void stop(float accel) = 0;  // decelerates to a stop
  virtual void forceStop() = 0;        // stops at once
  virtual float speed() = 0;           // current speed, signed
};

// The E-stop contact between EN_OUT and the drivers' EN line.
class EStop {
 public:
  virtual ~EStop() = default;
  virtual void enableDrivers(bool on) = 0;  // drives EN_OUT low (on) or high
  // True while the drivers are off although the firmware enables them.
  virtual bool pressed() = 0;
};

// The hand unit.
class Inputs {
 public:
  virtual ~Inputs() = default;
  virtual float joyX() = 0;  // -1..1, raw (no dead zone)
  virtual float joyY() = 0;
  virtual bool joySwitch() = 0;
  virtual bool button(int i) = 0;  // i = 0..2
};

// The GPS module's UART: raw NMEA bytes.
class Gps {
 public:
  virtual ~Gps() = default;
  virtual int read() = 0;  // next byte, or -1
};

class Leds {
 public:
  virtual ~Leds() = default;
  virtual void set(uint8_t red, uint8_t blue) = 0;  // brightness 0..255
};

// Small persistent records (NVS on the ESP32).
class Storage {
 public:
  virtual ~Storage() = default;
  virtual bool load(const char *key, void *data, size_t len) = 0;  // false if absent or a different size
  virtual void save(const char *key, const void *data, size_t len) = 0;
  virtual void erase(const char *key) = 0;
};

// Sends one protocol message (a JSON object, without a newline) to a client.
class Link {
 public:
  virtual ~Link() = default;
  virtual void send(int client, const char *text) = 0;
};

struct Platform {
  Clock &clock;
  Axis &az;
  Axis &alt;
  EStop &estop;
  Inputs &inputs;
  Gps &gps;
  Leds &leds;
  Storage &storage;
  Link &link;
  const char *board;  // "esp32s3" or "sim"
};

}  // namespace core
