#ifndef TWIM_SIN_COS
#define TWIM_SIN_COS

#include <array>

#include "platform.h"

namespace twim {

class SinCos {
 public:
  static constexpr const int32_t kOne = 1u << 18u;
  static constexpr const uint32_t kMaxAngleBits = 9u;
  static constexpr const int32_t kMaxAngle = 1u << kMaxAngleBits;

  static const std::array<int32_t, kMaxAngle> kSin;
  static const std::array<double, kMaxAngle> kInvSin;
  static const std::array<int32_t, kMaxAngle> kCos;
  static const std::array<float, kMaxAngle> kMinusCot;
};

}  // namespace twim

#endif  // TWIM_SIN_COS
