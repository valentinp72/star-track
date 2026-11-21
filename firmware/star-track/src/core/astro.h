// Time and coordinate conversions. Angles in degrees unless named otherwise,
// right ascension in hours, time in UTC milliseconds since 1970.
#pragma once

#include <stdint.h>

namespace core {

struct Vec3 {
  double x, y, z;
};

struct Mat3 {
  double m[3][3];
};

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;

double wrap360(double deg);  // 0 ≤ result < 360
double wrap180(double deg);  // -180 < result ≤ 180
double wrap24(double hours);

double julianDate(int64_t utcMs);
// Greenwich mean sidereal time (Meeus, eq. 12.4), degrees.
double gmstDeg(int64_t utcMs);
double lstDeg(int64_t utcMs, double lonDeg);

// Geometric conversions between equatorial (RA/Dec of date) and horizontal
// coordinates. Azimuth from north through east.
void equToHor(double raH, double decDeg, double lstDeg, double latDeg, double &altDeg, double &azDeg);
void horToEqu(double altDeg, double azDeg, double lstDeg, double latDeg, double &raH, double &decDeg);

// Atmospheric refraction at 10 °C and 1010 hPa, in degrees.
double refractionFromTrue(double trueAltDeg);           // Saemundsson
double refractionFromApparent(double apparentAltDeg);   // Bennett

// Apparent (refracted) alt/az of a target, and back.
void radecToAltaz(double raH, double decDeg, int64_t utcMs, double latDeg, double lonDeg, double &altDeg,
                  double &azDeg);
void altazToRadec(double altDeg, double azDeg, int64_t utcMs, double latDeg, double lonDeg, double &raH,
                  double &decDeg);

// The Sun's apparent RA/Dec of date, to about 0.01° (Astronomical Almanac's
// low-precision formula).
void sunRadec(int64_t utcMs, double &raH, double &decDeg);

// Unit vector for alt/az: x north, y east, z up.
Vec3 vecFromAltaz(double altDeg, double azDeg);
void altazFromVec(const Vec3 &v, double &altDeg, double &azDeg);

Vec3 cross(const Vec3 &a, const Vec3 &b);
double dot(const Vec3 &a, const Vec3 &b);
Vec3 normalize(const Vec3 &v);
double separationDeg(const Vec3 &a, const Vec3 &b);

Vec3 mul(const Mat3 &m, const Vec3 &v);
Mat3 mul(const Mat3 &a, const Mat3 &b);
Mat3 identity();
Mat3 rotZ(double deg);
bool invert(const Mat3 &m, Mat3 &out);  // false if singular

// Civil date to days since 1970-01-01 (proleptic Gregorian).
int64_t daysFromCivil(int y, unsigned m, unsigned d);

}  // namespace core
