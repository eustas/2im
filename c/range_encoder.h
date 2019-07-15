#ifndef TWIM_RANGE_ENCODER
#define TWIM_RANGE_ENCODER

#include <string>
#include <vector>

#include "platform.h"

namespace twim {

struct Triplet {
  int32_t bottom;
  int32_t top;
  int32_t total_range;
};

class RangeEncoder {
 public:
  void INLINE encodeRange(Triplet triplet) { triplets.emplace_back(triplet); }

  std::vector<uint8_t> finish() { return optimize(encode()); }

  static void writeNumber(RangeEncoder* dst, int32_t max, int32_t value);

  static void writeSize(RangeEncoder* dst, int32_t value);

 private:
  std::vector<uint8_t> encode();
  std::vector<uint8_t> optimize(std::vector<uint8_t> data);

  std::vector<Triplet> triplets;
};

}  // namespace twim

#endif  // TWIM_RANGE_ENCODER
