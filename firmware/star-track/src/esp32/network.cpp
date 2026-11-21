#include "network.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_random.h>

#include "../core/config.h"
#include "../core/json.h"
#include "diag.h"

namespace esp32 {

namespace {

constexpr int kMaxMessage = core::Mount::kMaxMessage;

// 12 letters and digits from the hardware random number generator (random
// while the radio is on), without 0/O and 1/l/I.
void makePassword(char *out) {
  static const char kChars[] = "ABCDEFGHJKMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  const uint32_t n = sizeof kChars - 1;
  for (int i = 0; i < 12; i++) {
    uint32_t r;
    do r = esp_random();
    while (r >= UINT32_MAX - UINT32_MAX % n);  // no modulo bias
    out[i] = kChars[r % n];
  }
  out[12] = 0;
}

// A message longer than the limit is replaced by this many 'x', which the
// mount answers with the too_long error.
char *tooLong() {
  char *t = (char *)malloc(kMaxMessage + 2);
  if (!t) return nullptr;
  memset(t, 'x', kMaxMessage + 1);
  t[kMaxMessage + 1] = 0;
  return t;
}

char *copy(const std::string &s) {
  if (s.size() > (size_t)kMaxMessage) return tooLong();
  char *t = (char *)malloc(s.size() + 1);
  if (t) memcpy(t, s.c_str(), s.size() + 1);
  return t;
}

}  // namespace

void Network::logLine(const char *level, const char *msg, char *out, size_t n) {
  JsonDocument doc;
  doc["ev"] = "log";
  doc["level"] = level;
  doc["msg"] = msg;
  serializeJson(doc, out, n);
}

void Network::consoleLog(const char *level, const char *msg) {
  char buf[480];
  logLine(level, msg, buf, sizeof buf);
  Serial.write(buf);
  Serial.write('\n');
}

// ---------------------------------------------------------------- Wi-Fi

void Network::beginWifi(core::Mount &mount) {
  WiFi.mode(WIFI_AP);  // the radio is on from here: esp_random() is truly random
  if (strlen(mount.config().wifiPassword) < 8) {
    char pw[13];
    makePassword(pw);
    mount.setWifiPassword(pw);
    consoleLog("info", "first boot: made a new Wi-Fi password");
  }
  const core::Config &cfg = mount.config();
  // WPA2 (a password is given), channel 1, SSID visible, at most 4 stations.
  bool ok = WiFi.softAP(cfg.wifiSsid, cfg.wifiPassword, 1, 0, kMaxWebSockets);
  char msg[160];
  wifiUp_ = ok;
  if (ok) {
    snprintf(msg, sizeof msg, "Wi-Fi network \"%s\", password \"%s\", app at http://%s/", cfg.wifiSsid,
             cfg.wifiPassword, WiFi.softAPIP().toString().c_str());
    consoleLog("info", msg);
  } else {
    consoleLog("error", "the Wi-Fi access point did not start");
  }
  if (MDNS.begin("star-track")) MDNS.addService("http", "tcp", 80);
}

// ---------------------------------------------------------------- web server

bool Network::beginServer() {
  queue_ = xQueueCreate(64, sizeof(Event));
  ws_.onEvent([this](AsyncWebSocket *, AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data,
                     size_t len) { onEvent(c, type, arg, data, len); });
  server_.addHandler(&ws_);

  // Each file of app/dist is stored gzipped (name.gz), and served as it is.
  bool fs = LittleFS.begin(false);
  if (fs) {
    // Vite's file names under assets/ change with their contents.
    server_.serveStatic("/assets/", LittleFS, "/assets/").setCacheControl("max-age=31536000, immutable");
    server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setCacheControl("no-cache");
  } else {
    consoleLog("error", "no LittleFS file system: the phone app is not served (see firmware/star-track/README.md)");
  }
  server_.onNotFound([fs](AsyncWebServerRequest *r) {
    r->send(404, "text/plain", fs ? "Not found" : "The phone app is not on the mount's flash.");
  });
  server_.begin();
  return fs;
}

void Network::push(Kind kind, int client, char *text) {
  Event e{kind, client, text};
  if (xQueueSend(queue_, &e, 0) != pdTRUE) free(text);
}

// Runs in AsyncTCP's task.
void Network::onEvent(AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  int id = (int)c->id();
  switch (type) {
    case WS_EVT_CONNECT:
      if (ws_.count() > kMaxWebSockets) {
        c->close(1008, "too many clients");
        return;
      }
      c->_tempObject = new std::string();
      push(Kind::Connect, id, nullptr);
      break;
    case WS_EVT_DISCONNECT:
      delete static_cast<std::string *>(c->_tempObject);
      c->_tempObject = nullptr;
      push(Kind::Disconnect, id, nullptr);
      break;
    case WS_EVT_DATA: {
      auto *buf = static_cast<std::string *>(c->_tempObject);
      auto *info = static_cast<AwsFrameInfo *>(arg);
      if (!buf || info->message_opcode != WS_TEXT) return;
      // A frame may come in several parts (`index` is the offset in the frame),
      // and a message in several frames (`num`).
      if (info->num == 0 && info->index == 0) buf->clear();
      if (buf->size() <= (size_t)kMaxMessage)
        buf->append((const char *)data, std::min(len, (size_t)kMaxMessage + 1 - buf->size()));
      if (info->final && info->index + len == info->len) {
        push(Kind::Message, id, copy(*buf));
        buf->clear();
      }
      break;
    }
    default:
      break;
  }
}

// ---------------------------------------------------------------- console

void Network::beginConsole() {
  Serial.setRxBufferSize(kMaxMessage + 256);
  Serial.setTxBufferSize(4096);
  Serial.begin(115200);
}

void Network::handle(core::Mount &mount, int client, const char *text, bool updating) {
  if (!updating) {
    mount.onMessage(client, text);
    return;
  }
  JsonDocument filter, cmd, rep;
  filter["id"] = true;
  if (deserializeJson(cmd, text, DeserializationOption::Filter(filter))) return;
  rep["re"] = cmd["id"];
  rep["ok"] = false;
  rep["err"] = "updating";
  rep["msg"] = "an update is being written; the mount restarts when it is done";
  char buf[200];
  serializeJson(rep, buf, sizeof buf);
  send(client, buf);
}

void Network::readConsole(core::Mount &mount, bool updating) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (line_.size() <= (size_t)kMaxMessage) line_ += c;
      continue;
    }
    char *t = copy(line_);
    if (t) {
      handle(mount, kConsole, t, updating);
      free(t);
    }
    line_.clear();
  }
}

