#include "ota.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>

#include <algorithm>
#include <esp_ota_ops.h>

#include "network.h"

namespace esp32 {

namespace {

constexpr uint8_t kImageMagic = 0xE9;  // first byte of an ESP32 firmware image

// The request's own error, freed with the request.
void setError(AsyncWebServerRequest *r, const char *msg) {
  if (!r->_tempObject) r->_tempObject = strdup(msg);
}

bool onTrial() {
  esp_ota_img_states_t s;
  return esp_ota_get_state_partition(esp_ota_get_running_partition(), &s) == ESP_OK &&
         s == ESP_OTA_IMG_PENDING_VERIFY;
}

}  // namespace

void Ota::begin(AsyncWebServer &server) {
  server.on(
      "/update", HTTP_POST, [this](AsyncWebServerRequest *r) { reply(r); },
      [this](AsyncWebServerRequest *r, const String &, size_t index, uint8_t *data, size_t len, bool final) {
        upload(r, index, data, len, final);
      });
  char msg[120];
  snprintf(msg, sizeof msg, "firmware %s in %s%s", core::Mount::kFirmwareVersion,
           esp_ota_get_running_partition()->label, onTrial() ? ", on trial until confirmed" : "");
  Network::consoleLog("info", msg);
}

// ---------------------------------------------------------------- AsyncTCP's task

void Ota::start(AsyncWebServerRequest *r, uint8_t firstByte) {
  if (state_ != Idle) {
    setError(r, "another update is running");
    return;
  }
  bool app = r->hasParam("target") && r->getParam("target")->value() == "app";
  if (app && firstByte != 'S') {
    setError(r, firstByte == kImageMagic ? "this is a firmware image: send it without ?target=app"
                                         : "not an app image from app-fs.sh (build/app-fs.ota)");
    return;
  }
  if (!app && firstByte != kImageMagic) {
    setError(r, "not a firmware image (for the app's LittleFS image, add ?target=app)");
    return;
  }
  owner_ = r;
  app_ = app;
  error_[0] = 0;

  // The loop task checks that the mount is still, and turns the drivers off.
  state_ = Requested;
  for (int i = 0; i < 1000 && state_ == Requested; i++) delay(1);
  int expected = Requested;
  if (state_.compare_exchange_strong(expected, Idle) || state_ == Refused) {
    state_ = Idle;
    owner_ = nullptr;
    setError(r, "the mount is moving: stop it (or park it) first");
    return;
  }

  if (app_) {
    LittleFS.end();
    if (!beginApp()) {
      setError(r, "no memory or no spiffs partition");
      fail("no memory or no spiffs partition");
      return;
    }
  } else if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
    setError(r, Update.errorString());
    fail(Update.errorString());
    return;
  }
  r->onDisconnect([this, r]() {
    if (owner_ == r && state_ == Writing) {
      if (!app_) Update.abort();
      fail("the upload was interrupted");
    }
  });
  state_ = Writing;
}

void Ota::upload(AsyncWebServerRequest *r, size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) start(r, len ? data[0] : 0);
  if (owner_ != r || state_ != Writing) return;
  const char *err = nullptr;
  if (app_) {
    if (len && !writeApp(data, len, err)) {
      setError(r, err);
      fail(err);
      return;
    }
    if (!final) return;
    if (endApp(err)) {
      state_ = Done;
    } else {
      setError(r, err);
      fail(err);
    }
    return;
  }
  if (len && Update.write(data, len) != len) {
    setError(r, Update.errorString());
    Update.abort();
    fail(Update.errorString());
    return;
  }
  if (!final) return;
  if (Update.end(true)) {
    state_ = Done;
  } else {
    setError(r, Update.errorString());
    fail(Update.errorString());
  }
}

// ---------------------------------------------------------------- the app's image

namespace {
constexpr size_t kHeader = 12;  // "STFS", version, number of blocks
uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
}  // namespace

