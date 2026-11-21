// Minimal NMEA 0183 reader for the GPS module: RMC (time, date, validity) and
// GGA (fix, satellites, elevation). Sentences with a bad checksum are dropped.
#pragma once

#include <stdint.h>

namespace core {

struct GpsFix {
  bool timeValid = false;  // last RMC had status A
  int64_t utcMs = 0;       // time of that RMC
  bool posValid = false;   // last GGA had a fix
  double lat = 0, lon = 0, elev = 0;
  int sats = 0;
};

class Nmea {
 public:
  // Feeds one byte. Returns 1 after a valid RMC, 2 after a valid GGA, else 0.
  int feed(char c);
  const GpsFix &fix() const { return fix_; }
  uint32_t goodSentences() const { return good_; }

 private:
  int parse();
  char buf_[96];
  int len_ = 0;
  bool inSentence_ = false;
  uint32_t good_ = 0;
  GpsFix fix_;
};

}  // namespace core
