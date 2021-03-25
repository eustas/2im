#ifndef TWIM_XRANGE_ENCODER
#define TWIM_XRANGE_ENCODER

#include <cmath>
#include <string>
#include <vector>

#include "sin_cos.h"
#include "platform.h"

namespace twim {

class XRangeEncoder {
 public:
  XRangeEncoder() : entries(1024) {}
  void finish(Array<uint8_t>* out);

  NOINLINE static void writeNumber(XRangeEncoder* dst, uint32_t max, uint32_t value) {
    // if (value >= max || value == 0) __builtin_trap();
    if (max > 1) {
      Array<Entry>& entries = dst->entries;
      MAYBE_GROW_ARRAY(entries);
      Entry& next = entries.data[entries.size++];
      next.value = value;
      next.max = max;
    }
  }

 private:
  friend class XRangeEncoderFriend;
  struct Entry {
    uint32_t value;
    uint32_t max;
  };

  Array<Entry> entries;
};

}  // namespace twim

#endif  // TWIM_XRANGE_ENCODER
