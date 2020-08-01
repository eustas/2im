#ifndef TWIM_XRANGE_ENCODER
#define TWIM_XRANGE_ENCODER

#include <cmath>
#include <string>
#include <vector>

#include "platform.h"

namespace twim {

class XRangeEncoder {
 public:
  std::vector<uint8_t> finish();

  static void writeNumber(XRangeEncoder* dst, uint32_t max, uint32_t value) {
    if (max > 1) dst->entries.push_back({value, max});
  }

  static double estimateCost(XRangeEncoder* dst) {
    double cost = 0.0;
    for (size_t i = 0; i < dst->entries.size(); ++i) {
      cost += std::log(dst->entries[i].max);
    }
    return cost / std::log(2.0);
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
