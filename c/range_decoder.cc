#include "range_decoder.h"

#include "range_code.h"

namespace twim {

uint32_t RangeDecoder::readNumber(RangeDecoder* src, uint32_t max) {
  if (max < 2) {
    return 0;
  }
  uint32_t result = src->currentCount(max);
  src->removeRange(result, result + 1);
  return result;
}

uint32_t RangeDecoder::readSize(RangeDecoder* src) {
  uint32_t plus = 0;
  uint32_t bits = readNumber(src, 8);
  do {
    plus = (plus + 1) << 3u;
    uint32_t extra = readNumber(src, 8);
    bits = (bits << 3u) + extra;
  } while ((readNumber(src, 2) == 1));
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

uint32_t RangeDecoder::readNibble() {
  return (offset < data.size()) ? (data[offset++] & RangeCode::kNibbleMask) : 0;
}

void RangeDecoder::removeRange(uint32_t bottom, uint32_t top) {
  low += bottom * range;
  range *= top - bottom;
  while (true) {
    if ((low ^ (low + range - 1)) >= RangeCode::kHeadStart) {
      if (range > RangeCode::kRangeLimitMask) {
        break;
      }
      range = -low & RangeCode::kValueMask;
    }
    code = ((code << RangeCode::kNibbleBits) & RangeCode::kValueMask) |
           readNibble();
    range = ((range << RangeCode::kNibbleBits) & RangeCode::kValueMask) | RangeCode::kNibbleMask;
    low = (low << RangeCode::kNibbleBits) & RangeCode::kValueMask;
  }
}

uint32_t RangeDecoder::currentCount(uint32_t totalRange) {
  range /= totalRange;
  uint32_t result = (uint32_t)((code - low) / range);
  // corrupted input
  if (result > totalRange) {
    healthy = false;
    return 0;
  }
  return result;
}

}  // namespace twim
