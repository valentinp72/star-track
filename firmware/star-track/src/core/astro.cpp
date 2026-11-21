#include "astro.h"

#include <math.h>

namespace core {

double wrap360(double deg) {
  double r = fmod(deg, 360.0);
  if (r < 0) r += 360.0;
  if (r >= 360.0) r -= 360.0;
  return r;
}

double wrap180(double deg) {
  double r = wrap360(deg);
  return r > 180.0 ? r - 360.0 : r;
}

double wrap24(double hours) {
  double r = fmod(hours, 24.0);
  if (r < 0) r += 24.0;
  if (r >= 24.0) r -= 24.0;
  return r;
}

double julianDate(int64_t utcMs) { return (double)utcMs / 86400000.0 + 2440587.5; }

double gmstDeg(int64_t utcMs) {
  // Split the day count so the large constant does not eat the precision.
  const int64_t j2000Ms = 946728000000LL;  // 2000-01-01 12:00 UTC
  double d = (double)(utcMs - j2000Ms) / 86400000.0;
  double t = d / 36525.0;
  double theta = 280.46061837 + 360.98564736629 * d + 0.000387933 * t * t - t * t * t / 38710000.0;
  return wrap360(theta);
}

double lstDeg(int64_t utcMs, double lonDeg) { return wrap360(gmstDeg(utcMs) + lonDeg); }

void equToHor(double raH, double decDeg, double lst, double latDeg, double &altDeg, double &azDeg) {
  double h = (lst - raH * 15.0) * kDeg;
  double d = decDeg * kDeg, p = latDeg * kDeg;
  double sinAlt = sin(p) * sin(d) + cos(p) * cos(d) * cos(h);
  if (sinAlt > 1) sinAlt = 1;
  if (sinAlt < -1) sinAlt = -1;
  altDeg = asin(sinAlt) / kDeg;
  azDeg = wrap360(atan2(-cos(d) * sin(h), sin(d) * cos(p) - cos(d) * cos(h) * sin(p)) / kDeg);
}

void horToEqu(double altDeg, double azDeg, double lst, double latDeg, double &raH, double &decDeg) {
  double a = altDeg * kDeg, z = azDeg * kDeg, p = latDeg * kDeg;
  double sinDec = sin(p) * sin(a) + cos(p) * cos(a) * cos(z);
  if (sinDec > 1) sinDec = 1;
  if (sinDec < -1) sinDec = -1;
  decDeg = asin(sinDec) / kDeg;
  double h = atan2(-cos(a) * sin(z), sin(a) * cos(p) - cos(a) * cos(z) * sin(p)) / kDeg;
  raH = wrap24((lst - h) / 15.0);
}

double refractionFromTrue(double h) {
  if (h < -1.0) h = -1.0;
  if (h > 89.9) return 0;
  return 1.02 / tan((h + 10.3 / (h + 5.11)) * kDeg) / 60.0;
}

double refractionFromApparent(double h) {
  if (h < -1.0) h = -1.0;
  if (h > 89.9) return 0;
  return 1.0 / tan((h + 7.31 / (h + 4.4)) * kDeg) / 60.0;
}

void radecToAltaz(double raH, double decDeg, int64_t utcMs, double latDeg, double lonDeg, double &altDeg,
                  double &azDeg) {
  equToHor(raH, decDeg, lstDeg(utcMs, lonDeg), latDeg, altDeg, azDeg);
  altDeg += refractionFromTrue(altDeg);
}

void altazToRadec(double altDeg, double azDeg, int64_t utcMs, double latDeg, double lonDeg, double &raH,
                  double &decDeg) {
  double trueAlt = altDeg - refractionFromApparent(altDeg);
  horToEqu(trueAlt, azDeg, lstDeg(utcMs, lonDeg), latDeg, raH, decDeg);
}

Vec3 vecFromAltaz(double altDeg, double azDeg) {
  double a = altDeg * kDeg, z = azDeg * kDeg;
  return {cos(a) * cos(z), cos(a) * sin(z), sin(a)};
}

void altazFromVec(const Vec3 &v0, double &altDeg, double &azDeg) {
  Vec3 v = normalize(v0);
  double s = v.z;
  if (s > 1) s = 1;
  if (s < -1) s = -1;
  altDeg = asin(s) / kDeg;
  azDeg = wrap360(atan2(v.y, v.x) / kDeg);
}

Vec3 cross(const Vec3 &a, const Vec3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 normalize(const Vec3 &v) {
  double n = sqrt(dot(v, v));
  if (n == 0) return {1, 0, 0};
  return {v.x / n, v.y / n, v.z / n};
}

double separationDeg(const Vec3 &a, const Vec3 &b) {
  // atan2 of |a×b| and a·b stays accurate for small angles.
  Vec3 c = cross(a, b);
  return atan2(sqrt(dot(c, c)), dot(a, b)) / kDeg;
}

Vec3 mul(const Mat3 &m, const Vec3 &v) {
  return {m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z,
          m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z,
          m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z};
}

Mat3 mul(const Mat3 &a, const Mat3 &b) {
  Mat3 r{};
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 3; k++) r.m[i][j] += a.m[i][k] * b.m[k][j];
  return r;
}

Mat3 identity() { return {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}; }

Mat3 rotZ(double deg) {
  double c = cos(deg * kDeg), s = sin(deg * kDeg);
  return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

bool invert(const Mat3 &a, Mat3 &out) {
  const double(*m)[3] = a.m;
  double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
               m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
               m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  if (fabs(det) < 1e-12) return false;
  double k = 1.0 / det;
  out.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * k;
  out.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * k;
  out.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * k;
  out.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * k;
  out.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * k;
  out.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * k;
  out.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * k;
  out.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * k;
  out.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * k;
  return true;
}

int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  // Howard Hinnant's algorithm.
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

}  // namespace core
