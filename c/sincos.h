#ifndef TWIM_SINCOS
#define TWIM_SINCOS

#include <array>

#include "platform.h"

namespace twim {

class SinCos {
 public:
  static constexpr const int32_t kOne = 1 << 18;
  static constexpr const int32_t kMaxAngleBits = 9;
  static constexpr const int32_t kMaxAngle = 1 << kMaxAngleBits;

  static const std::array<int32_t, kMaxAngle> kSin;
  static const std::array<int32_t, kMaxAngle> kCos;
};

}  // namespace twim

#endif  // TWIM_SINCOS
