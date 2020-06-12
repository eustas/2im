#ifndef TWIM_XRANGE_DECODER
#define TWIM_XRANGE_DECODER

#include <vector>

#include "platform.h"

namespace twim {

class XRangeDecoder {
 public:
  explicit XRangeDecoder(std::vector<uint8_t>&& data);

  static uint32_t readNumber(XRangeDecoder* src, size_t max);

 private:
  size_t readBit();

  std::vector<uint8_t> data;
  size_t state;
  size_t pos;
};

}  // namespace twim

#endif  // TWIM_XRANGE_DECODER
