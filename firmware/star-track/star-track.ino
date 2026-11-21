// The mount's firmware for the ESP32-S3: the core (src/core/, the same code as
// in the simulator) on the board's hardware (src/esp32/). See README.md.

#include "src/core/mount.h"
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
static Network net;
static Ota ota(estop, az, alt);
static core::Platform platform{clock_, az, alt, estop, inputs, gps, leds, storage, net, "esp32s3"};
static core::Mount mount(platform);

void setup() {
  estop.begin();  // drivers off before anything else
  for (int p : {kPinAzStep, kPinAzDir, kPinAltStep, kPinAltDir}) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  leds.begin();
  net.beginConsole();
  storage.begin();
  inputs.begin();
  gps.begin();

  char hand[100];
  inputs.describe(hand, sizeof hand);
  Network::consoleLog("info", hand);

  engine.init();
  if (!az.begin(engine, kPinAzStep, kPinAzDir) || !alt.begin(engine, kPinAltStep, kPinAltDir))
    Network::consoleLog("error", "no step generator for an axis: the motors will not move");

  mount.onConnect(Network::kConsole, core::ClientKind::Console);
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
