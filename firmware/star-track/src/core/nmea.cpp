#include "nmea.h"

#include <stdlib.h>
#include <string.h>

#include "astro.h"

namespace core {

namespace {

int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Splits on commas in place; returns the number of fields.
int split(char *s, char **fields, int max) {
  int n = 0;
  fields[n++] = s;
  for (char *p = s; *p && n < max; p++) {
    if (*p == ',') {
      *p = 0;
      fields[n++] = p + 1;
    }
  }
  return n;
}

// ddmm.mmmm with a hemisphere letter, to signed degrees.
bool coord(const char *v, const char *hemi, double &out) {
  if (!*v || !*hemi) return false;
  double raw = atof(v);
  int deg = (int)(raw / 100);
  out = deg + (raw - deg * 100) / 60.0;
  if (*hemi == 'S' || *hemi == 'W') out = -out;
  return true;
}

}  // namespace

int Nmea::feed(char c) {
  if (c == '$') {
    inSentence_ = true;
    len_ = 0;
    return 0;
  }
  if (!inSentence_) return 0;
  if (c == '\r' || c == '\n') {
    inSentence_ = false;
    buf_[len_] = 0;
    return parse();
  }
  if (len_ >= (int)sizeof(buf_) - 1) {
    inSentence_ = false;
    return 0;
  }
  buf_[len_++] = c;
  return 0;
}

int Nmea::parse() {
  char *star = strchr(buf_, '*');
  if (!star || hexVal(star[1]) < 0 || hexVal(star[2]) < 0) return 0;
  uint8_t sum = 0;
  for (char *p = buf_; p < star; p++) sum ^= (uint8_t)*p;
  if (sum != hexVal(star[1]) * 16 + hexVal(star[2])) return 0;
  *star = 0;
  good_++;

  char *f[20];
  int n = split(buf_, f, 20);
  if (strlen(f[0]) != 5) return 0;
  const char *type = f[0] + 2;  // GPRMC, GNRMC, ...

  if (!strcmp(type, "RMC") && n >= 10) {
    // 1 time hhmmss.ss, 2 status, 3-6 position, 9 date ddmmyy
    if (*f[2] != 'A' || strlen(f[1]) < 6 || strlen(f[9]) != 6) {
      fix_.timeValid = false;
      return 0;
    }
    int hh = (f[1][0] - '0') * 10 + (f[1][1] - '0');
    int mm = (f[1][2] - '0') * 10 + (f[1][3] - '0');
    double ss = atof(f[1] + 4);
    int dd = (f[9][0] - '0') * 10 + (f[9][1] - '0');
    int mo = (f[9][2] - '0') * 10 + (f[9][3] - '0');
    int yy = (f[9][4] - '0') * 10 + (f[9][5] - '0');
    int64_t days = daysFromCivil(2000 + yy, mo, dd);
    fix_.utcMs = days * 86400000LL + (int64_t)hh * 3600000 + (int64_t)mm * 60000 + (int64_t)(ss * 1000 + 0.5);
    fix_.timeValid = true;
    return 1;
  }
  if (!strcmp(type, "GGA") && n >= 10) {
    // 2-5 position, 6 fix quality, 7 satellites, 9 altitude (m)
    int quality = atoi(f[6]);
    fix_.sats = atoi(f[7]);
    double lat, lon;
    if (quality > 0 && coord(f[2], f[3], lat) && coord(f[4], f[5], lon)) {
      fix_.lat = lat;
      fix_.lon = lon;
      fix_.elev = atof(f[9]);
      fix_.posValid = true;
    } else {
      fix_.posValid = false;
    }
    return 2;
  }
  return 0;
}

}  // namespace core