// ---------------------------------------------------------------- both

void Network::poll(core::Mount &mount, bool updating) {
  readConsole(mount, updating);
  Event e;
  while (queue_ && xQueueReceive(queue_, &e, 0) == pdTRUE) {
    switch (e.kind) {
      case Kind::Connect:
        mount.onConnect(e.client, core::ClientKind::WebSocket);
        // Tells the app after an unexpected restart, which the user may have
        // seen only as a lost connection.
        if (resetWasFault()) {
          char msg[360];
          snprintf(msg, sizeof msg, "the mount %s, %lu s ago", resetReport(), (unsigned long)(millis() / 1000));
          char buf[440];
          logLine("warn", msg, buf, sizeof buf);
          send(e.client, buf);
        }
        break;
      case Kind::Disconnect:
        mount.onDisconnect(e.client);
        break;
      case Kind::Message:
        if (e.text) handle(mount, e.client, e.text, updating);
        free(e.text);
        break;
    }
  }
  uint32_t t = millis();
  if (t - lastCleanup_ > 1000) {
    lastCleanup_ = t;
    ws_.cleanupClients(kMaxWebSockets);
  }
}

void Network::send(int client, const char *text) {
  if (client == kConsole) {
    Serial.write(text);
    Serial.write('\n');
    return;
  }
  ws_.text((uint32_t)client, text);
}

}  // namespace esp32
