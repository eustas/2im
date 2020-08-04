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
  std::vector<uint8_t> finish();

  static void writeNumber(XRangeEncoder* dst, uint32_t max, uint32_t value) {
    if (max > 1) dst->entries.push_back({value, max});
  }

  static float estimateCost(XRangeEncoder* dst) {
    float cost = 0.0f;
    for (size_t i = 0; i < dst->entries.size(); ++i) {
      uint32_t v = dst->entries[i].max;
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

  std::vector<Entry> entries;
};

}  // namespace twim

#endif  // TWIM_XRANGE_ENCODER
