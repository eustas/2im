#include "range_encoder.h"

#include "range_code.h"

namespace twim {

namespace {

struct Decoder {
  const uint8_t* data;
  size_t data_length;
  size_t offset = 0;
  uint64_t code = 0;
  uint64_t low = 0;
  uint64_t range = RangeCode::kValueMask;

  explicit Decoder(const std::vector<uint8_t>& data)
      : data(data.data()), data_length(data.size()) {}

  bool INLINE decodeRange(Triplet t) {
    range /= t.total_range;
    if (range == 0) return false;
    uint32_t count = static_cast<uint32_t>((code - low) / range);
    if ((count < t.bottom) || (count >= t.top)) return false;
    low += t.bottom * range;
    range *= t.top - t.bottom;
    while (true) {
      if ((low ^ (low + range - 1)) >= RangeCode::kHeadStart) {
        if (range > RangeCode::kRangeLimitMask) {
          break;
        }
        range = -low & RangeCode::kValueMask;
      }
      uint64_t nibble = (offset < data_length) ? (data[offset++] & 0xFFu) : 0;
      code =
          ((code << RangeCode::kNibbleBits) & RangeCode::kValueMask) | nibble;
      range = ((range << RangeCode::kNibbleBits) & RangeCode::kValueMask) | RangeCode::kNibbleMask;
      low = (low << RangeCode::kNibbleBits) & RangeCode::kValueMask;
    }
    return true;
  }
};

}  // namespace

void RangeEncoder::writeNumber(RangeEncoder* dst, uint32_t max,
                               uint32_t value) {
  if (max == 1) return;
  dst->encodeRange({value, value + 1, max});
}

void RangeEncoder::writeSize(RangeEncoder* dst, uint32_t value) {
  value -= 8;
  uint32_t chunks = 2;
  while (value > (1u << (chunks * 3))) {
    value -= (1u << (chunks * 3));
    chunks++;
  }
  for (uint32_t i = 0; i < chunks; ++i) {
    if (i > 1) {
      writeNumber(dst, 2, 1);
    }
    writeNumber(dst, 8, (value >> (3 * (chunks - i - 1))) & 7u);
  }
  writeNumber(dst, 2, 0);
}

std::vector<uint8_t> RangeEncoder::encode() {
  std::vector<uint8_t> out;
  uint64_t low = 0;
  uint64_t range = RangeCode::kValueMask;
  for (const Triplet& t : triplets) {
    range /= t.total_range;
    low += t.bottom * range;
    range *= t.top - t.bottom;
    while (true) {
      if ((low ^ (low + range - 1)) >= RangeCode::kHeadStart) {
        if (range > RangeCode::kRangeLimitMask) {
          break;
        }
        range = -low & RangeCode::kValueMask;
      }
      out.emplace_back(low >> RangeCode::kHeadNibbleShift);
      range = ((range << RangeCode::kNibbleBits) & RangeCode::kValueMask) | RangeCode::kNibbleMask;
      low = (low << RangeCode::kNibbleBits) & RangeCode::kValueMask;
    }
  }
  for (size_t i = 0; i < RangeCode::kNumNibbles; ++i) {
    out.emplace_back(low >> RangeCode::kHeadNibbleShift);
    low = (low << RangeCode::kNibbleBits) & RangeCode::kValueMask;
  }
  return out;
}

std::vector<uint8_t> RangeEncoder::optimize(std::vector<uint8_t> data) {
  // KISS
  if (data.size() <= 2 * RangeCode::kNumNibbles) {
    return data;
  }

  Decoder current(data);
  for (size_t i = 0; i < RangeCode::kNumNibbles; ++i) {
    current.code = (current.code << RangeCode::kNibbleBits) |
                   (current.data[current.offset++] & 0xFFu);
  }
  current.range = RangeCode::kValueMask;
  Decoder good = current;

  size_t triplets_size = triplets.size();
  size_t i = 0;
  while (i < triplets_size) {
    current.decodeRange(triplets[i]);
    if (current.offset + 2 * RangeCode::kNumNibbles > data.size()) {
      break;
    }
    good = current;
    i++;
  }

  size_t best_cut = 0;
  int32_t best_cut_delta = 0;
  for (size_t cut = 1; cut <= RangeCode::kNumNibbles; ++cut) {
    good.data_length = data.size() - cut;
    uint8_t original_tail = data[good.data_length - 1];
    for (int32_t delta = -1; delta <= 1; ++delta) {
      current = good;
      data[current.data_length - 1] = (uint8_t)(original_tail + delta);
      size_t j = i;
      bool ok = true;
      while (ok && (j < triplets_size)) {
        ok = current.decodeRange(triplets[j++]);
      }
      if (ok) {
        best_cut = cut;
        best_cut_delta = delta;
      }
    }
    data[current.data_length - 1] = original_tail;
  }
  data.resize(data.size() - best_cut);
  data.back() += best_cut_delta;
  return data;
}

}  // namespace twim
