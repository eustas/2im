#ifndef TWIM_XRANGE_CODE
#define TWIM_XRANGE_CODE

#include "platform.h"

namespace twim {

class XRangeCode {
 public:
  static constexpr size_t kSpace = 1u << 11u;
  static constexpr size_t kMask = kSpace - 1u;
  static constexpr size_t kBits = 16u;
  static constexpr size_t kMin = 1u << kBits;
  static constexpr size_t kMax = 2u * kMin;
};

}  // namespace twim

#endif  // TWIM_XRANGE_ENCODER
