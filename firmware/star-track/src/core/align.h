// Pointing model: maps the sky's horizontal coordinates (apparent alt/az) to the
// mount's axis angles and back. Two parts:
//   - a rotation from the horizon to the axes: a tilted base and an unknown
//     north. 3 parameters only, so centring errors are not amplified across
//     the sky the way a free 3×3 matrix (Taki's method) amplifies them;
//   - the altitude index: what the altitude counter reads when the tube is
//     square to the azimuth axis. Not 0 when the tube was not level at power-on.
// From the stars:
//   0 stars: axis angles are taken as alt/az (level base, north unknown).
//   1 star:  an azimuth offset and the index (level base assumed).
//   2+ stars: the index within ±kMaxIndexDeg and the rotation (Horn's
//            quaternion method for each index tried) that fit the stars best.
//            Two stars fit exactly; a third shows the error.
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
  static constexpr double kMaxIndexDeg = 10;

  void clear();
  // Adds a star, or replaces one within kSameStarDeg of it; the oldest is
  // dropped beyond kMaxStars. Returns true if it replaced a star.
  bool add(const AlignStar &s);
  int count() const { return n_; }
  const AlignStar &star(int i) const { return stars_[i]; }
  double residualDeg(int i) const;  // 0 with fewer than 3 stars
  double rmsDeg() const;
  double fitErrorDeg(int i) const;  // how far star i is from the model (also with 2 stars)
  double altIndexDeg() const { return index_; }

  void sync(double hAlt, double hAz, double axAlt, double axAz);
  void clearSync() { hasSync_ = false; }

  void toAxis(double hAlt, double hAz, double &axAlt, double &axAz) const;
  void toHorizon(double axAlt, double axAz, double &hAlt, double &hAz) const;

  // For saving: the stars alone; the model is rebuilt with restore().
  void restore(const AlignStar *stars, int n);

 private:
  void compute();
  Mat3 rotationFor(double index, double *cost) const;
  void modelToAxis(const Vec3 &h, double &axAlt, double &axAz) const;
  double syncWeight(const Vec3 &h) const;

  AlignStar stars_[kMaxStars];
  int n_ = 0;
  Mat3 t_ = identity();     // horizon vector -> axis vector (axis altitude without the index)
  Mat3 tInv_ = identity();
  double index_ = 0;        // altitude counter reading minus the axis altitude
  bool hasSync_ = false;
  Vec3 syncH_{1, 0, 0};
  double syncDAlt_ = 0, syncDAz_ = 0;
};

}  // namespace core
