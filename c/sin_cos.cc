#include "sin_cos.h"

#include <cmath>

namespace twim {

static SinCosT makeSinCos() {
  SinCosT result;

  double sx = 0.006135884649154475;
  double cx = 0.999981175282601109;
  double s = 0.0;
  double c = 1.0;
  const int32_t kOne = result.kOne;
  const int32_t kMaxAngle = result.kMaxAngle;
  auto& kSin = result.kSin;
  auto& kCos = result.kCos;
  for (int32_t i = 0; i < kMaxAngle; ++i) {
    kSin[i] = s * kOne + 0.5;
    kCos[i] = c * kOne + (c < 0 ? -0.5 : 0.5);
    double is = (i != 0) ? (1.0 / kSin[i]) : 0.0;
    result.kInvSin[i] = is;
    result.kMinusCot[i] = -kCos[i] * is;
    double tmp = s * cx + c * sx;
    c = c * cx - s * sx;
    s = tmp;
  }

  auto& kLog2 = result.kLog2;
  for (size_t j = 1; j < kLog2.size(); ++j) {
    double v = j;
    double r = 0.0;
    for (double plus = 1.0; plus > 1e-8;) {
      if (v >= 2.0) {
        v /= 2.0;
        r += plus;
      } else {
        v = v * v;
        plus = plus / 2.0;
      }
    }
    kLog2[j] = r;
  }

  return result;
}

const SinCosT SinCos = makeSinCos();

}  // namespace twim
