#include "align.h"

#include <math.h>
#include <string.h>

namespace core {

namespace {
constexpr double kSyncRadiusDeg = 30.0;

// Eigenvector of the largest eigenvalue of a symmetric 4×4 matrix (cyclic Jacobi).
void largestEigenvector(double a[4][4], double out[4]) {
  double v[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
  for (int sweep = 0; sweep < 50; sweep++) {
    double off = 0;
    for (int p = 0; p < 4; p++)
      for (int q = p + 1; q < 4; q++) off += a[p][q] * a[p][q];
    if (off < 1e-24) break;
    for (int p = 0; p < 4; p++)
      for (int q = p + 1; q < 4; q++) {
        if (fabs(a[p][q]) < 1e-30) continue;
        double theta = (a[q][q] - a[p][p]) / (2 * a[p][q]);
        double t = (theta >= 0 ? 1 : -1) / (fabs(theta) + sqrt(theta * theta + 1));
        double c = 1 / sqrt(t * t + 1), s = t * c;
        for (int k = 0; k < 4; k++) {  // columns p, q
          double akp = a[k][p], akq = a[k][q];
          a[k][p] = c * akp - s * akq;
          a[k][q] = s * akp + c * akq;
        }
        for (int k = 0; k < 4; k++) {  // rows p, q
          double apk = a[p][k], aqk = a[q][k];
          a[p][k] = c * apk - s * aqk;
          a[q][k] = s * apk + c * aqk;
        }
        for (int k = 0; k < 4; k++) {
          double vkp = v[k][p], vkq = v[k][q];
          v[k][p] = c * vkp - s * vkq;
          v[k][q] = s * vkp + c * vkq;
        }
      }
  }
  int best = 0;
  for (int i = 1; i < 4; i++)
    if (a[i][i] > a[best][best]) best = i;
  for (int k = 0; k < 4; k++) out[k] = v[k][best];
}

// The rotation R minimising Σ |t_i − R s_i|² (Horn, 1987).
Mat3 bestRotation(const Vec3 *s, const Vec3 *t, int n) {
  double sxx = 0, sxy = 0, sxz = 0, syx = 0, syy = 0, syz = 0, szx = 0, szy = 0, szz = 0;
  for (int i = 0; i < n; i++) {
    sxx += s[i].x * t[i].x, sxy += s[i].x * t[i].y, sxz += s[i].x * t[i].z;
    syx += s[i].y * t[i].x, syy += s[i].y * t[i].y, syz += s[i].y * t[i].z;
    szx += s[i].z * t[i].x, szy += s[i].z * t[i].y, szz += s[i].z * t[i].z;
  }
  double nm[4][4] = {{sxx + syy + szz, syz - szy, szx - sxz, sxy - syx},
                     {syz - szy, sxx - syy - szz, sxy + syx, szx + sxz},
                     {szx - sxz, sxy + syx, -sxx + syy - szz, syz + szy},
                     {sxy - syx, szx + sxz, syz + szy, -sxx - syy + szz}};
  double q[4];
  largestEigenvector(nm, q);
  double w = q[0], x = q[1], y = q[2], z = q[3];
  return {{{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
           {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
           {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}}};
}
}  // namespace

void Alignment::clear() {
  n_ = 0;
  hasSync_ = false;
  compute();
}

bool Alignment::add(const AlignStar &s) {
  Vec3 h = vecFromAltaz(s.hAlt, s.hAz);
  for (int i = 0; i < n_; i++) {
    if (separationDeg(h, vecFromAltaz(stars_[i].hAlt, stars_[i].hAz)) < kSameStarDeg) {
      stars_[i] = s;
      hasSync_ = false;
      compute();
      return true;
    }
  }
  if (n_ == kMaxStars) {
    memmove(&stars_[0], &stars_[1], sizeof(AlignStar) * (kMaxStars - 1));
    n_--;
  }
  stars_[n_++] = s;
  hasSync_ = false;
  compute();
  return false;
}

void Alignment::restore(const AlignStar *stars, int n) {
  if (n < 0) n = 0;
  if (n > kMaxStars) n = kMaxStars;
  memcpy(stars_, stars, sizeof(AlignStar) * n);
  n_ = n;
  hasSync_ = false;
  compute();
}

// The best rotation with this altitude index, and its cost: the sum of the
// squared distances (unit vectors) between the stars' axis directions and the
// rotated horizon directions.
Mat3 Alignment::rotationFor(double index, double *cost) const {
  Vec3 sv[kMaxStars], tv[kMaxStars];
  for (int i = 0; i < n_; i++) {
    sv[i] = vecFromAltaz(stars_[i].hAlt, stars_[i].hAz);
    tv[i] = vecFromAltaz(stars_[i].axAlt - index, stars_[i].axAz);
  }
  Mat3 r = bestRotation(sv, tv, n_);
  if (cost) {
    double c = 0;
    for (int i = 0; i < n_; i++) {
      Vec3 p = mul(r, sv[i]);
      double dx = p.x - tv[i].x, dy = p.y - tv[i].y, dz = p.z - tv[i].z;
      c += dx * dx + dy * dy + dz * dz;
    }
    *cost = c;
  }
  return r;
}

void Alignment::compute() {
  if (n_ == 0) {
    t_ = tInv_ = identity();
    index_ = 0;
    return;
  }
  if (n_ == 1) {
    // Level base assumed: the star's altitude gives the index, its azimuth north.
    index_ = stars_[0].axAlt - stars_[0].hAlt;
    t_ = rotZ(wrap180(stars_[0].axAz - stars_[0].hAz));
    tInv_ = rotZ(-wrap180(stars_[0].axAz - stars_[0].hAz));
    return;
  }
  // The index: a scan every 0.25°, then a golden-section search around the
  // best point. Among equal minima (two stars can fit exactly at two indexes),
  // the one closest to 0.
  const double step = 0.25;
  double best = 0, bestCost = 1e30;
  for (double d = -kMaxIndexDeg; d <= kMaxIndexDeg + 1e-9; d += step) {
    double c;
    rotationFor(d, &c);
    if (c < bestCost - 1e-12 || (fabs(c - bestCost) <= 1e-12 && fabs(d) < fabs(best))) {
      bestCost = c;
      best = d;
    }
  }
  double lo = fmax(best - step, -kMaxIndexDeg), hi = fmin(best + step, kMaxIndexDeg);
  const double g = 0.6180339887498949;
  double x1 = hi - g * (hi - lo), x2 = lo + g * (hi - lo), c1, c2;
  rotationFor(x1, &c1);
  rotationFor(x2, &c2);
  for (int k = 0; k < 40; k++) {
    if (c1 < c2) {
      hi = x2, x2 = x1, c2 = c1, x1 = hi - g * (hi - lo);
      rotationFor(x1, &c1);
    } else {
      lo = x1, x1 = x2, c1 = c2, x2 = lo + g * (hi - lo);
      rotationFor(x2, &c2);
    }
  }
  index_ = 0.5 * (lo + hi);
  t_ = rotationFor(index_, nullptr);
  // The inverse of a rotation is its transpose.
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) tInv_.m[i][j] = t_.m[j][i];
}

void Alignment::modelToAxis(const Vec3 &h, double &axAlt, double &axAz) const {
  altazFromVec(mul(t_, h), axAlt, axAz);
  axAlt += index_;
}

double Alignment::syncWeight(const Vec3 &h) const {
  if (!hasSync_) return 0;
  double d = separationDeg(h, syncH_);
  return d >= kSyncRadiusDeg ? 0 : 1.0 - d / kSyncRadiusDeg;
}

void Alignment::toAxis(double hAlt, double hAz, double &axAlt, double &axAz) const {
  Vec3 h = vecFromAltaz(hAlt, hAz);
  modelToAxis(h, axAlt, axAz);
  double w = syncWeight(h);
  axAlt += w * syncDAlt_;
  axAz = wrap360(axAz + w * syncDAz_);
}

void Alignment::toHorizon(double axAlt, double axAz, double &hAlt, double &hAz) const {
  // Remove the sync correction (weight from a first estimate), then invert the model.
  for (int pass = 0, done = 0; !done; pass++) {
    double w = 0;
    if (hasSync_ && pass > 0) w = syncWeight(vecFromAltaz(hAlt, hAz));
    altazFromVec(mul(tInv_, vecFromAltaz(axAlt - w * syncDAlt_ - index_, axAz - w * syncDAz_)), hAlt, hAz);
    done = !hasSync_ || pass >= 2;
  }
}

void Alignment::sync(double hAlt, double hAz, double axAlt, double axAz) {
  hasSync_ = false;
  double pAlt, pAz;
  Vec3 h = vecFromAltaz(hAlt, hAz);
  modelToAxis(h, pAlt, pAz);
  syncH_ = h;
  syncDAlt_ = axAlt - pAlt;
  syncDAz_ = wrap180(axAz - pAz);
  hasSync_ = true;
}

double Alignment::fitErrorDeg(int i) const {
  Vec3 predicted = mul(t_, vecFromAltaz(stars_[i].hAlt, stars_[i].hAz));
  return separationDeg(normalize(predicted), vecFromAltaz(stars_[i].axAlt - index_, stars_[i].axAz));
}

double Alignment::residualDeg(int i) const { return n_ < 3 ? 0 : fitErrorDeg(i); }

double Alignment::rmsDeg() const {
  if (n_ < 3) return 0;
  double s = 0;
  for (int i = 0; i < n_; i++) s += residualDeg(i) * residualDeg(i);
  return sqrt(s / n_);
}

}  // namespace core
