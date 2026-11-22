// The mount: modes, tracking control and the protocol of docs/protocol.md.
// The platform calls begin() once, loop() as often as it can (every few ms),
// and the onConnect / onMessage / onDisconnect hooks from its transports.
#pragma once

#include <stdint.h>

#include "align.h"
#include "config.h"
#include "json.h"
#include "nmea.h"
#include "platform.h"

namespace core {

enum class Mode { EStop, Idle, Manual, Goto, Tracking, Parked };
enum class Source { None, Phone, Gps, Saved };
enum class ClientKind { WebSocket, Console };

const char *modeName(Mode m);

class Mount {
 public:
  static constexpr int kProto = 1;
  static constexpr const char *kFirmwareVersion = "0.3.2";
  static constexpr int kMaxMessage = 4096;
  static constexpr int kMaxClients = 8;
  static constexpr int kMaxTablePoints = 64;
  static constexpr int kMaxSettings = 1024;  // bytes of the settings' JSON record
  static constexpr int kControlPeriodMs = 50;  // 20 Hz
  static constexpr int kJogLifetimeMs = 200;
  static constexpr int kSunWarnEveryMs = 5 * 60 * 1000;  // the unaligned-Sun warning, while jogging

  explicit Mount(Platform &p) : p_(p) {}

  void begin();
  void loop();

  void onConnect(int client, ClientKind kind);
  void onDisconnect(int client);
  void onMessage(int client, const char *text);

  Mode mode() const { return mode_; }
  const Config &config() const { return cfg_; }
  // For the platform, which makes the password at the first boot. Saved.
  void setWifiPassword(const char *password);
  // The status event's fields (for the simulator's snapshots).
  void statusJson(JsonObject o) { fillStatus(o); }

 private:
  struct TablePoint {
    int64_t utc;
    double ra, dec;
  };
  struct Target {
    enum Kind { None, Sky, Table, Horizon, AxisPos, Sun } kind = None;
    double ra = 0, dec = 0;  // Sky
    double alt = 0, az = 0;  // Horizon (apparent) or AxisPos (axis degrees)
    TablePoint table[kMaxTablePoints];
    int tableN = 0;
  };
  struct Client {
    bool used = false;
    int id = 0;
    ClientKind kind = ClientKind::Console;
    int hz = 0;
    int64_t lastStatus = 0;
  };
  struct Error {
    const char *code = nullptr;
    char msg[200] = "";
  };
  using Handler = bool (Mount::*)(JsonObjectConst cmd, JsonObject reply);

  // Time and site.
  int64_t now() { return p_.clock.monoMs(); }
  int64_t utc() { return now() + utcOffset_; }
  bool gpsTime() { return gpsTimeMono_ >= 0 && now() - gpsTimeMono_ < 3000; }
  bool gpsSite() { return gpsSiteMono_ >= 0 && now() - gpsSiteMono_ < 3000; }
  void readGps();

  // Axes, in degrees on the sky axis.
  double azAxisDeg();
  double altAxisDeg();
  double azSpeedDeg();
  double altSpeedDeg();
  void runAz(double degPerS);
  void runAlt(double degPerS);
  void stopAxes();
  void setAxisPosition(Axis &a, double deg, int32_t spr, bool reverse);

  // Targets and control.
  bool targetRadec(int64_t t, double &ra, double &dec);
  bool targetHorizon(int64_t t, double &alt, double &az);
  bool targetAxis(int64_t t, double &axAlt, double &axAz);
  void control(double dt);
  void follow(double jogAz, double jogAlt);
  double etaSeconds(double axAlt, double axAz);
  void setMode(Mode m);
  void stopMotion();
  void startGoto(bool track, bool park);
  void updateLeds();

  // Pointing.
  bool horizonFromAxis(double &alt, double &az);  // false before alignment
  // After lost steps: sets the axis counters so the model puts (alt, az) where the
  // tube points now. Corrects the whole sky, unlike a sync.
  void resetAxes(double alt, double az);

