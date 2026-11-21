#include "sim.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <fstream>

using namespace core;

namespace sim {

// ---------------------------------------------------------------- axis

int32_t SimAxis::position() { return (int32_t)lround(pos); }

void SimAxis::setPosition(int32_t steps) {
  mechOffset += pos - steps;  // the mechanics do not move
  pos = steps;
}

void SimAxis::runAt(float speed, float a) {
  targetV = speed;
  accel = a > 0 ? a : 1;
}

void SimAxis::stop(float a) { runAt(0, a); }

void SimAxis::forceStop() { v = targetV = 0; }

void SimAxis::advance(double dt, bool driversOn) {
  double dv = targetV - v, maxDv = accel * dt;
  v += dv > maxDv ? maxDv : (dv < -maxDv ? -maxDv : dv);
  double step = v * dt;
  pos += step;
  if (!driversOn) mechOffset -= step;  // pulses with the drivers off move nothing
}

// ---------------------------------------------------------------- GPS

namespace {

void appendSentence(std::deque<char> &q, const char *body) {
  uint8_t sum = 0;
  for (const char *p = body; *p; p++) sum ^= (uint8_t)*p;
  char line[128];
  snprintf(line, sizeof line, "$%s*%02X\r\n", body, sum);
  for (const char *p = line; *p; p++) q.push_back(*p);
}

void nmeaCoord(double deg, bool lat, char *out, size_t n) {
  char hemi = lat ? (deg < 0 ? 'S' : 'N') : (deg < 0 ? 'W' : 'E');
  deg = fabs(deg);
  int d = (int)deg;
  double m = (deg - d) * 60;
  snprintf(out, n, lat ? "%02d%07.4f,%c" : "%03d%07.4f,%c", d, m, hemi);
}

}  // namespace

void SimGps::tick(int64_t utcMs) {
  if (!enabled) return;
  int64_t second = utcMs / 1000;
  if (second == lastSecond_) return;
  lastSecond_ = second;
  int64_t days = second / 86400, s = second % 86400;
  // Civil date from days since 1970 (Howard Hinnant's algorithm).
  int64_t z = days + 719468, era = z / 146097, doe = z - era * 146097;
  int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
  int d = (int)(doy - (153 * mp + 2) / 5 + 1), m = (int)(mp < 10 ? mp + 3 : mp - 9);
  int y = (int)(yoe + era * 400 + (m <= 2));
  char la[24], lo[24], body[100];
  nmeaCoord(lat, true, la, sizeof la);
  nmeaCoord(lon, false, lo, sizeof lo);
  int hh = (int)(s / 3600), mi = (int)(s / 60 % 60), ss = (int)(s % 60);
  snprintf(body, sizeof body, "GPRMC,%02d%02d%02d.00,%c,%s,%s,0.0,0.0,%02d%02d%02d,,,A", hh, mi, ss,
           fix ? 'A' : 'V', la, lo, d, m, y % 100);
  appendSentence(queue_, body);
  snprintf(body, sizeof body, "GPGGA,%02d%02d%02d.00,%s,%s,%d,%02d,1.0,%.1f,M,47.0,M,,", hh, mi, ss, la, lo,
           fix ? 1 : 0, fix ? sats : 0, elev);
  appendSentence(queue_, body);
}

int SimGps::read() {
  if (queue_.empty()) return -1;
  char c = queue_.front();
  queue_.pop_front();
  return (unsigned char)c;
}

// ---------------------------------------------------------------- storage

SimStorage::SimStorage(std::string path) : path_(std::move(path)) {
  if (path_.empty()) return;
  std::ifstream f(path_, std::ios::binary);
  uint32_t kl, dl;
  while (f.read(reinterpret_cast<char *>(&kl), 4)) {
    std::string key(kl, 0);
    f.read(&key[0], kl);
    f.read(reinterpret_cast<char *>(&dl), 4);
    std::vector<uint8_t> data(dl);
    f.read(reinterpret_cast<char *>(data.data()), dl);
    if (!f) break;
    records_[key] = data;
  }
}

bool SimStorage::load(const char *key, void *data, size_t len) {
  auto it = records_.find(key);
  if (it == records_.end() || it->second.size() != len) return false;
  memcpy(data, it->second.data(), len);
  return true;
}

void SimStorage::save(const char *key, const void *data, size_t len) {
  const uint8_t *p = static_cast<const uint8_t *>(data);
  records_[key] = std::vector<uint8_t>(p, p + len);
  write();
}

void SimStorage::erase(const char *key) {
  records_.erase(key);
  write();
}

void SimStorage::write() {
  if (path_.empty()) return;
  std::ofstream f(path_, std::ios::binary | std::ios::trunc);
  for (auto &kv : records_) {
    uint32_t kl = kv.first.size(), dl = kv.second.size();
    f.write(reinterpret_cast<const char *>(&kl), 4);
    f.write(kv.first.data(), kl);
    f.write(reinterpret_cast<const char *>(&dl), 4);
    f.write(reinterpret_cast<const char *>(kv.second.data()), dl);
  }
}

// ---------------------------------------------------------------- link

void ConsoleLink::send(int, const char *text) {
  fputs(text, stdout);
  fputc('\n', stdout);
  fflush(stdout);
}

// ---------------------------------------------------------------- sim

Sim::Sim(const std::string &storagePath)
    : storage_(storagePath),
      platform_{clock_, az_, alt_, estop_, inputs_, gps_, leds_, storage_, link_, "sim"},
      mount_(platform_) {}

void Sim::begin(int64_t utcMs) {
  utcOffset_ = utcMs - clock_.ms;
  mount_.begin();
  mount_.onConnect(kConsole, ClientKind::Console);
}

void Sim::advance(int64_t ms) {
  for (int64_t done = 0; done < ms;) {
    int64_t step = ms - done < kStepMs ? ms - done : kStepMs;
    double dt = step / 1000.0;
    az_.advance(dt, estop_.driversOn());
    alt_.advance(dt, estop_.driversOn());
    clock_.ms += step;
    done += step;
    gps_.tick(trueUtc());
    mount_.loop();
  }
}

// Mount frame (axis angles, x along azimuth axis 0) to the true horizon frame:
// north offset, then the tilt of the base.
Mat3 Sim::mountToWorld() {
  double c = cos(tiltDeg_ * kDeg), s = sin(tiltDeg_ * kDeg);
  Mat3 ry = {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};  // tilts the vertical towards north
  Mat3 tilt = mul(rotZ(tiltAzDeg_), mul(ry, rotZ(-tiltAzDeg_)));
  return mul(tilt, rotZ(-northDeg_));
}

Vec3 Sim::truthVector() {
  double azAxis = az_.mechSteps() * 360.0 / stepsPerRev_;
  double altAxis = alt_.mechSteps() * 360.0 / stepsPerRev_ + altZeroDeg_;
  return mul(mountToWorld(), vecFromAltaz(altAxis, azAxis));
}

void Sim::pointAt(const Vec3 &world) {
  Mat3 inv;
  invert(mountToWorld(), inv);
  double altAxis, azAxis;
  altazFromVec(mul(inv, world), altAxis, azAxis);
  altAxis -= altZeroDeg_;
  double curAz = az_.mechSteps() * 360.0 / stepsPerRev_;
  azAxis = curAz + wrap180(azAxis - curAz);  // the shorter way round
  // As if the user had jogged there: the step counters move with the mechanics.
  az_.pos = azAxis / 360.0 * stepsPerRev_ - az_.mechOffset;
  alt_.pos = altAxis / 360.0 * stepsPerRev_ - alt_.mechOffset;
  az_.v = az_.targetV = alt_.v = alt_.targetV = 0;
}

bool Sim::simCommand(JsonObjectConst cmd, JsonObject r, std::string &err, std::string &msg) {
  std::string name = cmd["cmd"] | "";
  auto num = [&](const char *key, double &out) {
    JsonVariantConst v = cmd[key];
    if (v.isNull()) return false;
    out = v.as<double>();
    return true;
  };
  double v;

  if (name == "sim_clock") {
    if (num("rate", v)) {
      if (v < 0) return err = "bad_arg", msg = "rate: 0 or more", false;
      rate = v;
    }
    if (!cmd["utc"].isNull()) utcOffset_ = cmd["utc"].as<long long>() - clock_.ms;
    r["rate"] = rate;
    r["utc"] = trueUtc();
    return true;
  }
  if (name == "sim_run") {
    if (!num("seconds", v) || v < 0 || v > 7 * 86400) return err = "bad_arg", msg = "seconds: 0 to 604800", false;
    advance((int64_t)llround(v * 1000));
    r["utc"] = trueUtc();
    return true;
  }
  if (name == "sim_estop") {
    estop_.isPressed = cmd["pressed"] | false;
    return true;
  }
  if (name == "sim_hand") {
    if (num("x", v)) inputs_.x = (float)v;
    if (num("y", v)) inputs_.y = (float)v;
    if (!cmd["sw"].isNull()) inputs_.sw = cmd["sw"].as<bool>();
    JsonArrayConst b = cmd["buttons"];
    for (int i = 0; i < 3 && i < (int)b.size(); i++) inputs_.buttons[i] = b[i].as<bool>();
    return true;
  }
  if (name == "sim_gps") {
    gps_.enabled = cmd["on"] | true;
    if (!cmd["fix"].isNull()) gps_.fix = cmd["fix"].as<bool>();
    if (num("lat", v)) gps_.lat = v;
    if (num("lon", v)) gps_.lon = v;
    if (num("elev", v)) gps_.elev = v;
    if (num("sats", v)) gps_.sats = (int)v;
    return true;
  }
  if (name == "sim_mount") {
    if (num("tilt", v)) tiltDeg_ = v;
    if (num("tilt_az", v)) tiltAzDeg_ = v;
    if (num("north", v)) northDeg_ = v;
    if (num("alt_zero", v)) altZeroDeg_ = v;
    return true;
  }
  if (name == "sim_fault") {
    std::string axis = cmd["axis"] | "";
    if ((axis != "az" && axis != "alt") || !num("slip", v)) return err = "bad_arg", msg = "axis and slip required", false;
    (axis == "az" ? az_ : alt_).mechOffset += v;
    return true;
  }
  if (name == "sim_point") {
    double ra, dec, alt, az;
    if (num("ra", ra) && num("dec", dec)) {
      radecToAltaz(ra, dec, trueUtc(), gps_.lat, gps_.lon, alt, az);
    } else if (!num("alt", alt) || !num("az", az)) {
      return err = "bad_arg", msg = "ra and dec, or alt and az, required", false;
    }
    pointAt(vecFromAltaz(alt, az));
    return true;
  }
  if (name == "sim_truth") {
    double alt, az, ra, dec;
    altazFromVec(truthVector(), alt, az);
    altazToRadec(alt, az, trueUtc(), gps_.lat, gps_.lon, ra, dec);
    r["utc"] = trueUtc();
    r["alt"] = alt;
    r["az"] = az;
    r["ra"] = ra;
    r["dec"] = dec;
    r["leds"]["red"] = leds_.red;
    r["leds"]["blue"] = leds_.blue;
    r["drivers"] = estop_.driversOn();
    return true;
  }
  err = "unknown_cmd";
  msg = "unknown simulator command \"" + name + "\"";
  return false;
}

void Sim::handleLine(int client, const char *line) {
  // sim_* commands are answered here; everything else goes to the firmware.
  const char *p = line;
  while (*p == ' ' || *p == '\t') p++;
  if (*p == '{' && strstr(p, "\"sim_")) {
    JsonDocument doc;
    if (!deserializeJson(doc, p) && doc.is<JsonObject>()) {
      JsonObjectConst cmd = doc.as<JsonObjectConst>();
      const char *name = cmd["cmd"] | "";
      if (!strncmp(name, "sim_", 4)) {
        JsonDocument rep;
        rep["re"] = cmd["id"];
        rep["ok"] = true;
        std::string err, msg;
        bool ok = simCommand(cmd, rep.as<JsonObject>(), err, msg);
        if (!ok) {
          rep.clear();
          rep["re"] = cmd["id"];
          rep["ok"] = false;
          rep["err"] = err;
          rep["msg"] = msg;
        }
        if (!cmd["id"].isNull()) {
          std::string out;
          serializeJson(rep, out);
          link_.send(client, out.c_str());
        }
        return;
      }
    }
  }
  mount_.onMessage(client, line);
}

}  // namespace sim
