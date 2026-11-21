// Simulated mount: implements the core's platform interfaces with a model of
// the hardware, and handles the sim_* commands.
#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "astro.h"
#include "json.h"
#include "mount.h"
#include "platform.h"

namespace sim {

class SimClock : public core::Clock {
 public:
  int64_t monoMs() override { return ms; }
  int64_t ms = 0;
};

// A stepper and its driver. `pos` is the firmware's step counter; the
// mechanics follow it while the drivers are on, and `mechOffset` holds the
// difference (steps lost with the drivers off, slips, setPosition).
class SimAxis : public core::Axis {
 public:
  int32_t position() override;
  void setPosition(int32_t steps) override;
  void runAt(float speed, float accel) override;
  void stop(float accel) override;
  void forceStop() override;
  float speed() override { return (float)v; }

  void advance(double dt, bool driversOn);
  double mechSteps() const { return pos + mechOffset; }

  double pos = 0, v = 0, mechOffset = 0;
  double targetV = 0, accel = 1000;
};

class SimEStop : public core::EStop {
 public:
  void enableDrivers(bool on) override { enabled = on; }
  bool pressed() override { return isPressed; }
  bool driversOn() const { return enabled && !isPressed; }
  bool enabled = false, isPressed = false;
};

class SimInputs : public core::Inputs {
 public:
  float joyX() override { return x; }
  float joyY() override { return y; }
  bool joySwitch() override { return sw; }
  bool button(int i) override { return i >= 0 && i < 3 && buttons[i]; }
  float x = 0, y = 0;
  bool sw = false, buttons[3] = {false, false, false};
};

// Sends RMC and GGA sentences once per simulated second while enabled.
class SimGps : public core::Gps {
 public:
  int read() override;
  void tick(int64_t utcMs);
  bool enabled = false, fix = true;
  double lat = 45, lon = 5, elev = 200;
  int sats = 8;

 private:
  std::deque<char> queue_;
  int64_t lastSecond_ = -1;
};

class SimLeds : public core::Leds {
 public:
  void set(uint8_t r, uint8_t b) override {
    red = r;
    blue = b;
  }
  uint8_t red = 0, blue = 0;
};

// In memory, written to a file when one is given.
class SimStorage : public core::Storage {
 public:
  explicit SimStorage(std::string path = "");
  bool load(const char *key, void *data, size_t len) override;
  size_t size(const char *key) override;
  void save(const char *key, const void *data, size_t len) override;
  void erase(const char *key) override;

 private:
  void write();
  std::string path_;
  std::map<std::string, std::vector<uint8_t>> records_;
};

class Server;

// Messages from the core: client 0 is the console (stdout), the others are
// WebSocket clients of the server.
class SimLink : public core::Link {
 public:
  void send(int client, const char *text) override;
  Server *server = nullptr;
};

// The whole simulated mount.
class Sim {
 public:
  static constexpr int kStepMs = 5;  // physics and loop() period
  static constexpr int kConsole = 0;  // client id of stdin/stdout

  explicit Sim(const std::string &storagePath);
  void begin(int64_t utcMs);
  void advance(int64_t ms);  // runs the mount for `ms` of simulated time
  void handleLine(int client, const char *line);
  void connect(int client);  // a WebSocket client
  void disconnect(int client);
  void setServer(Server *s) { link_.server = s; }

  double rate = 1;  // sim clock speed for the real-time loop
  double slewRateMax = 10;  // clock speed limit during slews, so they can be watched (0: none)
  // The clock speed the real-time loop should use now.
  double effectiveRate() {
    core::Mode m = mount_.mode();
    bool slewing = m == core::Mode::Goto || m == core::Mode::Manual;
    return slewing && slewRateMax > 0 && rate > slewRateMax ? slewRateMax : rate;
  }

 private:
  bool simCommand(JsonObjectConst cmd, JsonObject reply, std::string &err, std::string &msg);
  void fillTruth(JsonObject r);
  core::Vec3 truthVector();
  void pointAt(const core::Vec3 &world);
  core::Mat3 mountToWorld();
  int64_t trueUtc() { return clock_.ms + utcOffset_; }

  SimClock clock_;
  SimAxis az_, alt_;
  SimEStop estop_;
  SimInputs inputs_;
  SimGps gps_;
  SimLeds leds_;
  SimStorage storage_;
  SimLink link_;
  core::Platform platform_;
  core::Mount mount_;

  int64_t utcOffset_ = 0;  // true UTC = clock + offset
  double stepsPerRev_ = 160000;
  double tiltDeg_ = 0, tiltAzDeg_ = 0, northDeg_ = 0, altZeroDeg_ = 0;
};

}  // namespace sim
