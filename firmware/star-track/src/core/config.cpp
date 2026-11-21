#include "config.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace core {

namespace {

enum Type { INT, BOOL, FLOAT, TEXT };

struct Key {
  const char *name;
  Type type;
  size_t offset;
  double min, max;  // range for numbers, maximum length for text
};

#define K(name, type, field, lo, hi) {name, type, offsetof(Config, field), lo, hi}
const Key kKeys[] = {
    K("az.steps_per_rev", INT, azStepsPerRev, 1000, 10000000),
    K("alt.steps_per_rev", INT, altStepsPerRev, 1000, 10000000),
    K("az.reverse", BOOL, azReverse, 0, 0),
    K("alt.reverse", BOOL, altReverse, 0, 0),
    K("alt.min", FLOAT, altMin, -10, 90),
    K("alt.max", FLOAT, altMax, 0, 95),
    K("slew.speed", FLOAT, slewSpeed, 0.1, 20),
    K("slew.accel", FLOAT, slewAccel, 0.1, 50),
    K("jog.max_speed", FLOAT, jogMaxSpeed, 0.01, 20),
    K("park.alt", FLOAT, parkAlt, -10, 95),
    K("park.az", FLOAT, parkAz, -360, 360),
    K("track.gain", FLOAT, trackGain, 0.05, 10),
    K("sun.min_sep", FLOAT, sunMinSep, 5, 45),
    K("wifi.ssid", TEXT, wifiSsid, 1, 32),
    K("wifi.password", TEXT, wifiPassword, 8, 63),
};
#undef K

const Key *find(const char *name) {
  for (const Key &k : kKeys)
    if (!strcmp(k.name, name)) return &k;
  return nullptr;
}

void put(const Config &c, const Key &k, JsonObject out) {
  const char *p = reinterpret_cast<const char *>(&c) + k.offset;
  switch (k.type) {
    case INT: out[k.name] = *reinterpret_cast<const int32_t *>(p); break;
    case BOOL: out[k.name] = *reinterpret_cast<const bool *>(p); break;
    case FLOAT: out[k.name] = *reinterpret_cast<const float *>(p); break;
    case TEXT:
      // The password is never sent back.
      out[k.name] = strcmp(k.name, "wifi.password") ? p : (*p ? "********" : "");
      break;
  }
}

bool valid(const Key &k, JsonVariantConst v) {
  switch (k.type) {
    case INT: return v.is<long>() && v.as<long>() >= k.min && v.as<long>() <= k.max;
    case BOOL: return v.is<bool>();
    case FLOAT: return (v.is<float>() || v.is<long>()) && v.as<double>() >= k.min && v.as<double>() <= k.max;
    case TEXT: {
      if (!v.is<const char *>()) return false;
      size_t n = strlen(v.as<const char *>());
      return n >= k.min && n <= k.max;
    }
  }
  return false;
}

void apply(Config &c, const Key &k, JsonVariantConst v) {
  char *p = reinterpret_cast<char *>(&c) + k.offset;
  switch (k.type) {
    case INT: *reinterpret_cast<int32_t *>(p) = (int32_t)v.as<long>(); break;
    case BOOL: *reinterpret_cast<bool *>(p) = v.as<bool>(); break;
    case FLOAT: *reinterpret_cast<float *>(p) = v.as<float>(); break;
    case TEXT: {
      size_t cap = (size_t)k.max + 1;
      strncpy(p, v.as<const char *>(), cap - 1);
      p[cap - 1] = 0;
      break;
    }
  }
}

}  // namespace

void configToJson(const Config &c, JsonVariantConst keys, JsonObject out) {
  JsonArrayConst list = keys.as<JsonArrayConst>();
  if (list.isNull() || list.size() == 0) {
    for (const Key &k : kKeys) put(c, k, out);
    return;
  }
  for (JsonVariantConst name : list) {
    const Key *k = name.is<const char *>() ? find(name.as<const char *>()) : nullptr;
    if (k) put(c, *k, out);
  }
}

bool configFromJson(Config &c, JsonObjectConst in, char *err, int errLen) {
  for (JsonPairConst kv : in) {
    const Key *k = find(kv.key().c_str());
    if (!k) {
      snprintf(err, errLen, "unknown key %s", kv.key().c_str());
      return false;
    }
    if (!valid(*k, kv.value())) {
      snprintf(err, errLen, "bad value for %s", k->name);
      return false;
    }
  }
  Config next = c;
  for (JsonPairConst kv : in) apply(next, *find(kv.key().c_str()), kv.value());
  if (next.altMin >= next.altMax) {
    snprintf(err, errLen, "alt.min must be below alt.max");
    return false;
  }
  c = next;
  return true;
}

void configSave(const Config &c, JsonObject out) {
  for (const Key &k : kKeys) put(c, k, out);
  out["wifi.password"] = c.wifiPassword;
}

void configLoad(Config &c, JsonObjectConst in) {
  Config next = c;
  for (JsonPairConst kv : in) {
    const Key *k = find(kv.key().c_str());
    if (k && valid(*k, kv.value())) apply(next, *k, kv.value());
  }
  if (next.altMin >= next.altMax) {
    next.altMin = c.altMin;
    next.altMax = c.altMax;
  }
  c = next;
}

}  // namespace core