  // Sun safety (outside solar mode).
  bool sunWatch(double &sunAlt, double &sunAz);  // true when the keep-out zone applies
  bool movingIntoSun(double vAz, double vAlt);    // braking from this speed would end in the zone
  bool slewNearSun(double toAxAlt, double toAxAz);
  bool sunAllows(double alt, double az, double toAxAlt, double toAxAz);  // sets err_

  // Persistence.
  void loadConfig();
  void saveConfig();
  void saveState();
  void saveAlignment();

  // Protocol.
  void send(int client, JsonDocument &doc);
  void broadcastStatus(bool modeChange);
  void fillStatus(JsonObject o);
  void log(const char *level, const char *fmt, ...);
  bool fail(const char *code, const char *fmt, ...);
  bool need(bool ok, const char *flag, const char *hint);
  bool armed();
  bool getNumber(JsonObjectConst cmd, const char *key, double lo, double hi, double &out);
  Client *client(int id);

  bool cmdHello(JsonObjectConst, JsonObject);
  bool cmdStatus(JsonObjectConst, JsonObject);
  bool cmdStream(JsonObjectConst, JsonObject);
  bool cmdSetTime(JsonObjectConst, JsonObject);
  bool cmdSetSite(JsonObjectConst, JsonObject);
  bool cmdArm(JsonObjectConst, JsonObject);
  bool cmdStop(JsonObjectConst, JsonObject);
  bool cmdJog(JsonObjectConst, JsonObject);
  bool cmdGoto(JsonObjectConst, JsonObject);
  bool cmdGotoAltaz(JsonObjectConst, JsonObject);
  bool cmdTrack(JsonObjectConst, JsonObject);
  bool cmdTrackTable(JsonObjectConst, JsonObject);
  bool cmdAlignAdd(JsonObjectConst, JsonObject);
  bool cmdAlignClear(JsonObjectConst, JsonObject);
  bool cmdAlignList(JsonObjectConst, JsonObject);
  bool cmdSync(JsonObjectConst, JsonObject);
  bool cmdPark(JsonObjectConst, JsonObject);
  bool cmdUnpark(JsonObjectConst, JsonObject);
  bool cmdSetAltZero(JsonObjectConst, JsonObject);
  bool cmdGetConfig(JsonObjectConst, JsonObject);
  bool cmdSetConfig(JsonObjectConst, JsonObject);
  bool cmdSolar(JsonObjectConst, JsonObject);

  Platform &p_;
  Config cfg_;
  Alignment align_;
  Nmea nmea_;

  Mode mode_ = Mode::Idle;
  bool timeOk_ = false, siteOk_ = false, posOk_ = true;
  int64_t utcOffset_ = 0;
  Source timeSrc_ = Source::None, siteSrc_ = Source::None;
  double lat_ = 0, lon_ = 0, elev_ = 0;
  int64_t gpsTimeMono_ = -1, gpsSiteMono_ = -1, gpsSeenMono_ = -1;
  uint32_t boots_ = 0;  // starts of the board, counted in flash

  Target target_;
  bool gotoTrack_ = false, gotoPark_ = false;
  double offAz_ = 0, offAlt_ = 0;  // jog offset while tracking, axis degrees
  bool manualFromTracking_ = false;

  float appJogX_ = 0, appJogY_ = 0;
  int64_t appJogUntil_ = 0;
  int appJogClient_ = -1;
  bool handActive_ = false;
  bool stopButton_ = false;
  bool solar_ = false;          // solar mode: the user confirmed a solar filter
  bool sunJogWarned_ = false;
  int64_t sunUnalignedWarned_ = -1;  // when "the Sun is up and the mount is not aligned" was last logged

  int64_t lastControl_ = 0;
  int currentClient_ = -1;  // sender of the command being handled
  Client clients_[kMaxClients];
  Error err_;
};

}  // namespace core
