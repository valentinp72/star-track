// Board test for the star-track PCB: LEDs, E-stop, GPS, motors, hand unit.
// Serial console at 115200 baud on the ESP32 board's UART port. Pins as in
// docs/architecture.md.

const int PIN_AZ_DIR = 4, PIN_AZ_STEP = 5;
const int PIN_ALT_DIR = 10, PIN_ALT_STEP = 11;
const int PIN_EN_OUT = 9;   // LOW = drivers on, through the E-stop
const int PIN_DRV_EN = 18;  // reads the drivers' EN line
const int PIN_LED_RED = 12, PIN_LED_BLUE = 13;
const int PIN_GPS_TX = 16, PIN_GPS_RX = 17;  // ESP32 side
const int PIN_SPARE = 14;  // unconnected: UART TX while the GPS test only listens
const int PIN_JOY_X = 1, PIN_JOY_Y = 2, PIN_JOY_SW = 42;
const int PIN_BTN[3] = {41, 40, 39};

const long STEPS_PER_REV = 200L * 64;  // 1/64 microsteps (MS1 GND, MS2 3.3 V)

enum Result { R_NOT_RUN, R_PASS, R_FAIL, R_SKIPPED };
const char *RESULT_NAMES[] = {"not run", "PASS", "FAIL", "skipped"};
const char *TEST_NAMES[] = {"LEDs", "E-stop", "GPS", "Motors"};
Result results[4] = {R_NOT_RUN, R_NOT_RUN, R_NOT_RUN, R_NOT_RUN};

struct NmeaScan {
  long bytes = 0, good = 0, bad = 0;
  String rmc, gga;
};

// Serial input

void flushInput() {
  delay(20);
  while (Serial.available()) Serial.read();
}

// Waits for one key; returns 0 after timeoutMs (0 = wait forever).
char readKey(unsigned long timeoutMs = 0) {
  flushInput();
  unsigned long start = millis();
  while (!timeoutMs || millis() - start < timeoutMs) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c != '\r' && c != '\n') return tolower(c);
    }
    delay(5);
  }
  return 0;
}

// y = yes, n = no, s = skip
char ask(const char *question) {
  Serial.printf("%s [y/n/s] ", question);
  for (;;) {
    char c = readKey();
    if (c == 'y' || c == 'n' || c == 's') {
      Serial.println(c);
      return c;
    }
  }
}

// Drivers

void driversOff() { digitalWrite(PIN_EN_OUT, HIGH); }
void driversOn() { digitalWrite(PIN_EN_OUT, LOW); }

// Moves one axis by the given number of steps, with a short linear ramp.
void move(int stepPin, int dirPin, long steps) {
  digitalWrite(dirPin, steps > 0 ? HIGH : LOW);
  delayMicroseconds(5);
  steps = labs(steps);
  const long ramp = min(steps / 2, 800L);
  const float vMin = 400, vMax = 1600;  // steps/s; 1600 = 7.5 rpm
  for (long i = 0; i < steps; i++) {
    long fromEdge = min(i, steps - 1 - i);
    float v = fromEdge < ramp ? vMin + (vMax - vMin) * fromEdge / ramp : vMax;
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(3);
    digitalWrite(stepPin, LOW);
    delayMicroseconds((unsigned long)(1e6 / v) - 3);
    if (i % 256 == 0) yield();
  }
}

// 1. LEDs

Result testLeds() {
  Serial.println("\n--- LEDs ---");
  Serial.println("Red LED on for 2 s.");
  analogWrite(PIN_LED_RED, 255);
  delay(2000);
  analogWrite(PIN_LED_RED, 0);
  char red = ask("Did the red LED light?");

  Serial.println("Blue LED on for 2 s.");
  analogWrite(PIN_LED_BLUE, 255);
  delay(2000);
  analogWrite(PIN_LED_BLUE, 0);
  char blue = ask("Did the blue LED light?");

  Serial.println("Both LEDs fade up and down (PWM).");
  for (int rep = 0; rep < 2; rep++)
    for (int i = 0; i <= 510; i += 5) {
      int v = i <= 255 ? i : 510 - i;
      analogWrite(PIN_LED_RED, v);
      analogWrite(PIN_LED_BLUE, v);
      delay(8);
    }
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_BLUE, 0);
  char fade = ask("Did both fade smoothly?");

  if (red == 'n' || blue == 'n')
    Serial.println("  Dark LED: check its polarity (long leg in the square pad) and its "
                   "resistor (R7 red, R8 blue, under the ESP32 board).");
  if (red == 's' && blue == 's' && fade == 's') return R_SKIPPED;
  return (red == 'n' || blue == 'n' || fade == 'n') ? R_FAIL : R_PASS;
}

