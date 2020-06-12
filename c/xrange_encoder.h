#ifndef TWIM_XRANGE_ENCODER
#define TWIM_XRANGE_ENCODER

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

 private:
  struct Entry {
    uint32_t value;
    uint32_t max;
  };

  std::vector<Entry> entries;
};

}  // namespace twim

#endif  // TWIM_XRANGE_ENCODER
