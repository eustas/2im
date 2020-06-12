#include "xrange_encoder.h"

#include "xrange_code.h"

namespace twim {

namespace {

size_t encodeNumber(size_t state, size_t value, size_t max,
                    std::vector<uint8_t>* bits) {
  size_t low = value * XRangeCode::kSpace;
  size_t base = low / max;
  size_t freq = (low + XRangeCode::kSpace) / max - base;
  while (state >= XRangeCode::kMax * freq / XRangeCode::kSpace) {
    bits->emplace_back(state & 1u);
    state >>= 1u;
  }
  return ((state / freq) * XRangeCode::kSpace) + (state % freq) + base;
}

}  // namespace

std::vector<uint8_t> XRangeEncoder::finish() {
  // First of all, reverse the input.
  std::reverse(entries.begin(), entries.end());
  std::vector<uint8_t> bits;

  // Calculate the "head" length.
  size_t limit = entries.size();
  {
    double cost = 4.3e9;  // ~2^32
    for (size_t i = 0; i < entries.size(); ++i) {
      cost /= entries[i].max;
      if (cost < 1.0) {
        limit = i + 1;
        break;
      }
    }
  }

  size_t max_leading_zeros = 0;
  size_t best_initial_state = XRangeCode::kMin;
  for (size_t initial_state = XRangeCode::kMin;
       initial_state < XRangeCode::kMax; ++initial_state) {
    bits.clear();
    size_t state = initial_state;
    for (size_t i = 0; i < limit; ++i) {
      state = encodeNumber(state, entries[i].value, entries[i].max, &bits);
    }
    size_t num_leading_zeros = bits.size();
    for (size_t i = 0; i < bits.size(); ++i) {
      if (bits[i] != 0) {
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
    bits.clear();
    size_t state = best_initial_state;
    for (auto & entry : entries) {
      state = encodeNumber(state, entry.value, entry.max, &bits);
    }
    for (size_t i = 0; i < XRangeCode::kBits; ++i) {
      bits.emplace_back(state & 1u);
      state >>= 1u;
    }
  }

  // Now reverse the output. Double reverse -> first bits define first values.
  std::reverse(bits.begin(), bits.end());
  // Remove tail of zeroes.
  while (!bits.empty() && (bits.back() == 0)) {
    bits.pop_back();
  }

  // But add some padding zeroes back for byte-alignment.
  while ((bits.size() & 7u) != 0) {
    bits.emplace_back(0);
  }

  size_t num_bytes = bits.size() >> 3u;
  std::vector<uint8_t> bytes(num_bytes);
  for (size_t i = 0; i < num_bytes; ++i) {
    uint8_t byte = 0;
    for (size_t j = 0; j < 8; ++j) byte |= bits[8u * i + j] << j;
    bytes[i] = byte;
  }
  return bytes;
}

}  // namespace twim