// 2. E-stop and driver enable

Result testEstop() {
  Serial.println("\n--- E-stop and driver enable ---");
  bool ok = true;

  // EN_OUT released, internal pull-up off: R3 alone must pull DRV_EN high.
  pinMode(PIN_EN_OUT, INPUT);
  pinMode(PIN_DRV_EN, INPUT);
  delay(5);
  int released = digitalRead(PIN_DRV_EN);
  Serial.printf("  EN_OUT released -> DRV_EN %s (expected HIGH)\n", released ? "HIGH" : "LOW");
  if (!released) {
    // The ESP32's weak internal pulls (about 45 kΩ) tell an open R3 from a line held low.
    pinMode(PIN_DRV_EN, INPUT_PULLDOWN);
    delay(5);
    int withPulldown = digitalRead(PIN_DRV_EN);
    pinMode(PIN_DRV_EN, INPUT_PULLUP);
    delay(5);
    int withPullup = digitalRead(PIN_DRV_EN);
    Serial.printf("  With internal pull-down: %s, with internal pull-up: %s\n",
                  withPulldown ? "HIGH" : "LOW", withPullup ? "HIGH" : "LOW");
    if (!withPulldown && withPullup) {
      Serial.println("  WARNING: DRV_EN floats, R3 is not connected. The ESP32's internal "
                     "pull-up replaces it while the firmware runs, not during a reset.");
    } else {
      Serial.println("  FAIL: something holds DRV_EN low: a short to GND, or the driver "
                     "modules' EN pins (retest without them).");
      ok = false;
    }
  }
  pinMode(PIN_DRV_EN, INPUT_PULLUP);

  pinMode(PIN_EN_OUT, OUTPUT);
  driversOff();
  delay(5);
  int high = digitalRead(PIN_DRV_EN);
  Serial.printf("  EN_OUT HIGH     -> DRV_EN %s (expected HIGH)\n", high ? "HIGH" : "LOW");
  if (!high) ok = false;

  driversOn();
  delay(5);
  int low = digitalRead(PIN_DRV_EN);
  driversOff();
  Serial.printf("  EN_OUT LOW      -> DRV_EN %s (expected LOW, E-stop released)\n",
                low ? "HIGH" : "LOW");
  if (low) {
    Serial.println("  FAIL: the contact at P7 is open. Either the E-stop is pressed or "
                   "not wired (bridge P7 with a wire for now), or its contact is NO, not NC.");
    return R_FAIL;
  }
  if (!ok) return R_FAIL;

  Serial.println("  Press the E-stop, then release it (10 s each, any key to skip).");
  driversOn();
  bool pressSeen = false, releaseSeen = false;
  unsigned long start = millis();
  flushInput();
  while (millis() - start < 10000 && !Serial.available())
    if (digitalRead(PIN_DRV_EN)) { pressSeen = true; break; }
  if (pressSeen) {
    Serial.println("  Pressed: DRV_EN HIGH, drivers off.");
    start = millis();
    while (millis() - start < 10000 && !Serial.available())
      if (!digitalRead(PIN_DRV_EN)) { releaseSeen = true; break; }
    if (releaseSeen) Serial.println("  Released: DRV_EN LOW.");
  }
  driversOff();
  if (Serial.available()) {
    flushInput();
    Serial.println("  Press check skipped.");
    return R_PASS;
  }
  if (!pressSeen) Serial.println("  No press seen: is P7 bridged instead of wired to the E-stop?");
  else if (!releaseSeen) Serial.println("  No release seen (twist or pull the button to release it).");
  return pressSeen && releaseSeen ? R_PASS : R_FAIL;
}

