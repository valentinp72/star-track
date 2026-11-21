// Pointing model: maps the sky's horizontal coordinates (apparent alt/az) to the
// mount's axis angles and back.
//   0 stars: axis angles are taken as alt/az (level base, north unknown).
//   1 star:  an azimuth offset (level base assumed).
//   2+ stars: the rotation that best maps the stars' horizon directions onto the
//            axis directions (Horn's quaternion method). A rotation covers a
//            tilted base and an unknown north, and has 3 parameters only, so
//            centring errors are not amplified across the sky the way a free
//            3×3 matrix (Taki's method) amplifies them.
// A star centred within kSameStarDeg of an earlier one replaces it.
// A sync adds a local correction that fades out 30° away from the synced point.
#pragma once

#include <stdint.h>

#include "astro.h"

namespace core {

struct AlignStar {
  char name[16];
  double ra, dec;          // as sent
  double hAlt, hAz;        // apparent alt/az when added
  double axAlt, axAz;      // axis angles when added
};

class Alignment {
 public:
  static constexpr int kMaxStars = 6;
  static constexpr double kSameStarDeg = 10;

  void clear();
  // Adds a star, or replaces one within kSameStarDeg of it; the oldest is
  // dropped beyond kMaxStars. Returns true if it replaced a star.
  bool add(const AlignStar &s);
  int count() const { return n_; }
  const AlignStar &star(int i) const { return stars_[i]; }
  double residualDeg(int i) const;  // 0 with fewer than 3 stars
  double rmsDeg() const;

  void sync(double hAlt, double hAz, double axAlt, double axAz);
  void clearSync() { hasSync_ = false; }

  void toAxis(double hAlt, double hAz, double &axAlt, double &axAz) const;
  void toHorizon(double axAlt, double axAz, double &hAlt, double &hAz) const;

  // For saving: the stars alone; the model is rebuilt with restore().
  void restore(const AlignStar *stars, int n);

 private:
  void compute();
  void modelToAxis(const Vec3 &h, double &axAlt, double &axAz) const;
  double syncWeight(const Vec3 &h) const;

  AlignStar stars_[kMaxStars];
  int n_ = 0;
  Mat3 t_ = identity();     // horizon vector -> axis vector
  Mat3 tInv_ = identity();
  bool hasSync_ = false;
  Vec3 syncH_{1, 0, 0};
  double syncDAlt_ = 0, syncDAz_ = 0;
};

}  // namespace core
