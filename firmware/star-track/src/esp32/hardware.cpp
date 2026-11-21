#include "hardware.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_timer.h>
#include <math.h>

#include "pins.h"

namespace esp32 {

// ---------------------------------------------------------------- clock

int64_t EspClock::monoMs() { return esp_timer_get_time() / 1000; }

// ---------------------------------------------------------------- axes

namespace {
// FastAccelStepper's slowest speed is about 250 million ticks (16 MHz) per
// step, 0.064 steps/s. Slower requests stop the axis; the core's position
// correction then moves it a step at a time.
constexpr uint32_t kMinSpeedMilliHz = 70;
}  // namespace

bool EspAxis::begin(FastAccelStepperEngine &engine, int stepPin, int dirPin) {
  s_ = engine.stepperConnectToPin(stepPin);
  if (!s_) return false;
  s_->setDirectionPin(dirPin);
  return true;
}

int32_t EspAxis::position() { return s_ ? s_->getCurrentPosition() + offset_ : offset_; }

void EspAxis::setPosition(int32_t steps) { offset_ = steps - (s_ ? s_->getCurrentPosition() : 0); }

void EspAxis::setAccel(float accel) {
  int32_t a = (int32_t)lroundf(accel);
  s_->setAcceleration(a < 1 ? 1 : a);
}

void EspAxis::runAt(float speed, float accel) {
  if (!s_) return;
  float mhz = fabsf(speed) * 1000.0f;
  if (mhz < kMinSpeedMilliHz) {
    stop(accel);
    return;
  }
  setAccel(accel);
  uint32_t max = s_->getMaxSpeedInMilliHz();
  s_->setSpeedInMilliHz(mhz > max ? max : (uint32_t)mhz);
  if (speed > 0)
    s_->runForward();
  else
    s_->runBackward();
}

void EspAxis::stop(float accel) {
  if (!s_ || !s_->isRunning()) return;
  setAccel(accel);
  s_->applySpeedAcceleration();  // stopMove() uses the last applied acceleration
  s_->stopMove();
}

void EspAxis::forceStop() {
  if (s_) s_->forceStop();
}

// The ramp generator's speed, which is smooth; the queue's can jump.
float EspAxis::speed() { return s_ ? s_->getCurrentSpeedInMilliHz(false) / 1000.0f : 0; }

// ---------------------------------------------------------------- E-stop

void EspEStop::begin() {
  pinMode(kPinEnOut, OUTPUT);
  digitalWrite(kPinEnOut, HIGH);  // drivers off
  // EN_OUT holds DRV_EN low against the 220 Ω pull-up: about 15 mA.
  gpio_set_drive_capability((gpio_num_t)kPinEnOut, GPIO_DRIVE_CAP_3);
  pinMode(kPinDrvEn, INPUT_PULLUP);  // backs up R3
}

void EspEStop::enableDrivers(bool on) {
  on_ = on;
  digitalWrite(kPinEnOut, on ? LOW : HIGH);
  // A first read once the line has settled, so the next pressed() can already
  // report an E-stop held down at power-on.
  delayMicroseconds(50);
  lastHigh_ = on && digitalRead(kPinDrvEn);
}

// DRV_EN high while EN_OUT is low, on two reads in a row (one loop pass
// apart), so a single glitch does not stop the mount.
bool EspEStop::pressed() {
  if (!on_) return false;
  bool high = digitalRead(kPinDrvEn);
  bool r = high && lastHigh_;
  lastHigh_ = high;
  return r;
}

// ---------------------------------------------------------------- hand unit

namespace {
constexpr int kAdcMax = 4095;
constexpr int kAbsent = 120;  // both axes below this (about 0.1 V): no hand unit
}  // namespace

int EspInputs::read(int pin) {
  int sum = 0;
  for (int i = 0; i < 4; i++) sum += analogRead(pin);
  return sum / 4;
}

void EspInputs::begin() {
  pinMode(kPinJoySw, INPUT_PULLUP);
  for (int p : kPinButton) pinMode(p, INPUT_PULLUP);
  const int pins[2] = {kPinJoyX, kPinJoyY};
  for (int i = 0; i < 2; i++) {
    analogRead(pins[i]);  // sets the pin up as an ADC input, which turns its pulls off
    rtc_gpio_pullup_dis((gpio_num_t)pins[i]);
    rtc_gpio_pulldown_en((gpio_num_t)pins[i]);
  }
  delay(5);
  if (!connected()) return;
  for (int i = 0; i < 2; i++) {
    int c = read(pins[i]);
    // A joystick held off centre at power-on: keep the nominal centre.
    if (c > kAdcMax / 4 && c < kAdcMax * 3 / 4) centre_[i] = c;
  }
}

bool EspInputs::connected() { return read(kPinJoyX) >= kAbsent || read(kPinJoyY) >= kAbsent; }

float EspInputs::axis(int i) {
  int x = read(i == 0 ? kPinJoyX : kPinJoyY);
  int y = read(i == 0 ? kPinJoyY : kPinJoyX);
  if (x < kAbsent && y < kAbsent) return 0;
  int c = centre_[i];
  float v = x >= c ? (float)(x - c) / (kAdcMax - c) : (float)(x - c) / c;
  return v > 1 ? 1 : (v < -1 ? -1 : v);
}

bool EspInputs::joySwitch() { return !digitalRead(kPinJoySw); }

bool EspInputs::button(int i) { return i >= 0 && i < 3 && !digitalRead(kPinButton[i]); }

// ---------------------------------------------------------------- GPS

void EspGps::begin() { Serial1.begin(9600, SERIAL_8N1, kPinGpsRx, kPinGpsTx); }

int EspGps::read() { return Serial1.available() ? Serial1.read() : -1; }

// ---------------------------------------------------------------- LEDs

void EspLeds::begin() { set(0, 0); }

void EspLeds::set(uint8_t red, uint8_t blue) {
  if (red != red_) analogWrite(kPinLedRed, red);
  if (blue != blue_) analogWrite(kPinLedBlue, blue);
  red_ = red;
  blue_ = blue;
}

// ---------------------------------------------------------------- NVS

void EspStorage::begin() { prefs_.begin("star-track", false); }

bool EspStorage::load(const char *key, void *data, size_t len) {
  if (prefs_.getBytesLength(key) != len) return false;
  return prefs_.getBytes(key, data, len) == len;
}

void EspStorage::save(const char *key, const void *data, size_t len) { prefs_.putBytes(key, data, len); }

void EspStorage::erase(const char *key) { prefs_.remove(key); }

}  // namespace esp32