// 3. GPS

bool nmeaChecksumOk(const String &s) {
  int star = s.lastIndexOf('*');
  if (!s.startsWith("$") || star < 0 || star + 3 > (int)s.length()) return false;
  uint8_t sum = 0;
  for (int i = 1; i < star; i++) sum ^= s[i];
  return strtol(s.substring(star + 1, star + 3).c_str(), nullptr, 16) == sum;
}

String nmeaField(const String &s, int n) {
  int start = 0;
  for (int i = 0; i < n; i++) {
    start = s.indexOf(',', start) + 1;
    if (start == 0) return "";
  }
  int end = s.indexOf(',', start);
  int star = s.indexOf('*', start);
  if (end < 0 || (star >= 0 && star < end)) end = star;
  return s.substring(start, end < 0 ? s.length() : end);
}

// Listens on one pin for a few seconds and counts NMEA sentences. The UART's TX
// goes to a spare pin, so it never drives a GPS output wired to the other P4 pin.
NmeaScan listenGps(int rxPin, long baud, unsigned long ms) {
  NmeaScan r;
  String line;
  Serial1.begin(baud, SERIAL_8N1, rxPin, PIN_SPARE);
  unsigned long start = millis();
  while (millis() - start < ms) {
    while (Serial1.available()) {
      char c = Serial1.read();
      r.bytes++;
      if (c == '\n') {
        line.trim();
        if (nmeaChecksumOk(line)) {
          r.good++;
          if (line.substring(3, 6) == "RMC") r.rmc = line;
          if (line.substring(3, 6) == "GGA") r.gga = line;
        } else if (line.length()) {
          r.bad++;
        }
        line = "";
      } else if (line.length() < 120) {
        line += c;
      }
    }
    delay(2);
  }
  Serial1.end();
  return r;
}

Result testGps() {
  Serial.println("\n--- GPS (P4, 9600 baud) ---");
  NmeaScan r = listenGps(PIN_GPS_RX, 9600, 5000);
  Serial.printf("  %ld bytes, %ld valid sentences, %ld bad in 5 s\n", r.bytes, r.good, r.bad);

  if (r.good == 0) {
    // Look for the module elsewhere: swapped wires, other baud rates.
    Serial.println("  Searching: swapped TX/RX and other baud rates...");
    const long bauds[] = {9600, 4800, 19200, 38400, 57600, 115200};
    for (int swapped = 0; swapped < 2; swapped++)
      for (long baud : bauds) {
        if (!swapped && baud == 9600) continue;
        NmeaScan t = listenGps(swapped ? PIN_GPS_TX : PIN_GPS_RX, baud, 2500);
        Serial.printf("    %s, %6ld baud: %ld bytes, %ld valid\n",
                      swapped ? "swapped" : "normal ", baud, t.bytes, t.good);
        if (t.good > 0) {
          Serial.printf("  FAIL: the module answers %s at %ld baud.\n",
                        swapped ? "with TX and RX swapped" : "", baud);
          if (swapped)
            Serial.println("  Wire it straight: module TX to P4 \"TX\" (pin 3), "
                           "module RX to P4 \"RX\" (pin 2).");
          if (baud != 9600)
            Serial.println("  The module is set to another baud rate (the firmware "
                           "expects 9600, the NEO-6M default).");
          return R_FAIL;
        }
      }
    if (r.bytes == 0) {
      // Raw levels with a weak pull-down: an idle UART output from the module holds
      // the line HIGH, a wire that reaches nothing reads LOW.
      Serial.println("  Line levels for 1 s, internal pull-down on:");
      for (int pin : {PIN_GPS_RX, PIN_GPS_TX}) {
        pinMode(pin, INPUT_PULLDOWN);
        delay(5);
        long highs = 0, edges = 0, samples = 0;
        int last = digitalRead(pin);
        unsigned long start = millis();
        while (millis() - start < 1000) {
          int v = digitalRead(pin);
          highs += v;
          edges += v != last;
          last = v;
          samples++;
          delayMicroseconds(10);
        }
        pinMode(pin, INPUT);
        Serial.printf("    GPIO%d (P4 %s): HIGH %ld%% of the time, %ld edges\n", pin,
                      pin == PIN_GPS_RX ? "pin 3, TX" : "pin 2, RX",
                      highs * 100 / samples, edges);
      }
      Serial.println("  FAIL: nothing received. If P4 pin 3 reads LOW all the time, the "
                     "module's TX does not reach the board (wire, joints) or the module "
                     "is not running.");
    }
    else
      Serial.println("  FAIL: data but no valid NMEA at any baud rate: noise, or a bad joint.");
    return R_FAIL;
  }
  String rmc = r.rmc, gga = r.gga;
  if (rmc.length()) {
    Serial.println("  " + rmc);
    String t = nmeaField(rmc, 1), d = nmeaField(rmc, 9);
    if (t.length() >= 6) Serial.printf("  UTC time %s:%s:%s", t.substring(0, 2).c_str(),
                                       t.substring(2, 4).c_str(), t.substring(4, 6).c_str());
    if (d.length() == 6) Serial.printf(", date %s/%s/20%s", d.substring(0, 2).c_str(),
                                       d.substring(2, 4).c_str(), d.substring(4, 6).c_str());
    if (t.length() < 6) Serial.print("  No UTC time yet (the module needs one satellite)");
    Serial.println();
  }
  if (gga.length()) {
    String fix = nmeaField(gga, 6), sats = nmeaField(gga, 7);
    Serial.printf("  Fix: %s, satellites used: %s\n",
                  fix == "0" || fix == "" ? "none yet" : "yes", sats.length() ? sats.c_str() : "0");
    if (fix == "0" || fix == "")
      Serial.println("  (No fix is normal indoors; outdoors, a cold start takes 1-5 min.)");
  }
  return R_PASS;
}

