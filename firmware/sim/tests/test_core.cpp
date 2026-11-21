// Unit tests for the core's maths, NMEA reader and pointing model.
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "align.h"
#include "astro.h"
#include "nmea.h"

using namespace core;

static int failures = 0, checks = 0;

#define CHECK_NEAR(a, b, tol)                                                                          \
  do {                                                                                                 \
    double _a = (a), _b = (b);                                                                         \
    checks++;                                                                                          \
    if (!(fabs(_a - _b) <= (tol))) {                                                                   \
      printf("FAIL %s:%d: %s = %.9g, expected %.9g (±%g)\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); \
      failures++;                                                                                      \
    }                                                                                                  \
  } while (0)

#define CHECK(cond)                                              \
  do {                                                           \
    checks++;                                                    \
    if (!(cond)) {                                               \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
      failures++;                                                \
    }                                                            \
  } while (0)

static int64_t utcMs(int y, int mo, int d, int h, int mi, double s) {
  return daysFromCivil(y, mo, d) * 86400000LL + h * 3600000LL + mi * 60000LL + (int64_t)llround(s * 1000);
}

static void testTime() {
  CHECK(daysFromCivil(1970, 1, 1) == 0);
  CHECK(daysFromCivil(2000, 3, 1) == 11017);
  CHECK_NEAR(julianDate(utcMs(2000, 1, 1, 12, 0, 0)), 2451545.0, 1e-9);
  // Meeus, Astronomical Algorithms, examples 12.a and 12.b.
  CHECK_NEAR(gmstDeg(utcMs(1987, 4, 10, 0, 0, 0)), (13 + 10 / 60.0 + 46.3668 / 3600) * 15, 1e-4);
  CHECK_NEAR(gmstDeg(utcMs(1987, 4, 10, 19, 21, 0)), (8 + 34 / 60.0 + 57.0896 / 3600) * 15, 1e-4);
}

static void testSun() {
  // Meeus, example 25.a: 1992-10-13 0h TD, apparent RA 13h13m31.4s, Dec -7°47'06".
  double ra, dec;
  sunRadec(utcMs(1992, 10, 13, 0, 0, 0), ra, dec);
  CHECK_NEAR(ra, 13 + 13 / 60.0 + 31.4 / 3600, 0.01 / 15);
  CHECK_NEAR(dec, -(7 + 47 / 60.0 + 6 / 3600.0), 0.01);
  // Near the equinox the Sun is close to RA 0, Dec 0.
  sunRadec(utcMs(2026, 3, 20, 14, 46, 0), ra, dec);
  CHECK_NEAR(wrap180(ra * 15), 0, 0.05);
  CHECK_NEAR(dec, 0, 0.02);
}

static void testCoordinates() {
  // Round trips through alt/az, with and without refraction.
  const double lat = 45, lon = 5;
  const int64_t t = utcMs(2026, 11, 21, 22, 0, 0);
  const double targets[][2] = {{5.588, -5.39}, {5.278, 46.0}, {20.69, 45.28}, {2.53, 89.26}, {0.1, 10}};
  for (auto &tg : targets) {
    double alt, az, ra, dec;
    radecToAltaz(tg[0], tg[1], t, lat, lon, alt, az);
    altazToRadec(alt, az, t, lat, lon, ra, dec);
    if (alt > 0) {  // refraction inverses agree to a few arcseconds above the horizon
      CHECK_NEAR(wrap180((ra - tg[0]) * 15) * cos(tg[1] * kDeg), 0, 2e-3);
      CHECK_NEAR(dec, tg[1], 2e-3);
    }
  }
  // Polaris stays near the latitude in altitude.
  double alt, az;
  radecToAltaz(2.53, 89.26, t, lat, lon, alt, az);
  CHECK_NEAR(alt, lat, 1.0);
  CHECK(az < 2 || az > 358);
  // Refraction: about 34' at the horizon, 1' at 45°, none at the zenith.
  CHECK_NEAR(refractionFromApparent(0) * 60, 34.5, 1.0);
  CHECK_NEAR(refractionFromApparent(45) * 60, 1.0, 0.05);
  CHECK_NEAR(refractionFromApparent(90), 0, 1e-9);
  CHECK_NEAR(wrap180(190), -170, 1e-12);
  CHECK_NEAR(wrap360(-10), 350, 1e-12);
}

static void sendNmea(Nmea &n, const char *body, int &result) {
  unsigned sum = 0;
  for (const char *p = body; *p; p++) sum ^= (unsigned char)*p;
  char line[128];
  snprintf(line, sizeof line, "$%s*%02X\r\n", body, sum);
  result = 0;
  for (const char *p = line; *p; p++) {
    int r = n.feed(*p);
    if (r) result = r;
  }
}

static void testNmea() {
  Nmea n;
  int r;
  sendNmea(n, "GPRMC,221530.00,A,4500.0000,N,00500.0000,E,0.0,0.0,211126,,,A", r);
  CHECK(r == 1);
  CHECK(n.fix().timeValid);
  CHECK(n.fix().utcMs == utcMs(2026, 11, 21, 22, 15, 30));
  sendNmea(n, "GNGGA,221530.00,4830.5000,N,00012.0000,W,1,07,1.0,51.0,M,47.0,M,,", r);
  CHECK(r == 2);
  CHECK(n.fix().posValid);
  CHECK_NEAR(n.fix().lat, 48.508333, 1e-6);
  CHECK_NEAR(n.fix().lon, -0.2, 1e-9);
  CHECK_NEAR(n.fix().elev, 51, 1e-9);
  CHECK(n.fix().sats == 7);
  sendNmea(n, "GPRMC,221531.00,V,,,,,,,211126,,,N", r);
  CHECK(!n.fix().timeValid);
  // A bad checksum is dropped.
  uint32_t good = n.goodSentences();
  const char *bad = "$GPRMC,221532.00,A,4500.0000,N,00500.0000,E,0.0,0.0,211126,,,A*00\r\n";
  for (const char *p = bad; *p; p++) n.feed(*p);
  CHECK(n.goodSentences() == good);
}

// Synthetic mount: tilted base, unknown north, as in the simulator.
static void mountAxes(double tilt, double tiltAz, double north, double hAlt, double hAz, double &axAlt,
                      double &axAz) {
  double c = cos(tilt * kDeg), s = sin(tilt * kDeg);
  Mat3 ry = {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
  Mat3 toWorld = mul(mul(rotZ(tiltAz), mul(ry, rotZ(-tiltAz))), rotZ(-north));
  Mat3 inv;
  invert(toWorld, inv);
  altazFromVec(mul(inv, vecFromAltaz(hAlt, hAz)), axAlt, axAz);
}

static void testAlignment() {
  const double tilt = 2, tiltAz = 40, north = 123.4;
  const double stars[][2] = {{40, 50}, {30, 200}, {60, 300}, {20, 120}};
  Alignment a;
  // One star: the azimuth offset alone.
  AlignStar s{};
  s.hAlt = stars[0][0];
  s.hAz = stars[0][1];
  mountAxes(0, 0, north, s.hAlt, s.hAz, s.axAlt, s.axAz);
  a.add(s);
  double axAlt, axAz, ex, ez;
  a.toAxis(25, 10, axAlt, axAz);
  mountAxes(0, 0, north, 25, 10, ex, ez);
  CHECK_NEAR(axAlt, ex, 1e-9);
  CHECK_NEAR(wrap180(axAz - ez), 0, 1e-9);

  // Two and three stars on a tilted base: exact for a pure rotation.
  for (int n = 2; n <= 4; n++) {
    a.clear();
    for (int i = 0; i < n; i++) {
      AlignStar st{};
      st.hAlt = stars[i][0];
      st.hAz = stars[i][1];
      mountAxes(tilt, tiltAz, north, st.hAlt, st.hAz, st.axAlt, st.axAz);
      a.add(st);
    }
    for (double alt = 10; alt < 85; alt += 15)
      for (double az = 0; az < 360; az += 30) {
        a.toAxis(alt, az, axAlt, axAz);
        mountAxes(tilt, tiltAz, north, alt, az, ex, ez);
        CHECK_NEAR(axAlt, ex, 1e-6);
        CHECK_NEAR(wrap180(axAz - ez) * cos(ex * kDeg), 0, 1e-6);
        double hAlt, hAz;
        a.toHorizon(axAlt, axAz, hAlt, hAz);
        CHECK_NEAR(hAlt, alt, 1e-6);
      }
    CHECK_NEAR(a.rmsDeg(), 0, 1e-6);
  }

  // A sync corrects its own point fully and fades to nothing 30° away.
  double pAlt, pAz;
  a.toAxis(45, 90, pAlt, pAz);
  a.sync(45, 90, pAlt + 0.2, pAz + 0.3);
  a.toAxis(45, 90, axAlt, axAz);
  CHECK_NEAR(axAlt, pAlt + 0.2, 1e-6);
  CHECK_NEAR(wrap180(axAz - pAz - 0.3), 0, 1e-6);
  a.toAxis(45, 160, axAlt, axAz);
  mountAxes(tilt, tiltAz, north, 45, 160, ex, ez);
  CHECK_NEAR(axAlt, ex, 1e-6);
  double hAlt, hAz;
  a.toHorizon(pAlt + 0.2, pAz + 0.3, hAlt, hAz);
  CHECK_NEAR(hAlt, 45, 1e-3);
  CHECK_NEAR(wrap180(hAz - 90), 0, 1e-3);
}

int main() {
  testTime();
  testSun();
  testCoordinates();
  testNmea();
  testAlignment();
  printf("test_core: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