bool Ota::beginApp() {
  fs_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
  if (!buf_) buf_ = (uint8_t *)malloc(4 + kBlock);
  have_ = 0;
  header_ = false;
  blocks_ = written_ = 0;
  return fs_ && buf_;
}

bool Ota::writeApp(const uint8_t *data, size_t len, const char *&err) {
  while (len) {
    size_t need = header_ ? 4 + kBlock : kHeader;
    size_t n = std::min(len, need - have_);
    memcpy(buf_ + have_, data, n);
    have_ += n;
    data += n;
    len -= n;
    if (have_ < need) break;
    have_ = 0;
    if (!header_) {
      if (memcmp(buf_, "STFS", 4) || le32(buf_ + 4) != 1) {
        err = "not an app image from app-fs.sh (build/app-fs.ota)";
        return false;
      }
      blocks_ = le32(buf_ + 8);
      header_ = true;
      continue;
    }
    uint32_t block = le32(buf_);
    if (written_ >= blocks_ || (block + 1) * kBlock > fs_->size) {
      err = "the image does not fit the spiffs partition";
      return false;
    }
    if (esp_partition_erase_range(fs_, block * kBlock, kBlock) != ESP_OK ||
        esp_partition_write(fs_, block * kBlock, buf_ + 4, kBlock) != ESP_OK) {
      err = "flash write error";
      return false;
    }
    written_++;
  }
  return true;
}

bool Ota::endApp(const char *&err) {
  if (!header_ || have_ || written_ != blocks_) {
    err = "the image is incomplete";
    return false;
  }
  return true;
}

void Ota::fail(const char *msg) {
  snprintf(error_, sizeof error_, "update failed: %s", msg);
  if (app_) LittleFS.begin(false);
  state_ = Failed;
}

void Ota::reply(AsyncWebServerRequest *r) {
  if (r->_tempObject) {
    String msg = static_cast<const char *>(r->_tempObject);
    r->send(409, "text/plain", msg + "\n");
  } else if (owner_ == r && state_ == Done) {
    r->send(200, "text/plain", app_ ? "app updated: restarting\n" : "firmware written: restarting on trial\n");
  } else {
    r->send(400, "text/plain", "no file in the request: send it as multipart form data\n");
  }
}

// ---------------------------------------------------------------- loop task

void Ota::poll(const core::Mount &mount, bool wifiUp) {
  switch (state_.load()) {
    case Requested: {
      core::Mode m = mount.mode();
      bool still = (m == core::Mode::Idle || m == core::Mode::Parked || m == core::Mode::EStop) &&
                   az_.speed() == 0 && alt_.speed() == 0;
      int expected = Requested;
      if (!state_.compare_exchange_strong(expected, still ? Accepted : Refused)) break;
      if (still) {
        driversWereOn_ = estop_.enabled();
        estop_.enableDrivers(false);
        Network::consoleLog("info", app_ ? "updating the app: drivers off" : "updating the firmware: drivers off");
      }
      break;
    }
    case Done:
      if (!doneAt_) {
        doneAt_ = millis() | 1;
        Network::consoleLog("info", "update written: restarting");
      } else if (millis() - doneAt_ > 1000) {  // time for the HTTP reply
        ESP.restart();
      }
      break;
    case Failed:
      Network::consoleLog("error", error_);
      estop_.enableDrivers(driversWereOn_);
      owner_ = nullptr;
      state_ = Idle;
      break;
    default:
      break;
  }

  if (!confirmed_ && wifiUp && millis() >= kConfirmMs) {
    confirmed_ = true;
    if (onTrial()) {
      esp_ota_mark_app_valid_cancel_rollback();
      Network::consoleLog("info", "new firmware confirmed: it stays after a restart");
    }
  }
}

}  // namespace esp32

// The Arduino core would confirm a new firmware as soon as it starts; Ota::poll
// does it later.
extern "C" bool verifyRollbackLater() { return true; }
