#include "sin_cos.h"

#include <cmath>

namespace twim {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

typedef std::array<int32_t, SinCos::kMaxAngle> LutI;
typedef std::array<double, SinCos::kMaxAngle> LutD;
typedef std::array<float, SinCos::kMaxAngle> LutF;

const double kPi = std::acos(-1);

const LutI SinCos::kSin = []() -> LutI {
  LutI result;
  for (int32_t i = 0; i < SinCos::kMaxAngle; ++i) {
    result[i] = static_cast<int32_t>(
        std::round(SinCos::kOne * std::sin((kPi * i) / SinCos::kMaxAngle)));
  }
  return result;
}();

const LutD SinCos::kInvSin = []() -> LutD {
  LutD result;
  result[0] = 0.0;
  for (int32_t i = 1; i < SinCos::kMaxAngle; ++i) {
    result[i] = 1.0 / kSin[i];
  }
  return result;
}();

const LutI SinCos::kCos = []() -> LutI {
  LutI result;
  for (int32_t i = 0; i < SinCos::kMaxAngle; ++i) {
    result[i] = static_cast<int32_t>(
        std::round(SinCos::kOne * std::cos((kPi * i) / SinCos::kMaxAngle)));
  }
  return result;
}();

const LutF SinCos::kMinusCot = []() -> LutF {
  LutF result;
  for (int32_t i = 0; i < SinCos::kMaxAngle; ++i) {
    result[i] = -kCos[i] * kInvSin[i];
  }
  return result;
}();

#pragma clang diagnostic pop
}  // namespace twim
