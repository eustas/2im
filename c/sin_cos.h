#ifndef TWIM_SIN_COS
#define TWIM_SIN_COS

#include <array>

#include "platform.h"

namespace twim {

class SinCosT {
 public:
  static constexpr const int32_t kOne = 1u << 18u;
  static constexpr const uint32_t kMaxAngleBits = 9u;
  static constexpr const int32_t kMaxAngle = 1u << kMaxAngleBits;

  // round(kOne * sin((kPi * i) / kMaxAngle));
  std::array<int32_t, kMaxAngle> kSin;
  // round(kOne * cos((kPi * i) / kMaxAngle));
  std::array<int32_t, kMaxAngle> kCos;
  // 1.0 / kSin[i]
  std::array<double, kMaxAngle> kInvSin;
  // -kCos[i] / kSin[i]
  std::array<float, kMaxAngle> kMinusCot;
  // log(i) / log(2)
  std::array<float, 2049> kLog2;
  // 2**(i / 3.0)
  std::array<uint16_t, 34> kPow2;
};

extern const SinCosT SinCos;

}  // namespace twim

#endif  // TWIM_SIN_COS
