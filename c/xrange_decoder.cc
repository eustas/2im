#include "xrange_decoder.h"

#include "xrange_code.h"

namespace twim {

uint32_t XRangeDecoder::readNumber(XRangeDecoder* src, size_t max) {
  if (max == 1) return 0;
  size_t state = src->state;
  size_t offset = state & XRangeCode::kMask;
  size_t result = (offset * max + max - 1) / XRangeCode::kSpace;
  size_t low = result * XRangeCode::kSpace;
  size_t base = low / max;
  size_t freq = (low + XRangeCode::kSpace) / max - base;
  state = freq * (state / XRangeCode::kSpace) + offset - base;
  while (state < XRangeCode::kMin) state = (state << 1u) | src->readBit();
  src->state = state;
  return result;
}

XRangeDecoder::XRangeDecoder(std::vector<uint8_t>&& data)
    : data(std::move(data)),
      state(1u),
      pos(0) {
  for (size_t i = 0; i < XRangeCode::kBits; ++i) {
    state = (state << 1u) | readBit();
  }
}

size_t XRangeDecoder::readBit() {
  size_t offset = pos >> 3u;
  if (offset >= data.size()) return 0;
  return (data[offset] >> (pos++ & 7u)) & 1u;
}

}  // namespace twim
