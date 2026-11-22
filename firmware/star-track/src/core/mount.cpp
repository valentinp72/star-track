#include "mount.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "astro.h"

namespace core {

namespace {

constexpr double kArrivedDeg = 0.005;  // goto ends when both axes are this close
constexpr float kDeadZone = 0.1f;
// Two alignment stars are as far apart on the axes as in the sky (the mount is a
// rotation of the horizon), give or take a level error and refraction.
constexpr double kAlignToleranceDeg = 2.0;
// Near the zenith the azimuth of a direction is poorly defined (a few arcseconds
// of error become arcminutes of azimuth), so a sync or an axis reset there would
// spoil the rest of the sky.
constexpr double kZenithLimitDeg = 75.0;

// Flash records. Changing a struct changes its size, so an old record is ignored.
struct StateRecord {
  uint8_t parked;
  int32_t azSteps, altSteps;
};
struct SiteRecord {
  double lat, lon, elev;
};
struct AlignRecord {
  int32_t n;
  AlignStar stars[Alignment::kMaxStars];
};

float deadZone(float v) {
  if (v > 1) v = 1;
  if (v < -1) v = -1;
  if (fabsf(v) < kDeadZone) return 0;
  return (v - copysignf(kDeadZone, v)) / (1 - kDeadZone);
}

// A round joystick's position mapped onto the square: a push in any direction
// moves each axis as a straight push of the same length along that axis would
// (at the rim, 45° gives both axes full speed). A joystick that already reaches
// its corners is unchanged there.
void toSquare(float &x, float &y) {
  float m = fmaxf(fabsf(x), fabsf(y));
  if (m == 0) return;
  float r = fminf(hypotf(x, y), 1.0f);
  x *= r / m;
  y *= r / m;
}

// Correction speed for a position error: limited by the slew speed, by the
// distance needed to brake, and proportional (gain k) near the target.
double correction(double err, double vmax, double accel, double k) {
  double m = fabs(err);
  double v = fmin(vmax, fmin(sqrt(2 * accel * m * 0.8), k * m));
  return copysign(v, err);
}

double clampAbs(double v, double lim) { return v > lim ? lim : (v < -lim ? -lim : v); }

const char *sourceName(Source s) {
  switch (s) {
    case Source::Phone: return "phone";
    case Source::Gps: return "gps";
    case Source::Saved: return "saved";
    default: return "none";
  }
}

// Lagrange interpolation through up to four points.
double lagrange(const double *x, const double *y, int n, double t) {
  double r = 0;
  for (int i = 0; i < n; i++) {
    double l = 1;
    for (int j = 0; j < n; j++)
      if (j != i) l *= (t - x[j]) / (x[i] - x[j]);
    r += l * y[i];
  }
  return r;
}

}  // namespace

const char *modeName(Mode m) {
  switch (m) {
    case Mode::EStop: return "estop";
    case Mode::Idle: return "idle";
    case Mode::Manual: return "manual";
    case Mode::Goto: return "goto";
    case Mode::Tracking: return "tracking";
    case Mode::Parked: return "parked";
  }
  return "?";
}

// ---------------------------------------------------------------- lifecycle

void Mount::begin() {
  loadConfig();
  p_.storage.load("boots", &boots_, sizeof boots_);
  boots_++;
  p_.storage.save("boots", &boots_, sizeof boots_);
  SiteRecord site;
  if (p_.storage.load("site", &site, sizeof site)) {
    lat_ = site.lat;
    lon_ = site.lon;
    elev_ = site.elev;
    siteOk_ = true;
    siteSrc_ = Source::Saved;
  }
  StateRecord st;
  mode_ = Mode::Idle;
  posOk_ = true;  // the tube is assumed level at power-on, azimuth at axis 0
  if (p_.storage.load("state", &st, sizeof st) && st.parked) {
    p_.az.setPosition(st.azSteps);
    p_.alt.setPosition(st.altSteps);
    AlignRecord ar;
    if (p_.storage.load("align", &ar, sizeof ar)) align_.restore(ar.stars, ar.n);
    mode_ = Mode::Parked;
  }
  p_.estop.enableDrivers(true);
  if (p_.estop.pressed()) mode_ = Mode::EStop;
  lastControl_ = now();
  updateLeds();
}

void Mount::loop() {
  readGps();

  if (mode_ != Mode::EStop && p_.estop.pressed()) {
    p_.az.forceStop();
    p_.alt.forceStop();
    posOk_ = false;
    appJogUntil_ = 0;
    setMode(Mode::EStop);
    log("warn", "E-stop pressed: drivers off, position uncertain; release it and send arm");
  }

  int64_t t = now();
  if (t - lastControl_ >= kControlPeriodMs) {
    double dt = (t - lastControl_) / 1000.0;
    lastControl_ = t;
    control(dt);
    updateLeds();
  }
  broadcastStatus(false);
}

void Mount::readGps() {
  int c;
  while ((c = p_.gps.read()) >= 0) {
    int r = nmea_.feed((char)c);
    if (!r) continue;
    int64_t t = now();
    gpsSeenMono_ = t;
    const GpsFix &f = nmea_.fix();
    if (r == 1 && f.timeValid) {
      if (timeSrc_ != Source::Gps) log("info", "time from GPS");
      utcOffset_ = f.utcMs - t;
      timeOk_ = true;
      timeSrc_ = Source::Gps;
      gpsTimeMono_ = t;
    }
    if (r == 2 && f.posValid) {
      if (siteSrc_ != Source::Gps) log("info", "site from GPS");
      lat_ = f.lat;
      lon_ = f.lon;
      elev_ = f.elev;
      siteOk_ = true;
      siteSrc_ = Source::Gps;
      gpsSiteMono_ = t;
    }
  }
}

// ---------------------------------------------------------------- axes

double Mount::azAxisDeg() {
  return p_.az.position() * 360.0 / cfg_.azStepsPerRev * (cfg_.azReverse ? -1 : 1);
}
double Mount::altAxisDeg() {
  return p_.alt.position() * 360.0 / cfg_.altStepsPerRev * (cfg_.altReverse ? -1 : 1);
}
double Mount::azSpeedDeg() { return p_.az.speed() * 360.0 / cfg_.azStepsPerRev * (cfg_.azReverse ? -1 : 1); }
double Mount::altSpeedDeg() {
  return p_.alt.speed() * 360.0 / cfg_.altStepsPerRev * (cfg_.altReverse ? -1 : 1);
}

void Mount::runAz(double v) {
  double k = cfg_.azStepsPerRev / 360.0 * (cfg_.azReverse ? -1 : 1);
  p_.az.runAt((float)(v * k), (float)(cfg_.slewAccel * fabs(k)));
}
void Mount::runAlt(double v) {
  double k = cfg_.altStepsPerRev / 360.0 * (cfg_.altReverse ? -1 : 1);
  p_.alt.runAt((float)(v * k), (float)(cfg_.slewAccel * fabs(k)));
}

void Mount::stopAxes() {
  p_.az.stop((float)(cfg_.slewAccel * cfg_.azStepsPerRev / 360.0));
  p_.alt.stop((float)(cfg_.slewAccel * cfg_.altStepsPerRev / 360.0));
}

void Mount::setAxisPosition(Axis &a, double deg, int32_t spr, bool reverse) {
  a.setPosition((int32_t)lround(deg / 360.0 * spr * (reverse ? -1 : 1)));
}

// ---------------------------------------------------------------- targets

bool Mount::targetRadec(int64_t t, double &ra, double &dec) {
  if (target_.kind == Target::Sun) {
    sunRadec(t + utcOffset_, ra, dec);
    return true;
  }
  if (target_.kind == Target::Sky) {
    ra = target_.ra;
    dec = target_.dec;
    return true;
  }
  if (target_.kind != Target::Table) return false;
  const TablePoint *tp = target_.table;
  int n = target_.tableN;
  if (t < tp[0].utc || t > tp[n - 1].utc) return false;
  int i = 0;
  while (i < n - 2 && tp[i + 1].utc <= t) i++;
  int first = i - 1 < 0 ? 0 : i - 1;
  int last = first + 3 > n - 1 ? n - 1 : first + 3;
  if (last - first < 3 && last - 3 >= 0) first = last - 3;
  double x[4], yr[4], yd[4];
  int m = 0;
  for (int k = first; k <= last; k++, m++) {
    x[m] = (tp[k].utc - tp[first].utc) / 3600000.0;
    // Unwrap RA around the first point (a table can cross 0 h).
    yr[m] = tp[first].ra + wrap180((tp[k].ra - tp[first].ra) * 15.0) / 15.0;
    yd[m] = tp[k].dec;
  }
  double xt = (t - tp[first].utc) / 3600000.0;
  ra = wrap24(lagrange(x, yr, m, xt));
  dec = lagrange(x, yd, m, xt);
  return true;
}

bool Mount::targetHorizon(int64_t t, double &alt, double &az) {
  if (target_.kind == Target::Horizon) {
    alt = target_.alt;
    az = target_.az;
    return true;
  }
  double ra, dec;
  if (!timeOk_ || !siteOk_ || !targetRadec(t, ra, dec)) return false;
  radecToAltaz(ra, dec, t + utcOffset_, lat_, lon_, alt, az);
  return true;
}

bool Mount::targetAxis(int64_t t, double &axAlt, double &axAz) {
  if (target_.kind == Target::None) return false;
  if (target_.kind == Target::AxisPos) {
    axAlt = target_.alt;
    axAz = target_.az;
  } else {
    double alt, az;
    if (!targetHorizon(t, alt, az)) return false;
    align_.toAxis(alt, az, axAlt, axAz);
  }
  axAlt += offAlt_;
  axAz += offAz_;
  return true;
}

double Mount::etaSeconds(double axAlt, double axAz) {
  double v = cfg_.slewSpeed, a = cfg_.slewAccel, worst = 0;
  const double d[2] = {fabs(axAlt - altAxisDeg()), fabs(wrap180(axAz - azAxisDeg()))};
  for (double di : d) {
    double t = di > v * v / a ? di / v + v / a : 2 * sqrt(di / a);
    if (t > worst) worst = t;
  }
  return worst;
}

// ---------------------------------------------------------------- control

void Mount::setMode(Mode m) {
  if (m == mode_) return;
  mode_ = m;
  updateLeds();
  broadcastStatus(true);
}

void Mount::stopMotion() {
  if (mode_ == Mode::Manual || mode_ == Mode::Goto || mode_ == Mode::Tracking) {
    stopAxes();
    setMode(Mode::Idle);
  }
  appJogUntil_ = 0;
}

void Mount::startGoto(bool track, bool park) {
  gotoTrack_ = track;
  gotoPark_ = park;
  offAz_ = offAlt_ = 0;
  setMode(Mode::Goto);
}

void Mount::control(double dt) {
  // Hand unit first: it has priority over the app.
  float hx = deadZone(p_.inputs.joyX()), hy = deadZone(p_.inputs.joyY());
  handActive_ = hx != 0 || hy != 0;
  bool button = p_.inputs.button(0);
  if (button && !stopButton_) stopMotion();
  stopButton_ = button;

  float jx = 0, jy = 0;
  if (handActive_) {
    jx = hx;
    jy = hy;
  } else if (now() < appJogUntil_) {
    jx = appJogX_;
    jy = appJogY_;
  }
  toSquare(jx, jy);
  double vAz = cfg_.jogMaxSpeed * jx * fabsf(jx);
  double vAlt = cfg_.jogMaxSpeed * jy * fabsf(jy);
  bool jogging = vAz != 0 || vAlt != 0;

  switch (mode_) {
    case Mode::EStop:
    case Mode::Parked:
      return;
    case Mode::Idle:
      if (jogging) {
        manualFromTracking_ = false;
        setMode(Mode::Manual);
      }
      break;
    case Mode::Goto:
      if (jogging) {
        manualFromTracking_ = false;
        log("info", "goto cancelled by a jog");
        setMode(Mode::Manual);
      }
      break;
    case Mode::Tracking:
      if (jogging) {
        manualFromTracking_ = true;
        setMode(Mode::Manual);
      }
      break;
    case Mode::Manual:
      if (!jogging) {
        if (manualFromTracking_) {
          setMode(Mode::Tracking);
        } else {
          stopAxes();
          setMode(Mode::Idle);
          return;
        }
      }
      break;
  }

  if (mode_ == Mode::Manual && jogging) {
    if (movingIntoSun(vAz, vAlt)) {
      vAz = vAlt = 0;
      if (!sunJogWarned_) log("warn", "jog stopped %.0f° from the Sun: solar mode is off", cfg_.sunMinSep);
      sunJogWarned_ = true;
    } else {
      sunJogWarned_ = false;
    }
    double sAlt, sAz;
    if (align_.count() == 0 && (sunUnalignedWarned_ < 0 || now() - sunUnalignedWarned_ >= kSunWarnEveryMs) &&
        sunWatch(sAlt, sAz) && sAlt > 0) {
      log("warn", "the Sun is up and the mount is not aligned: it cannot keep the tube away from the Sun");
      sunUnalignedWarned_ = now();
    }
  }

  if (mode_ == Mode::Manual && !manualFromTracking_) {
    // The altitude limits, once aligned: before, the tube's altitude is unknown
    // and jogs go anywhere. The jog stops early enough to brake before the limit.
    double alt, az;
    if (horizonFromAxis(alt, az)) {
      double v = altSpeedDeg(), brake = v * v / (2 * cfg_.slewAccel);
      if ((alt - (v < 0 ? brake : 0) <= cfg_.altMin && vAlt < 0) ||
          (alt + (v > 0 ? brake : 0) >= cfg_.altMax && vAlt > 0))
        vAlt = 0;
    }
    runAz(vAz);
    runAlt(vAlt);
    return;
  }
  if (mode_ == Mode::Manual) {  // jog while tracking: the target follows
    offAz_ += vAz * dt;
    offAlt_ += vAlt * dt;
    follow(vAz, vAlt);
    return;
  }
  if (mode_ == Mode::Goto || mode_ == Mode::Tracking) follow(0, 0);
}

// One control step towards the target: predicted rate + correction.
void Mount::follow(double jogAz, double jogAlt) {
  int64_t t = now();
  double alt0, az0, alt1, az1;
  if (!targetAxis(t, alt0, az0) || !targetAxis(t + 1000, alt1, az1)) {
    stopAxes();
    setMode(Mode::Idle);
    log("warn", "target lost (no time or site, or the end of the table): tracking stopped");
    return;
  }
  double hAlt, hAz;
  if (mode_ != Mode::Goto && target_.kind != Target::AxisPos && targetHorizon(t, hAlt, hAz) &&
      hAlt < cfg_.altMin) {
    stopAxes();
    setMode(Mode::Idle);
    log("warn", "target below alt.min, tracking stopped");
    return;
  }
  if (target_.kind == Target::Sun && !solar_) {
    stopAxes();
    setMode(Mode::Idle);
    log("warn", "solar mode is off: stopped following the Sun");
    return;
  }
  double eAz = wrap180(az0 - azAxisDeg()), eAlt = alt0 - altAxisDeg();
  double rAz = wrap180(az1 - az0) + jogAz, rAlt = (alt1 - alt0) + jogAlt;
  double vmax = cfg_.slewSpeed;
  double k = cfg_.trackGain;
  double vAz = clampAbs(rAz + correction(eAz, vmax, cfg_.slewAccel, k), vmax);
  double vAlt = clampAbs(rAlt + correction(eAlt, vmax, cfg_.slewAccel, k), vmax);
  if (movingIntoSun(vAz, vAlt)) {
    stopAxes();
    setMode(Mode::Idle);
    log("warn", "stopped %.0f° from the Sun: solar mode is off", cfg_.sunMinSep);
    return;
  }
  runAz(vAz);
  runAlt(vAlt);

  if (mode_ == Mode::Goto && fabs(eAz) < kArrivedDeg && fabs(eAlt) < kArrivedDeg) {
    if (gotoPark_) {
      p_.az.stop(cfg_.slewAccel * cfg_.azStepsPerRev / 360.0f);
      p_.alt.stop(cfg_.slewAccel * cfg_.altStepsPerRev / 360.0f);
      target_.kind = Target::None;
      solar_ = false;
      setMode(Mode::Parked);
      saveState();
      saveAlignment();
    } else if (gotoTrack_) {
      setMode(Mode::Tracking);
    } else {
      stopAxes();
      setMode(Mode::Idle);
    }
  }
}

void Mount::updateLeds() {
  int64_t t = now();
  bool blink = (t / 250) % 2;
  uint8_t red = 0;
  switch (mode_) {
    case Mode::EStop: red = blink ? 255 : 0; break;
    case Mode::Goto:
    case Mode::Manual: red = 60; break;
    case Mode::Tracking: red = 20; break;
    case Mode::Idle: red = 8; break;
    case Mode::Parked: red = 0; break;
  }
  uint8_t blue = (!timeOk_ || !siteOk_) && (t / 1000) % 2 ? 6 : 0;  // waiting for time or site
  p_.leds.set(red, blue);
}

void Mount::resetAxes(double alt, double az) {
  align_.clearSync();
  double pAlt, pAz;
  align_.toAxis(alt, az, pAlt, pAz);
  double curAlt = altAxisDeg(), curAz = azAxisDeg();
  double dAlt = pAlt - curAlt, dAz = wrap180(pAz - curAz);
  setAxisPosition(p_.alt, pAlt, cfg_.altStepsPerRev, cfg_.altReverse);
  setAxisPosition(p_.az, curAz + dAz, cfg_.azStepsPerRev, cfg_.azReverse);
  log("info", "axis counters reset after lost steps: alt %+.3f°, az %+.3f°", dAlt, dAz);
}

bool Mount::horizonFromAxis(double &alt, double &az) {
  if (align_.count() == 0) return false;
  align_.toHorizon(altAxisDeg(), azAxisDeg(), alt, az);
  return true;
}

// ---------------------------------------------------------------- Sun safety

bool Mount::sunWatch(double &sunAlt, double &sunAz) {
  if (solar_ || !timeOk_ || !siteOk_) return false;
  double ra, dec;
  sunRadec(utc(), ra, dec);
  radecToAltaz(ra, dec, utc(), lat_, lon_, sunAlt, sunAz);
  return sunAlt > -2;  // below that, the Earth is in the way
}

bool Mount::movingIntoSun(double vAz, double vAlt) {
  double sAlt, sAz, alt0, az0, alt1, az1;
  if (align_.count() == 0 || !sunWatch(sAlt, sAz)) return false;
  Vec3 sun = vecFromAltaz(sAlt, sAz);
  align_.toHorizon(altAxisDeg(), azAxisDeg(), alt0, az0);
  // Where the axes would be after one more control period and a full stop.
  auto lead = [&](double v) { return v * 0.1 + v * fabs(v) / (2 * cfg_.slewAccel); };
  align_.toHorizon(altAxisDeg() + lead(vAlt), azAxisDeg() + lead(vAz), alt1, az1);
  double now = separationDeg(vecFromAltaz(alt0, az0), sun), ahead = separationDeg(vecFromAltaz(alt1, az1), sun);
  return ahead < cfg_.sunMinSep && ahead < now;
}

// Both axes slew at the same top speed, so the path is a diagonal then a
// straight run on the axis with further to go. Checked every 0.5°.
bool Mount::slewNearSun(double toAxAlt, double toAxAz) {
  double sAlt, sAz;
  if (align_.count() == 0 || !sunWatch(sAlt, sAz)) return false;
  Vec3 sun = vecFromAltaz(sAlt, sAz);
  double alt0 = altAxisDeg(), az0 = azAxisDeg();
  double dAlt = toAxAlt - alt0, dAz = wrap180(toAxAz - az0);
  double len = fmax(fabs(dAlt), fabs(dAz));
  int n = (int)ceil(len / 0.5);
  // A slew that starts inside the zone (solar mode just turned off) may leave it,
  // as long as it moves away from the Sun until it is out.
  bool leaving = true;
  double prev = -1;
  for (int i = 0; i <= n; i++) {
    double s = n ? len * i / n : 0;
    double a = alt0 + copysign(fmin(s, fabs(dAlt)), dAlt), z = az0 + copysign(fmin(s, fabs(dAz)), dAz);
    double hAlt, hAz;
    align_.toHorizon(a, z, hAlt, hAz);
    double d = separationDeg(vecFromAltaz(hAlt, hAz), sun);
    if (d >= cfg_.sunMinSep) {
      leaving = false;
    } else if (!leaving || d < prev) {
      return true;
    }
    prev = d;
  }
  return false;
}

bool Mount::sunAllows(double alt, double az, double toAxAlt, double toAxAz) {
  double sAlt, sAz;
  if (!sunWatch(sAlt, sAz)) return true;
  if (align_.count() == 0)
    return fail("sun", "the Sun is up and the azimuth is unknown: align first, or turn on solar mode");
  double d = separationDeg(vecFromAltaz(alt, az), vecFromAltaz(sAlt, sAz));
  if (d < cfg_.sunMinSep)
    return fail("sun", "target %.1f° from the Sun (limit %.0f°): turn on solar mode with a solar filter", d,
                cfg_.sunMinSep);
  if (slewNearSun(toAxAlt, toAxAz))
    return fail("sun", "the slew would pass within %.0f° of the Sun: go round it with a jog first", cfg_.sunMinSep);
  return true;
}

// ---------------------------------------------------------------- persistence

// The settings are a JSON record ("settings"), which other firmware versions
// can read. Firmware 0.1.0 saved the Config struct as it is ("config").
void Mount::loadConfig() {
  char buf[kMaxSettings];
  size_t n = p_.storage.size("settings");
  if (n > 0 && n <= sizeof buf && p_.storage.load("settings", buf, n)) {
    JsonDocument doc;
    if (!deserializeJson(doc, buf, n)) configLoad(cfg_, doc.as<JsonObjectConst>());
    return;
  }
  if (p_.storage.load("config", &cfg_, sizeof cfg_)) {
    saveConfig();
    p_.storage.erase("config");
  }
}

void Mount::saveConfig() {
  JsonDocument doc;
  configSave(cfg_, doc.to<JsonObject>());
  char buf[kMaxSettings];
  size_t n = serializeJson(doc, buf, sizeof buf);
  if (n > 0 && n < sizeof buf) p_.storage.save("settings", buf, n);
}

void Mount::setWifiPassword(const char *password) {
  snprintf(cfg_.wifiPassword, sizeof cfg_.wifiPassword, "%s", password);
  saveConfig();
}

void Mount::saveState() {
  StateRecord st{(uint8_t)(mode_ == Mode::Parked), p_.az.position(), p_.alt.position()};
  p_.storage.save("state", &st, sizeof st);
}

void Mount::saveAlignment() {
  AlignRecord ar{};
  ar.n = align_.count();
  for (int i = 0; i < ar.n; i++) ar.stars[i] = align_.star(i);
  p_.storage.save("align", &ar, sizeof ar);
}

// ---------------------------------------------------------------- protocol

Mount::Client *Mount::client(int id) {
  for (Client &c : clients_)
    if (c.used && c.id == id) return &c;
  return nullptr;
}

void Mount::onConnect(int id, ClientKind kind) {
  Client *c = client(id);
  if (!c)
    for (Client &f : clients_)
      if (!f.used) {
        c = &f;
        break;
      }
  if (!c) return;
  c->used = true;
  c->id = id;
  c->kind = kind;
  c->hz = kind == ClientKind::WebSocket ? 10 : 0;
  c->lastStatus = 0;
}

void Mount::onDisconnect(int id) {
  Client *c = client(id);
  if (c) c->used = false;
  if (appJogClient_ == id) appJogUntil_ = 0;
}

void Mount::send(int clientId, JsonDocument &doc) {
  static char buf[kMaxMessage];
  size_t n = serializeJson(doc, buf, sizeof buf);
  if (n == 0 || n >= sizeof buf - 1) return;
  p_.link.send(clientId, buf);
}

void Mount::broadcastStatus(bool modeChange) {
  int64_t t = now();
  JsonDocument doc;
  bool filled = false;
  for (Client &c : clients_) {
    if (!c.used || c.hz <= 0) continue;
    if (!modeChange && t - c.lastStatus < 1000 / c.hz) continue;
    if (!filled) {
      fillStatus(doc.to<JsonObject>());
      filled = true;
    }
    c.lastStatus = t;
    send(c.id, doc);
  }
}

void Mount::fillStatus(JsonObject o) {
  o["ev"] = "status";
  if (timeOk_)
    o["utc"] = utc();
  else
    o["utc"] = nullptr;
  o["mode"] = modeName(mode_);
  o["uptime"] = now() / 1000;
  if (p_.sys) p_.sys(o["sys"].to<JsonObject>());
  o["time_ok"] = timeOk_;
  o["site_ok"] = siteOk_;
  o["aligned"] = align_.count() > 0;
  o["pos_ok"] = posOk_;
  o["time_src"] = sourceName(timeSrc_);
  o["site_src"] = sourceName(siteSrc_);
  if (siteOk_) {
    JsonObject s = o["site"].to<JsonObject>();
    s["lat"] = lat_;
    s["lon"] = lon_;
    s["elev"] = elev_;
  } else {
    o["site"] = nullptr;
  }
  if (gpsSeenMono_ >= 0 && now() - gpsSeenMono_ < 3000) {
    JsonObject g = o["gps"].to<JsonObject>();
    g["fix"] = gpsTime();
    g["sats"] = nmea_.fix().sats;
  } else {
    o["gps"] = nullptr;
  }

  JsonObject pos = o["pos"].to<JsonObject>();
  double alt, az;
  if (horizonFromAxis(alt, az)) {
    if (timeOk_ && siteOk_) {
      double ra, dec;
      altazToRadec(alt, az, utc(), lat_, lon_, ra, dec);
      pos["ra"] = ra;
      pos["dec"] = dec;
    } else {
      pos["ra"] = nullptr;
      pos["dec"] = nullptr;
    }
    pos["alt"] = alt;
    pos["az"] = az;
  } else {
    pos["ra"] = nullptr;
    pos["dec"] = nullptr;
    pos["alt"] = altAxisDeg();
    pos["az"] = nullptr;
  }

  if (target_.kind == Target::None) {
    o["target"] = nullptr;
  } else {
    JsonObject tg = o["target"].to<JsonObject>();
    double ra, dec;
    if (targetRadec(now(), ra, dec)) {
      tg["ra"] = ra;
      tg["dec"] = dec;
    }
    if (target_.kind == Target::AxisPos) {
      if (align_.count() > 0) {
        align_.toHorizon(target_.alt, target_.az, alt, az);
        tg["alt"] = alt;
        tg["az"] = az;
      } else {
        tg["alt"] = target_.alt;
      }
    } else if (targetHorizon(now(), alt, az)) {
      tg["alt"] = alt;
      tg["az"] = az;
    }
  }

  JsonObject axes = o["axes"].to<JsonObject>();
  JsonObject a = axes["az"].to<JsonObject>();
  a["steps"] = p_.az.position();
  a["speed"] = azSpeedDeg();
  JsonObject l = axes["alt"].to<JsonObject>();
  l["steps"] = p_.alt.position();
  l["speed"] = altSpeedDeg();
  o["estop"] = p_.estop.pressed();
  o["hand"] = handActive_;
  o["solar"] = solar_;
  if (timeOk_ && siteOk_) {
    double ra, dec, sAlt, sAz;
    sunRadec(utc(), ra, dec);
    radecToAltaz(ra, dec, utc(), lat_, lon_, sAlt, sAz);
    JsonObject sun = o["sun"].to<JsonObject>();
    sun["alt"] = sAlt;
    sun["az"] = sAz;
    double pAlt, pAz;
    if (horizonFromAxis(pAlt, pAz))
      sun["sep"] = separationDeg(vecFromAltaz(pAlt, pAz), vecFromAltaz(sAlt, sAz));
    else
      sun["sep"] = nullptr;
  } else {
    o["sun"] = nullptr;
  }
}

void Mount::log(const char *level, const char *fmt, ...) {
  char msg[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof msg, fmt, ap);
  va_end(ap);
  JsonDocument doc;
  doc["ev"] = "log";
  doc["level"] = level;
  doc["msg"] = msg;
  bool debug = !strcmp(level, "debug");
  for (Client &c : clients_)
    if (c.used && (!debug || c.kind == ClientKind::Console)) send(c.id, doc);
}

bool Mount::fail(const char *code, const char *fmt, ...) {
  err_.code = code;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(err_.msg, sizeof err_.msg, fmt, ap);
  va_end(ap);
  return false;
}

bool Mount::need(bool ok, const char *flag, const char *hint) {
  return ok || fail("not_ready", "%s is false: %s", flag, hint);
}

bool Mount::armed() {
  if (mode_ == Mode::EStop) {
    return fail("estop", p_.estop.pressed() ? "the E-stop is pressed" : "the E-stop was pressed: send arm");
  }
  if (mode_ == Mode::Parked) return fail("parked", "the mount is parked: send unpark first");
  return true;
}

bool Mount::getNumber(JsonObjectConst cmd, const char *key, double lo, double hi, double &out) {
  JsonVariantConst v = cmd[key];
  if (!v.is<double>() && !v.is<long long>()) return fail("bad_arg", "%s: a number is required", key);
  out = v.as<double>();
  if (!(out >= lo && out <= hi)) return fail("bad_arg", "%s: out of range (%g to %g)", key, lo, hi);
  return true;
}

void Mount::onMessage(int clientId, const char *text) {
  while (*text == ' ' || *text == '\t') text++;
  size_t len = strlen(text);
  JsonDocument rep;
  if (len > (size_t)kMaxMessage) {
    rep["re"] = nullptr;
    rep["ok"] = false;
    rep["err"] = "too_long";
    rep["msg"] = "messages are limited to 4 KB";
    send(clientId, rep);
    return;
  }
  if (*text != '{') return;  // blank or stray terminal line

  JsonDocument doc;
  DeserializationError de = deserializeJson(doc, text, len);
  if (de || !doc.is<JsonObject>()) {
    rep["re"] = nullptr;
    rep["ok"] = false;
    rep["err"] = "bad_json";
    rep["msg"] = de ? de.c_str() : "not a JSON object";
    send(clientId, rep);
    return;
  }
  JsonObjectConst cmd = doc.as<JsonObjectConst>();
  bool hasId = !cmd["id"].isNull();
  const char *name = cmd["cmd"] | "";

  static const struct {
    const char *name;
    Handler fn;
  } kCommands[] = {
      {"hello", &Mount::cmdHello},           {"status", &Mount::cmdStatus},
      {"stream", &Mount::cmdStream},         {"set_time", &Mount::cmdSetTime},
      {"set_site", &Mount::cmdSetSite},      {"arm", &Mount::cmdArm},
      {"stop", &Mount::cmdStop},             {"jog", &Mount::cmdJog},
      {"goto", &Mount::cmdGoto},             {"goto_altaz", &Mount::cmdGotoAltaz},
      {"track", &Mount::cmdTrack},           {"track_table", &Mount::cmdTrackTable},
      {"align_add", &Mount::cmdAlignAdd},    {"align_clear", &Mount::cmdAlignClear},
      {"align_list", &Mount::cmdAlignList},  {"sync", &Mount::cmdSync},
      {"park", &Mount::cmdPark},             {"unpark", &Mount::cmdUnpark},
      {"set_alt_zero", &Mount::cmdSetAltZero}, {"get_config", &Mount::cmdGetConfig},
      {"set_config", &Mount::cmdSetConfig},  {"solar", &Mount::cmdSolar},
  };

  rep["re"] = cmd["id"];
  rep["ok"] = true;
  err_ = Error();
  bool ok = false;
  bool found = false;
  for (const auto &c : kCommands) {
    if (!strcmp(c.name, name)) {
      found = true;
      currentClient_ = clientId;
      ok = (this->*c.fn)(cmd, rep.as<JsonObject>());
      break;
    }
  }
  if (!found) ok = fail("unknown_cmd", "unknown command \"%s\"", name);
  if (!hasId) return;
  if (!ok) {
    rep.clear();
    rep["re"] = cmd["id"];
    rep["ok"] = false;
    rep["err"] = err_.code;
    rep["msg"] = err_.msg;
  }
  send(clientId, rep);
}

// ---------------------------------------------------------------- commands

bool Mount::cmdHello(JsonObjectConst, JsonObject r) {
  r["proto"] = kProto;
  r["fw"] = kFirmwareVersion;
  r["board"] = p_.board;
  r["sim"] = !strcmp(p_.board, "sim");
  r["uptime"] = now() / 1000;
  r["boots"] = boots_;
  r["reset"] = p_.reset;
  if (*p_.resetMsg) r["reset_msg"] = p_.resetMsg;
  return true;
}

bool Mount::cmdStatus(JsonObjectConst, JsonObject r) {
  fillStatus(r);
  r.remove("ev");
  return true;
}

bool Mount::cmdStream(JsonObjectConst cmd, JsonObject) {
  double hz;
  if (!getNumber(cmd, "hz", 0, 10, hz)) return false;
  Client *c = client(currentClient_);
  if (!c) {
    onConnect(currentClient_, ClientKind::Console);
    c = client(currentClient_);
  }
  if (c) c->hz = (int)hz;
  return true;
}

bool Mount::cmdSetTime(JsonObjectConst cmd, JsonObject r) {
  JsonVariantConst v = cmd["utc"];
  if (!v.is<long long>() || v.as<long long>() < 0) return fail("bad_arg", "utc: milliseconds since 1970 required");
  if (gpsTime()) {
    r["source"] = "gps";
    return true;
  }
  utcOffset_ = v.as<long long>() - now();
  timeOk_ = true;
  timeSrc_ = Source::Phone;
  r["source"] = "phone";
  return true;
}

bool Mount::cmdSetSite(JsonObjectConst cmd, JsonObject r) {
  double lat, lon, elev = 0;
  if (!getNumber(cmd, "lat", -90, 90, lat) || !getNumber(cmd, "lon", -180, 180, lon)) return false;
  if (!cmd["elev"].isNull() && !getNumber(cmd, "elev", -500, 9000, elev)) return false;
  if (gpsSite()) {
    r["source"] = "gps";
    return true;
  }
  lat_ = lat;
  lon_ = lon;
  elev_ = elev;
  siteOk_ = true;
  siteSrc_ = Source::Phone;
  if (cmd["save"] | false) {
    SiteRecord s{lat, lon, elev};
    p_.storage.save("site", &s, sizeof s);
  }
  r["source"] = "phone";
  return true;
}

bool Mount::cmdArm(JsonObjectConst, JsonObject) {
  if (p_.estop.pressed()) return fail("estop", "the E-stop is still pressed");
  if (mode_ == Mode::EStop) {
    p_.estop.enableDrivers(true);
    target_.kind = Target::None;
    setMode(Mode::Idle);
  }
  return true;
}

bool Mount::cmdStop(JsonObjectConst, JsonObject) {
  stopMotion();
  return true;
}

bool Mount::cmdJog(JsonObjectConst cmd, JsonObject) {
  if (!armed()) return false;
  double x, y;
  if (!getNumber(cmd, "x", -1, 1, x) || !getNumber(cmd, "y", -1, 1, y)) return false;
  appJogX_ = (float)x;
  appJogY_ = (float)y;
  appJogClient_ = currentClient_;
  appJogUntil_ = (x == 0 && y == 0) ? 0 : now() + kJogLifetimeMs;
  return true;
}

bool Mount::cmdGoto(JsonObjectConst cmd, JsonObject r) {
  double ra, dec;
  bool sun = cmd["sun"] | false;
  if (!armed() || !need(timeOk_, "time_ok", "send set_time") || !need(siteOk_, "site_ok", "send set_site") ||
      !need(align_.count() > 0, "aligned", "add an alignment star"))
    return false;
  if (sun) {
    if (!solar_) return fail("sun", "solar mode is off: fit a solar filter, then send solar with filter: true");
    sunRadec(utc(), ra, dec);
  } else if (!getNumber(cmd, "ra", 0, 24, ra) || !getNumber(cmd, "dec", -90, 90, dec)) {
    return false;
  }
  double alt, az, axAlt, axAz;
  radecToAltaz(ra, dec, utc(), lat_, lon_, alt, az);
  if (alt < cfg_.altMin || alt > cfg_.altMax)
    return fail("below_limit", "target at alt %.1f, limits %.1f to %.1f", alt, cfg_.altMin, cfg_.altMax);
  align_.toAxis(alt, az, axAlt, axAz);
  if (!sunAllows(alt, az, axAlt, axAz)) return false;
  target_.kind = sun ? Target::Sun : Target::Sky;
  target_.ra = wrap24(ra);
  target_.dec = dec;
  startGoto(cmd["track"] | true, false);
  r["alt"] = alt;
  r["az"] = az;
  r["eta"] = etaSeconds(axAlt, axAz);
  return true;
}

bool Mount::cmdGotoAltaz(JsonObjectConst cmd, JsonObject r) {
  if (!armed() || !need(posOk_, "pos_ok", "align on a star or sync")) return false;
  bool hasAlt = !cmd["alt"].isNull(), hasAz = !cmd["az"].isNull();
  if (!hasAlt && !hasAz) return fail("bad_arg", "alt or az is required");
  double alt = 0, az = 0;
  if (hasAlt && !getNumber(cmd, "alt", -90, 90, alt)) return false;
  if (hasAz && !getNumber(cmd, "az", -360, 360, az)) return false;
  if (hasAz && !need(align_.count() > 0, "aligned", "azimuth needs an alignment star")) return false;
  if (hasAlt && (alt < cfg_.altMin || alt > cfg_.altMax))
    return fail("below_limit", "alt %.1f outside %.1f to %.1f", alt, cfg_.altMin, cfg_.altMax);
  if (align_.count() > 0) {
    double curAlt, curAz, axAlt, axAz;
    horizonFromAxis(curAlt, curAz);
    double tAlt = hasAlt ? alt : curAlt, tAz = hasAz ? wrap360(az) : curAz;
    align_.toAxis(tAlt, tAz, axAlt, axAz);
    if (!sunAllows(tAlt, tAz, axAlt, axAz)) return false;
  } else if (!sunAllows(0, 0, 0, 0)) {  // refused while the Sun is up: the azimuth is unknown
    return false;
  }
  if (align_.count() == 0) {
    target_.kind = Target::AxisPos;
    target_.alt = alt;
    target_.az = azAxisDeg();
  } else {
    double curAlt, curAz;
    horizonFromAxis(curAlt, curAz);
    target_.kind = Target::Horizon;
    target_.alt = hasAlt ? alt : curAlt;
    target_.az = hasAz ? wrap360(az) : curAz;
  }
  startGoto(false, false);
  double axAlt, axAz;
  targetAxis(now(), axAlt, axAz);
  r["eta"] = etaSeconds(axAlt, axAz);
  return true;
}

bool Mount::cmdTrack(JsonObjectConst cmd, JsonObject) {
  if (!(cmd["on"] | false)) {
    if (mode_ == Mode::Tracking) {
      stopAxes();
      setMode(Mode::Idle);
    }
    return true;
  }
  if (!armed() || !need(timeOk_, "time_ok", "send set_time") || !need(siteOk_, "site_ok", "send set_site") ||
      !need(align_.count() > 0, "aligned", "add an alignment star"))
    return false;
  if (target_.kind == Target::Sun && !solar_)
    return fail("sun", "solar mode is off: fit a solar filter, then send solar with filter: true");
  if (target_.kind != Target::Sky && target_.kind != Target::Table && target_.kind != Target::Sun) {
    double alt, az;
    horizonFromAxis(alt, az);
    target_.kind = Target::Sky;
    altazToRadec(alt, az, utc(), lat_, lon_, target_.ra, target_.dec);
    offAz_ = offAlt_ = 0;
  }
  setMode(Mode::Tracking);
  return true;
}

bool Mount::cmdTrackTable(JsonObjectConst cmd, JsonObject r) {
  if (!armed() || !need(timeOk_, "time_ok", "send set_time") || !need(siteOk_, "site_ok", "send set_site") ||
      !need(align_.count() > 0, "aligned", "add an alignment star"))
    return false;
  JsonArrayConst pts = cmd["points"];
  int n = pts.size();
  if (n < 2 || n > kMaxTablePoints) return fail("bad_arg", "points: 2 to %d required", kMaxTablePoints);
  static TablePoint tmp[kMaxTablePoints];
  for (int i = 0; i < n; i++) {
    JsonArrayConst p = pts[i];
    if (p.size() != 3 || !p[0].is<long long>() || !(p[1].is<double>() || p[1].is<long long>()) ||
        !(p[2].is<double>() || p[2].is<long long>()))
      return fail("bad_arg", "points[%d]: [utc, ra, dec] required", i);
    tmp[i] = {p[0].as<long long>(), p[1].as<double>(), p[2].as<double>()};
    if (i > 0 && tmp[i].utc <= tmp[i - 1].utc) return fail("bad_arg", "points: not in time order");
  }
  int64_t t = utc();
  if (t < tmp[0].utc || t >= tmp[n - 1].utc) return fail("bad_arg", "points: the table does not cover now");
  // Sun: no point of the table may come near it (outside solar mode).
  double sAlt, sAz;
  if (sunWatch(sAlt, sAz)) {
    for (int i = 0; i < n; i++) {
      if (tmp[i].utc < t - 600000) continue;
      double sra, sdec;
      sunRadec(tmp[i].utc, sra, sdec);
      Vec3 a = vecFromAltaz(0, 0), b = a;
      double alt1, az1, alt2, az2;
      radecToAltaz(tmp[i].ra, tmp[i].dec, tmp[i].utc, lat_, lon_, alt1, az1);
      radecToAltaz(sra, sdec, tmp[i].utc, lat_, lon_, alt2, az2);
      a = vecFromAltaz(alt1, az1);
      b = vecFromAltaz(alt2, az2);
      if (alt2 > -2 && separationDeg(a, b) < cfg_.sunMinSep)
        return fail("sun", "points[%d] is %.1f° from the Sun (limit %.0f°): turn on solar mode with a solar filter", i,
                    separationDeg(a, b), cfg_.sunMinSep);
    }
    int k = 0;
    while (k < n - 2 && tmp[k + 1].utc <= t) k++;
    double f = (double)(t - tmp[k].utc) / (tmp[k + 1].utc - tmp[k].utc);
    double ra = tmp[k].ra + f * wrap180((tmp[k + 1].ra - tmp[k].ra) * 15) / 15, dec = tmp[k].dec + f * (tmp[k + 1].dec - tmp[k].dec);
    double alt, az, axAlt, axAz;
    radecToAltaz(ra, dec, t, lat_, lon_, alt, az);
    align_.toAxis(alt, az, axAlt, axAz);
    if (!sunAllows(alt, az, axAlt, axAz)) return false;
  }
  bool replace = target_.kind == Target::Table && (mode_ == Mode::Tracking || mode_ == Mode::Manual);
  memcpy(target_.table, tmp, sizeof(TablePoint) * n);
  target_.tableN = n;
  // Table times are UTC; targetRadec() takes the monotonic clock.
  for (int i = 0; i < n; i++) target_.table[i].utc -= utcOffset_;
  target_.kind = Target::Table;
  double alt, az;
  targetHorizon(now(), alt, az);
  if (!replace) {
    if (alt < cfg_.altMin) {
      target_.kind = Target::None;
      return fail("below_limit", "target at alt %.1f, below alt.min", alt);
    }
    startGoto(true, false);
  }
  double axAlt, axAz;
  targetAxis(now(), axAlt, axAz);
  r["alt"] = alt;
  r["az"] = az;
  r["eta"] = replace ? 0 : etaSeconds(axAlt, axAz);
  return true;
}

bool Mount::cmdAlignAdd(JsonObjectConst cmd, JsonObject r) {
  double ra, dec;
  if (!need(timeOk_, "time_ok", "send set_time") || !need(siteOk_, "site_ok", "send set_site") ||
      !getNumber(cmd, "ra", 0, 24, ra) || !getNumber(cmd, "dec", -90, 90, dec))
    return false;
  AlignStar s{};
  strncpy(s.name, cmd["name"] | "", sizeof s.name - 1);
  s.ra = ra;
  s.dec = dec;
  radecToAltaz(ra, dec, utc(), lat_, lon_, s.hAlt, s.hAz);
  if (!posOk_ && align_.count() > 0) {
    if (s.hAlt > kZenithLimitDeg)
      return fail("bad_arg", "after lost steps, use a star below %.0f° altitude: near the zenith the azimuth is "
                  "too uncertain", kZenithLimitDeg);
    resetAxes(s.hAlt, s.hAz);  // steps were lost: bring the earlier stars back in line first
    r["axes_reset"] = true;
  }
  s.axAlt = altAxisDeg();
  s.axAz = wrap360(azAxisDeg());
  // Fitted on a copy first: kept only if every star fits the new model.
  const char *name = s.name[0] ? s.name : "this star";
  Alignment next = align_;
  next.add(s);
  if (next.count() == 1 && fabs(next.altIndexDeg()) > Alignment::kMaxIndexDeg)
    return fail("align_mismatch",
                "%s is at %.1f° altitude but the tube reads %.1f°: centre the star, or level the tube and send "
                "set_alt_zero",
                name, s.hAlt, s.axAlt);
  for (int i = 0; i < next.count(); i++) {
    double e = next.fitErrorDeg(i);
    if (e > kAlignToleranceDeg) {
      const AlignStar &o = next.star(i);
      bool self = !strcmp(o.name, s.name) && o.ra == s.ra && o.dec == s.dec;
      return fail("align_mismatch",
                  "%s does not fit the earlier stars (%s is %.1f° off): centre the star, check its name, or clear "
                  "the alignment if the mount was moved",
                  name, self ? name : (o.name[0] ? o.name : "an earlier star"), e);
    }
  }
  align_ = next;
  // The model now accounts for the jog that centred the star.
  offAz_ = offAlt_ = 0;
  posOk_ = true;
  saveAlignment();
  r["n"] = align_.count();
  r["rms"] = align_.rmsDeg();
  r["alt_index"] = align_.altIndexDeg();
  return true;
}

bool Mount::cmdAlignClear(JsonObjectConst, JsonObject) {
  align_.clear();
  if (mode_ == Mode::Tracking || mode_ == Mode::Goto) stopMotion();
  saveAlignment();
  return true;
}

bool Mount::cmdAlignList(JsonObjectConst, JsonObject r) {
  JsonArray a = r["stars"].to<JsonArray>();
  for (int i = 0; i < align_.count(); i++) {
    JsonObject s = a.add<JsonObject>();
    s["name"] = align_.star(i).name;
    s["ra"] = align_.star(i).ra;
    s["dec"] = align_.star(i).dec;
    s["err"] = align_.residualDeg(i);
  }
  r["alt_index"] = align_.altIndexDeg();
  return true;
}

bool Mount::cmdSync(JsonObjectConst cmd, JsonObject r) {
  double ra, dec;
  if (!need(align_.count() > 0, "aligned", "add an alignment star first") || !getNumber(cmd, "ra", 0, 24, ra) ||
      !getNumber(cmd, "dec", -90, 90, dec))
    return false;
  double alt, az;
  radecToAltaz(ra, dec, utc(), lat_, lon_, alt, az);
  if (alt > kZenithLimitDeg)
    return fail("bad_arg", "sync on an object below %.0f° altitude: near the zenith the azimuth is too uncertain",
                kZenithLimitDeg);
  if (posOk_) {
    align_.sync(alt, az, altAxisDeg(), wrap360(azAxisDeg()));  // a local refinement
  } else {
    resetAxes(alt, az);  // steps were lost (E-stop): the same offset everywhere
    r["axes_reset"] = true;
  }
  offAz_ = offAlt_ = 0;
  posOk_ = true;
  return true;
}

bool Mount::cmdPark(JsonObjectConst, JsonObject r) {
  if (!armed() || !need(posOk_, "pos_ok", "align on a star or sync")) return false;
  if (align_.count() > 0) {
    double alt, az;
    align_.toHorizon(cfg_.parkAlt, cfg_.parkAz, alt, az);
    if (!sunAllows(alt, az, cfg_.parkAlt, cfg_.parkAz)) return false;
  } else if (!sunAllows(0, 0, 0, 0)) {
    return false;
  }
  target_.kind = Target::AxisPos;
  target_.alt = cfg_.parkAlt;
  target_.az = cfg_.parkAz;
  startGoto(false, true);
  r["eta"] = etaSeconds(target_.alt, target_.az);
  return true;
}

bool Mount::cmdUnpark(JsonObjectConst, JsonObject) {
  if (mode_ == Mode::Parked) {
    setMode(Mode::Idle);
    saveState();
  }
  return true;
}

bool Mount::cmdSetAltZero(JsonObjectConst, JsonObject) {
  if (mode_ == Mode::Goto || mode_ == Mode::Tracking || mode_ == Mode::Manual) stopMotion();
  setAxisPosition(p_.alt, 0, cfg_.altStepsPerRev, cfg_.altReverse);
  align_.clear();
  saveAlignment();
  posOk_ = true;
  return true;
}

bool Mount::cmdGetConfig(JsonObjectConst cmd, JsonObject r) {
  configToJson(cfg_, cmd["keys"], r["config"].to<JsonObject>());
  return true;
}

bool Mount::cmdSolar(JsonObjectConst cmd, JsonObject r) {
  bool on = cmd["on"] | false;
  if (on && !(cmd["filter"] | false))
    return fail("bad_arg", "filter: send filter: true to confirm a solar filter covers the front of the tube");
  if (on && !solar_) log("warn", "solar mode on: the Sun keep-out zone is off until solar mode is turned off");
  if (!on && solar_) log("info", "solar mode off: the tube keeps %.0f° away from the Sun again", cfg_.sunMinSep);
  solar_ = on;
  r["solar"] = solar_;
  return true;
}

bool Mount::cmdSetConfig(JsonObjectConst cmd, JsonObject) {
  JsonObjectConst in = cmd["config"];
  if (in.isNull()) return fail("bad_arg", "config: an object is required");
  char err[64];
  if (!configFromJson(cfg_, in, err, sizeof err)) return fail("bad_arg", "%s", err);
  saveConfig();
  return true;
}

}  // namespace core
