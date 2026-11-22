// The mount's firmware for the ESP32-S3: the core (src/core/, the same code as
// in the simulator) on the board's hardware (src/esp32/). See README.md.

#include <WiFi.h>
#include <esp_heap_caps.h>

#include "src/core/mount.h"
#include "src/esp32/diag.h"
#include "src/esp32/hardware.h"
#include "src/esp32/network.h"
#include "src/esp32/ota.h"
#include "src/esp32/pins.h"

using namespace esp32;

static FastAccelStepperEngine engine;
static EspClock clock_;
static EspAxis az, alt;
static EspEStop estop;
static EspInputs inputs;
static EspGps gps;
static EspLeds leds;
static EspStorage storage;
static esp32::Network net;
static Ota ota(estop, az, alt);
static core::Platform platform{clock_, az, alt, estop, inputs, gps, leds, storage, net, "esp32s3"};
static core::Mount mount(platform);

// The status's `sys`: internal RAM free now, its lowest since boot, its largest
// free block (bytes), PSRAM free, WebSocket clients, stations on the access point.
static void sysStatus(JsonObject o) {
  o["heap"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  o["heap_min"] = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
  o["heap_block"] = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  o["psram"] = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  o["ws"] = net.webSockets();
  o["ws_refused"] = net.refused();
  o["stations"] = WiFi.softAPgetStationNum();
}

void setup() {
  estop.begin();  // drivers off before anything else
  for (int p : {kPinAzStep, kPinAzDir, kPinAltStep, kPinAltDir}) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  leds.begin();
  net.beginConsole();
  readResetReason();
  if (*resetReport()) esp32::Network::consoleLog(resetWasFault() ? "warn" : "info", resetReport());
  platform.reset = resetCode();
  platform.resetMsg = resetReport();
  platform.sys = sysStatus;
  storage.begin();
  inputs.begin();
  gps.begin();

  char hand[100];
  inputs.describe(hand, sizeof hand);
  esp32::Network::consoleLog("info", hand);

  engine.init();
  if (!az.begin(engine, kPinAzStep, kPinAzDir) || !alt.begin(engine, kPinAltStep, kPinAltDir))
    esp32::Network::consoleLog("error", "no step generator for an axis: the motors will not move");

  mount.onConnect(esp32::Network::kConsole, core::ClientKind::Console);
  mount.begin();

  net.beginWifi(mount);
  ota.begin(net.server());  // before the app's files, which would take /update
  net.beginServer();
}

void loop() {
  net.poll(mount, ota.active());
  ota.poll(mount, net.wifiUp());
  if (!ota.active()) mount.loop();
  delay(1);  // lets the idle task run (watchdog), and AsyncTCP on this core
}
