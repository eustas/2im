#ifndef TWIM_RANGE_CODE
#define TWIM_RANGE_CODE

#include "platform.h"

namespace twim {

class RangeCode {
 public:
  static constexpr const uint32_t kNumNibbles = 6u;
  static constexpr const uint32_t kNibbleBits = 8u;
  static constexpr const uint32_t kNibbleMask = (1u << kNibbleBits) - 1u;
  static constexpr const uint32_t kValueBits = kNumNibbles * kNibbleBits;
  static constexpr const uint64_t kValueMask =
      (UINT64_C(1) << kValueBits) - UINT64_C(1);
  static constexpr const uint32_t kHeadNibbleShift = kValueBits - kNibbleBits;
  static constexpr const uint64_t kHeadStart = UINT64_C(1) << kHeadNibbleShift;
  static constexpr const uint32_t kRangeLimitBits =
      kHeadNibbleShift - kNibbleBits;
  static constexpr const uint64_t kRangeLimitMask =
      (UINT64_C(1) << kRangeLimitBits) - UINT64_C(1);
};

}  // namespace twim

#endif  // TWIM_RANGE_CODE
