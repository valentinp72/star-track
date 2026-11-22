// The protocol's transports on the ESP32: the Wi-Fi access point, the web
// server (the phone app from LittleFS, the WebSocket at /ws) and the serial
// console. The web server runs in AsyncTCP's task; its events wait in a queue
// until poll() hands them to the mount, in the Arduino loop task.
#pragma once

#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <string>

#include "../core/mount.h"
#include "../core/platform.h"

namespace esp32 {

class Network : public core::Link {
 public:
  static constexpr int kConsole = 0;  // client id of the serial console
  static constexpr int kMaxWebSockets = 4;

  Network() : server_(80), ws_("/ws") {}

  // Starts the access point with the SSID and password of the settings, making
  // and saving a password at the first boot. Prints both on the console.
  void beginWifi(core::Mount &mount);
  // Mounts LittleFS and starts the web server. False if LittleFS is missing
  // (the WebSocket still works; the app is not served).
  bool beginServer();
  void beginConsole();

  // Hands the queued WebSocket events and the console's lines to the mount.
  // While `updating`, commands get the error "updating" instead.
  void poll(core::Mount &mount, bool updating);
  bool wifiUp() const { return wifiUp_; }
  size_t webSockets() const { return ws_.count(); }
  uint32_t refused() const { return refused_; }  // WebSocket clients refused (more than 4)
  AsyncWebServer &server() { return server_; }
  void send(int client, const char *text) override;

  // A JSON log line on the console, before the mount runs.
  static void consoleLog(const char *level, const char *msg);
  static void logLine(const char *level, const char *msg, char *out, size_t n);  // a log event's JSON

 private:
  enum class Kind : uint8_t { Connect, Message, Disconnect };
  struct Event {
    Kind kind;
    int client;
    char *text;  // Message only; malloc'd, freed by poll()
  };
  void onEvent(AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data, size_t len);
  void push(Kind kind, int client, char *text);
  void readConsole(core::Mount &mount, bool updating);
  void handle(core::Mount &mount, int client, const char *text, bool updating);

  AsyncWebServer server_;
  AsyncWebSocket ws_;
  QueueHandle_t queue_ = nullptr;
  std::string line_;
  uint32_t lastCleanup_ = 0;
  bool wifiUp_ = false;
  uint32_t refused_ = 0;
};

}  // namespace esp32
