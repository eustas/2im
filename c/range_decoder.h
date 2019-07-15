#ifndef TWIM_RANGE_DECODER
#define TWIM_RANGE_DECODER

#include <string>
#include <vector>

#include "platform.h"

namespace twim {

class RangeDecoder {
 public:
  RangeDecoder(std::unique_ptr<std::vector<uint8_t>> data);
  void removeRange(int32_t bottom, int32_t top);
  int32_t currentCount(int32_t totalRange);

  static int32_t readNumber(RangeDecoder* src, int32_t max);
  static int32_t readSize(RangeDecoder* src);

 private:
  uint8_t readNibble();

  std::unique_ptr<std::vector<uint8_t>> data;
  int64_t low;
  int64_t range;
  int64_t code;
  int32_t offset;
  // TODO(eustas): add getter?
  bool healthy;
};

}  // namespace twim

#endif  // TWIM_RANGE_DECODER
