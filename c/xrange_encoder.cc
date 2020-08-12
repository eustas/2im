#include "xrange_encoder.h"

#include <algorithm>

#include "xrange_code.h"

namespace twim {

namespace {

NOINLINE size_t encodeNumber(size_t state, size_t value, size_t max,
                             Array<uint8_t>* bits) {
  size_t low = value * XRangeCode::kSpace;
  size_t base = low / max;
  size_t freq = (low + XRangeCode::kSpace) / max - base;
  while (state >= XRangeCode::kMax * freq / XRangeCode::kSpace) {
    MAYBE_GROW_ARRAY(*bits);
    bits->data[bits->size++] = state & 1u;
    state >>= 1u;
  }
  return ((state / freq) * XRangeCode::kSpace) + (state % freq) + base;
}

}  // namespace

void XRangeEncoder::finish(Array<uint8_t>* out) {
  // First of all, reverse the input.

  std::reverse(entries.data, entries.data + entries.size);
  Array<uint8_t> bits(1024);

  // Calculate the "head" length.
  size_t limit = entries.size;
  {
    double cost = 4.3e9;  // ~2^32
    for (size_t i = 0; i < entries.size; ++i) {
      cost /= entries.data[i].max;
      if (cost < 1.0) {
        limit = i + 1;
        break;
      }
    }
  }

  size_t max_leading_zeros = 0;
  size_t best_initial_state = XRangeCode::kMin;
  for (size_t initial_state = XRangeCode::kMin;
       initial_state < XRangeCode::kMax + 0x1C; initial_state += 32) {
    bits.size = 0;
    size_t state = initial_state;
    for (size_t i = 0; i < limit; ++i) {
      Entry& entry = entries.data[i];
      state = encodeNumber(state, entry.value, entry.max, &bits);
    }
    size_t num_leading_zeros = bits.size;
    for (size_t i = 0; i < bits.size; ++i) {
      if (bits.data[i] != 0) {
        num_leading_zeros = i;
        break;
      }
    }
    if (num_leading_zeros > max_leading_zeros) {
      max_leading_zeros = num_leading_zeros;
      best_initial_state = initial_state;
    }
  }

  {
    bits.size = 0;
    size_t state = best_initial_state;
    for (size_t i = 0; i < entries.size; ++i) {
      Entry& entry = entries.data[i];
      state = encodeNumber(state, entry.value, entry.max, &bits);
    }
    //for (size_t i = 0; i < XRangeCode::kBits; ++i) {
    //  bits.emplace_back(state & 1u);
    //  state >>= 1u;
    //}
    for (size_t i = 0; i < XRangeCode::kBits; ++i) {
      MAYBE_GROW_ARRAY(bits);
      bits.data[bits.size++] = (state >> (XRangeCode::kBits - 1 - i)) & 1u;
    }
  }

  // Now reverse the output. Double reverse -> first bits define first values.
  std::reverse(bits.data, bits.data + bits.size);
  // Remove tail of zeroes.
  while ((bits.size > 0) && (bits.data[bits.size -1] == 0)) {
    bits.size--;
  }

  // But add some padding zeroes back for byte-alignment.
  while ((bits.size & 7u) != 0) {
    MAYBE_GROW_ARRAY(bits);
    bits.data[bits.size++] = 0;
  }

  size_t num_bytes = bits.size >> 3u;
  for (size_t i = 0; i < num_bytes; ++i) {
    uint8_t byte = 0;
    #pragma nounroll
    for (size_t j = 0; j < 8; ++j) byte |= bits.data[8u * i + j] << j;
    MAYBE_GROW_ARRAY(*out);
    out->data[out->size++] = byte;
  }
}

}  // namespace twim
