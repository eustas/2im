#include "sincos.h"

#include <cmath>

namespace twim {

typedef std::array<int, kMaxAngle> Lut;

const float kPi = std::acos(-1);

const Lut kSin = []() -> Lut {
  Lut result;
  for (int i = 0; i < kMaxAngle; ++i) {
    result[i] = std::round(kOne * std::sin((kPi * i) / kMaxAngle));
  }
  return result;
}();

const Lut kCos = []() -> Lut {
  Lut result;
  for (int i = 0; i < kMaxAngle; ++i) {
    result[i] = std::round(kOne * std::cos((kPi * i) / kMaxAngle));
  }
  return result;
}();

}  // namespace twim
