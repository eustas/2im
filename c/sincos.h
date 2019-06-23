#ifndef TWIM_SINCOS
#define TWIM_SINCOS

#include <array>

namespace twim {

constexpr const int kOne = 1 << 18;

constexpr const int kMaxAngleBits = 9;

constexpr const int kMaxAngle = 1 << kMaxAngleBits;

extern const std::array<int, kMaxAngle> kSin;
extern const std::array<int, kMaxAngle> kCos;

}  // namespace twim

#endif  // TWIM_SINCOS
