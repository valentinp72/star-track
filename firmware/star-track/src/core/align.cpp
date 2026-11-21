#include "align.h"

#include <math.h>
#include <string.h>

namespace core {

namespace {
constexpr double kSyncRadiusDeg = 30.0;

void addOuter(Mat3 &m, const Vec3 &a, const Vec3 &b) {
  const double av[3] = {a.x, a.y, a.z}, bv[3] = {b.x, b.y, b.z};
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) m.m[i][j] += av[i] * bv[j];
}
}  // namespace

void Alignment::clear() {
  n_ = 0;
  hasSync_ = false;
  compute();
}

void Alignment::add(const AlignStar &s) {
  if (n_ == kMaxStars) {
    memmove(&stars_[0], &stars_[1], sizeof(AlignStar) * (kMaxStars - 1));
    n_--;
  }
  stars_[n_++] = s;
  hasSync_ = false;
  compute();
}

void Alignment::restore(const AlignStar *stars, int n) {
  if (n < 0) n = 0;
  if (n > kMaxStars) n = kMaxStars;
  memcpy(stars_, stars, sizeof(AlignStar) * n);
  n_ = n;
  hasSync_ = false;
  compute();
}

void Alignment::compute() {
  if (n_ == 0) {
    t_ = tInv_ = identity();
    return;
  }
  if (n_ == 1) {
    t_ = rotZ(wrap180(stars_[0].axAz - stars_[0].hAz));
    tInv_ = rotZ(-wrap180(stars_[0].axAz - stars_[0].hAz));
    return;
  }
  // T = (Σ t sᵀ)(Σ s sᵀ)⁻¹ over the stars and the normals of each pair of stars.
  // With two stars this is Taki's method (the two stars and their normal).
  Mat3 a{}, b{};
  for (int i = 0; i < n_; i++) {
    Vec3 s = vecFromAltaz(stars_[i].hAlt, stars_[i].hAz);
    Vec3 t = vecFromAltaz(stars_[i].axAlt, stars_[i].axAz);
    addOuter(a, t, s);
    addOuter(b, s, s);
    for (int j = i + 1; j < n_; j++) {
      Vec3 s2 = vecFromAltaz(stars_[j].hAlt, stars_[j].hAz);
      Vec3 t2 = vecFromAltaz(stars_[j].axAlt, stars_[j].axAz);
      Vec3 sn = normalize(cross(s, s2)), tn = normalize(cross(t, t2));
      addOuter(a, tn, sn);
      addOuter(b, sn, sn);
    }
  }
  Mat3 bInv, t, tInv;
  if (!invert(b, bInv)) {  // stars too close together: keep the first star's offset
    t_ = rotZ(wrap180(stars_[0].axAz - stars_[0].hAz));
    tInv_ = rotZ(-wrap180(stars_[0].axAz - stars_[0].hAz));
    return;
  }
  t = mul(a, bInv);
  if (!invert(t, tInv)) return;
  t_ = t;
  tInv_ = tInv;
}

void Alignment::modelToAxis(const Vec3 &h, double &axAlt, double &axAz) const {
  altazFromVec(mul(t_, h), axAlt, axAz);
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
    altazFromVec(mul(tInv_, vecFromAltaz(axAlt - w * syncDAlt_, axAz - w * syncDAz_)), hAlt, hAz);
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

double Alignment::residualDeg(int i) const {
  if (n_ < 3) return 0;
  Vec3 predicted = mul(t_, vecFromAltaz(stars_[i].hAlt, stars_[i].hAz));
  return separationDeg(normalize(predicted), vecFromAltaz(stars_[i].axAlt, stars_[i].axAz));
}

double Alignment::rmsDeg() const {
  if (n_ < 3) return 0;
  double s = 0;
  for (int i = 0; i < n_; i++) s += residualDeg(i) * residualDeg(i);
  return sqrt(s / n_);
}

}  // namespace core