// 4. Motors

Result testMotors() {
  Serial.println("\n--- Motors ---");
  Serial.println("  Needs 12 V on, both motors plugged in, and Vref set on each driver.");
  Serial.println("  Never plug or unplug a motor with 12 V on.");
  if (ask("Ready?") != 'y') return R_SKIPPED;

  driversOn();
  delay(5);
  if (digitalRead(PIN_DRV_EN)) {
    driversOff();
    Serial.println("  FAIL: DRV_EN is HIGH, the E-stop is pressed or P7 is open.");
    return R_FAIL;
  }
  delay(200);

  struct Axis { const char *name; int step, dir; } axes[] = {
      {"Azimuth (J4)", PIN_AZ_STEP, PIN_AZ_DIR},
      {"Altitude (J5)", PIN_ALT_STEP, PIN_ALT_DIR},
  };
  bool fail = false, allSkipped = true;
  for (auto &a : axes) {
    Serial.printf("  %s: a quarter turn one way, a pause, then back.\n", a.name);
    move(a.step, a.dir, STEPS_PER_REV / 4);
    delay(700);
    move(a.step, a.dir, -STEPS_PER_REV / 4);
    char c = ask("  Did it turn a quarter turn each way, smoothly?");
    if (c == 'n') {
      fail = true;
      Serial.println("    No motion: check the 12 V, Vref, and the motor's coil pairs.\n"
                     "    Buzzing or jerky: one coil wire swapped or open.\n"
                     "    Half or double the angle: MS1/MS2 not at 1/64 (MS1 GND, MS2 3.3 V).");
    }
    if (c != 's') allSkipped = false;
  }

  Serial.println("  Holding torque: the drivers stay on for 5 s; try turning a shaft by hand.");
  delay(5000);
  driversOff();
  char free = ask("  Drivers now off: do the shafts turn freely?");
  if (free == 'n') {
    fail = true;
    Serial.println("    The drivers ignore EN: check the EN pin of each driver socket.");
  }
  if (allSkipped && free == 's') return R_SKIPPED;
  return fail ? R_FAIL : R_PASS;
}

// 5. Hand unit (live readings, not in the summary)

