// Settings changed with get_config / set_config and saved in flash.
#pragma once

#include <stdint.h>

#include "json.h"

namespace core {

struct Config {
  int32_t azStepsPerRev = 160000;  // 200 steps × 1/64 × 250/20
  int32_t altStepsPerRev = 160000;
  bool azReverse = false;
  bool altReverse = false;
  float altMin = 0, altMax = 90;  // degrees
  float slewSpeed = 3;            // °/s
  float slewAccel = 2;            // °/s²
  float jogMaxSpeed = 3;          // °/s
  float parkAlt = 0, parkAz = 0;  // axis degrees
  float trackGain = 1;            // 1/s
  char wifiSsid[33] = "star-track";
  char wifiPassword[65] = "";
};

// Writes the keys in `keys` (all if null or empty) into `out`.
void configToJson(const Config &c, JsonVariantConst keys, JsonObject out);
// Checks every key first; on success applies them all. On failure, writes the
// offending key into `err` and changes nothing.
bool configFromJson(Config &c, JsonObjectConst in, char *err, int errLen);

}  // namespace core
