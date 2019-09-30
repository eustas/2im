#ifndef TWIM_RANGE_DECODER
#define TWIM_RANGE_DECODER

#include <vector>

#include "platform.h"

namespace twim {

class RangeDecoder {
 public:
  explicit RangeDecoder(std::vector<uint8_t>&& data);
  void removeRange(uint32_t bottom, uint32_t top);
  uint32_t currentCount(uint32_t totalRange);

  static uint32_t readNumber(RangeDecoder* src, uint32_t max);
  static uint32_t readSize(RangeDecoder* src);

 private:
  uint32_t readNibble();

  std::vector<uint8_t> data;
  uint64_t low;
  uint64_t range;
  uint64_t code;
  size_t offset;
  // TODO(eustas): add getter?
  bool healthy;
};

}  // namespace twim

#endif  // TWIM_RANGE_DECODER
