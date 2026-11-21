// Updates over Wi-Fi: POST /update with the firmware (written to the other app
// slot) or ?target=app with the phone app's LittleFS image, in app-fs.sh's
// sparse format (the blocks in use, each after its number). The upload runs in
// AsyncTCP's task; the loop task decides whether the mount may stop for it,
// turns the drivers off while the flash is written, and restarts at the end.
//
// A new firmware runs on trial: it confirms itself once the access point has
// been up for kConfirmMs. If it crashes or is reset before, the bootloader
// starts the previous one again.
#pragma once

#include <ESPAsyncWebServer.h>
#include <esp_partition.h>

#include <atomic>

#include "../core/mount.h"
#include "hardware.h"

namespace esp32 {

class Ota {
 public:
  static constexpr uint32_t kConfirmMs = 30000;

  Ota(EspEStop &estop, EspAxis &az, EspAxis &alt) : estop_(estop), az_(az), alt_(alt) {}
  void begin(AsyncWebServer &server);
  // In the loop task: answers a new upload, finishes one, confirms a new firmware.
  void poll(const core::Mount &mount, bool wifiUp);
  // From the upload's acceptance to the restart: the mount does not run.
  bool active() const { return state_ >= Accepted; }

 private:
  enum State : int { Idle, Requested, Refused, Accepted, Writing, Done, Failed };
  void upload(AsyncWebServerRequest *r, size_t index, uint8_t *data, size_t len, bool final);
  void start(AsyncWebServerRequest *r, uint8_t firstByte);
  void fail(const char *msg);
  void reply(AsyncWebServerRequest *r);
  // The app's image: false and an error on a bad stream or a flash error.
  bool beginApp();
  bool writeApp(const uint8_t *data, size_t len, const char *&err);
  bool endApp(const char *&err);

  EspEStop &estop_;
  EspAxis &az_, &alt_;
  std::atomic<int> state_{Idle};
  bool app_ = false;         // the app's image, not the firmware
  bool driversWereOn_ = false;
  AsyncWebServerRequest *owner_ = nullptr;
  char error_[120] = "";
  uint32_t doneAt_ = 0;
  bool confirmed_ = false;

  // The app's image being written.
  static constexpr size_t kBlock = 4096;
  const esp_partition_t *fs_ = nullptr;
  uint8_t *buf_ = nullptr;  // the header, then one record: block number and data
  size_t have_ = 0;
  bool header_ = false;
  uint32_t blocks_ = 0, written_ = 0;
};

}  // namespace esp32
