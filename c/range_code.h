#ifndef TWIM_RANGE_CODE
#define TWIM_RANGE_CODE

#include "platform.h"

namespace twim {

class RangeCode {
 public:
  static constexpr const int32_t kNumNibbles = 6;
  static constexpr const int32_t kNibbleBits = 8;
  static constexpr const int32_t kNibbleMask = (1 << kNibbleBits) - 1;
  static constexpr const int32_t kValueBits = kNumNibbles * kNibbleBits;
  static constexpr const int64_t kValueMask =
      (INT64_C(1) << kValueBits) - INT64_C(1);
  static constexpr const int32_t kHeadNibbleShift = kValueBits - kNibbleBits;
  static constexpr const int64_t kHeadStart = INT64_C(1) << kHeadNibbleShift;
  static constexpr const int32_t kRangeLimitBits =
      kHeadNibbleShift - kNibbleBits;
  static constexpr const int64_t kRangeLimitMask =
      (INT64_C(1) << kRangeLimitBits) - 1L;
};

}  // namespace twim

#endif  // TWIM_RANGE_ENCODER
