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
    // if (value >= max) __builtin_trap();
    if (max > 1) {
      Array<Entry>& entries = dst->entries;
      MAYBE_GROW_ARRAY(entries);
      Entry& next = entries.data[entries.size++];
      next.value = value;
      next.max = max;
    }
  }

  static float estimateCost(XRangeEncoder* dst) {
    float cost = 0.0f;
    Array<Entry>& entries = dst->entries;
    for (size_t i = 0; i < entries.size; ++i) {
      uint32_t v = entries.data[i].max;
      if (v >= 64) {
        cost += 6;
        v >>= 6;
      }
      cost += SinCos.kLog2[v];
    }
    return cost;
  }

 private:
  struct Entry {
    uint32_t value;
    uint32_t max;
  };

  Array<Entry> entries;
};

}  // namespace twim

#endif  // TWIM_XRANGE_ENCODER
