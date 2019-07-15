#include "sincos.h"

#include <cmath>

namespace twim {

typedef std::array<int, SinCos::kMaxAngle> Lut;

const double kPi = std::acos(-1);

const Lut SinCos::kSin = []() -> Lut {
  Lut result;
  for (int32_t i = 0; i < SinCos::kMaxAngle; ++i) {
    result[i] =
        std::round(SinCos::kOne * std::sin((kPi * i) / SinCos::kMaxAngle));
  }
  return result;
}();

const Lut SinCos::kCos = []() -> Lut {
  Lut result;
  for (int32_t i = 0; i < SinCos::kMaxAngle; ++i) {
    result[i] =
        std::round(SinCos::kOne * std::cos((kPi * i) / SinCos::kMaxAngle));
  }
  return result;
}();

}  // namespace twim