void testHandUnit() {
  Serial.println("\n--- Hand unit (P3), any key to stop ---");
  Serial.println("  Without the hand unit: X and Y float, switches read 'up'.");
  flushInput();
  while (!Serial.available()) {
    Serial.printf("  X %4d  Y %4d  SW %s  B1 %s  B2 %s  B3 %s\n",
                  analogRead(PIN_JOY_X), analogRead(PIN_JOY_Y),
                  digitalRead(PIN_JOY_SW) ? "up  " : "DOWN",
                  digitalRead(PIN_BTN[0]) ? "up  " : "DOWN",
                  digitalRead(PIN_BTN[1]) ? "up  " : "DOWN",
                  digitalRead(PIN_BTN[2]) ? "up  " : "DOWN");
    delay(300);
  }
  flushInput();
}

// 6. E-stop live (not in the summary)

// Drivers on; the red LED shows DRV_EN HIGH (drivers off), the blue LED DRV_EN LOW.
// The voltage tells a clean level from a line pulled half-way.
void testEstopLive() {
  Serial.println("\n--- E-stop live, drivers on, any key to stop ---");
  Serial.println("  Red LED: DRV_EN HIGH, drivers off. Blue LED: DRV_EN LOW, drivers on.");
  pinMode(PIN_EN_OUT, OUTPUT);
  driversOn();
  flushInput();
  int last = -1;
  unsigned long lastPrint = 0;
  while (!Serial.available()) {
    int mv = analogReadMilliVolts(PIN_DRV_EN);
    pinMode(PIN_DRV_EN, INPUT_PULLUP);  // analogRead detaches the digital input
    delayMicroseconds(50);
    int level = digitalRead(PIN_DRV_EN);
    analogWrite(PIN_LED_RED, level ? 255 : 0);
    analogWrite(PIN_LED_BLUE, level ? 0 : 40);
    if (level != last || millis() - lastPrint > 1000) {
      Serial.printf("  DRV_EN %s, %4d mV\n", level ? "HIGH" : "LOW ", mv);
      last = level;
      lastPrint = millis();
    }
    delay(20);
  }
  driversOff();
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_BLUE, 0);
  pinMode(PIN_DRV_EN, INPUT_PULLUP);
  flushInput();
}

// Menu

void printSummary() {
  Serial.println("\n=== Summary ===");
  for (int i = 0; i < 4; i++) Serial.printf("  %-8s %s\n", TEST_NAMES[i], RESULT_NAMES[results[i]]);
}

void printMenu() {
  Serial.println("\n=== star-track board test ===");
  Serial.println("  1  LEDs");
  Serial.println("  2  E-stop and driver enable");
  Serial.println("  3  GPS");
  Serial.println("  4  Motors (needs 12 V)");
  Serial.println("  5  Hand unit, live readings");
  Serial.println("  6  E-stop live, on the LEDs");
  Serial.println("  a  Tests 1-4 in order, then a summary");
  Serial.print("> ");
}

void setup() {
  // Drivers off before anything else.
  pinMode(PIN_EN_OUT, OUTPUT);
  driversOff();
  // Strongest output: EN_OUT holds DRV_EN low against the pull-up (about 15 mA through 220 Ω).
  gpio_set_drive_capability((gpio_num_t)PIN_EN_OUT, GPIO_DRIVE_CAP_3);
  pinMode(PIN_DRV_EN, INPUT_PULLUP);  // backs up R3
  for (int p : {PIN_AZ_STEP, PIN_AZ_DIR, PIN_ALT_STEP, PIN_ALT_DIR}) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  analogWrite(PIN_LED_RED, 0);
  analogWrite(PIN_LED_BLUE, 0);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  for (int p : PIN_BTN) pinMode(p, INPUT_PULLUP);

  Serial.begin(115200);
  delay(500);
  printMenu();
}

void loop() {
  char c = readKey();
  Serial.println(c);
  switch (c) {
    case '1': results[0] = testLeds(); break;
    case '2': results[1] = testEstop(); break;
    case '3': results[2] = testGps(); break;
    case '4': results[3] = testMotors(); break;
    case '5': testHandUnit(); break;
    case '6': testEstopLive(); break;
    case 'a':
      results[0] = testLeds();
      results[1] = testEstop();
      results[2] = testGps();
      results[3] = results[1] == R_PASS ? testMotors() : R_SKIPPED;
      printSummary();
      break;
  }
  printMenu();
}
