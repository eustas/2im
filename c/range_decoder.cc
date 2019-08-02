#include "range_decoder.h"

#include <vector>

#include "platform.h"
#include "range_code.h"

namespace twim {

int32_t RangeDecoder::readNumber(RangeDecoder* src, int32_t max) {
  if (max < 2) {
    return 0;
  }
  int32_t result = src->currentCount(max);
  src->removeRange(result, result + 1);
  return result;
}

int32_t RangeDecoder::readSize(RangeDecoder* src) {
  int32_t plus = -1;
  int32_t bits = 0;
  while ((plus <= 0) || (readNumber(src, 2) == 1)) {
    plus = (plus + 1) << 3;
    int32_t extra = readNumber(src, 8);
    bits = (bits << 3) + extra;
  }
  return bits + plus;
}

RangeDecoder::RangeDecoder(std::vector<uint8_t>&& data)
    : data(std::move(data)),
      low(0),
      range(RangeCode::kValueMask),
      code(0),
      offset(0),
      healthy(true) {
  for (size_t i = 0; i < RangeCode::kNumNibbles; ++i) {
    code = (code << RangeCode::kNibbleBits) | readNibble();
  }
}

uint8_t RangeDecoder::readNibble() {
  return (offset < data.size()) ? (data[offset++] & RangeCode::kNibbleMask) : 0;
}

void RangeDecoder::removeRange(int32_t bottom, int32_t top) {
  low += bottom * range;
  range *= top - bottom;
  while (true) {
    if ((low ^ (low + range - 1)) >= RangeCode::kHeadStart) {
      if (range > RangeCode::kRangeLimitMask) {
        break;
      }
      range = -low & RangeCode::kRangeLimitMask;
    }
    code = ((code << RangeCode::kNibbleBits) & RangeCode::kValueMask) |
           readNibble();
    range = (range << RangeCode::kNibbleBits) & RangeCode::kValueMask;
    low = (low << RangeCode::kNibbleBits) & RangeCode::kValueMask;
  }
}

int32_t RangeDecoder::currentCount(int32_t totalRange) {
  range /= totalRange;
  int32_t result = (int32_t)((code - low) / range);
  // corrupted input
  if (result < 0 || result > totalRange) {
    healthy = false;
    return 0;
  }
  return result;
}

}  // namespace twim
