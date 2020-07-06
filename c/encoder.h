#ifndef TWIM_ENCODER
#define TWIM_ENCODER

#include <vector>

#include "image.h"
#include "platform.h"

namespace twim {

namespace Encoder {

struct Variant {
  uint32_t partitionCode : 9;
  uint32_t lineLimit : 6;
  uint64_t colorOptions : 49;
};

struct Params {
  uint32_t targetSize;
  uint32_t numThreads;
  std::vector<Variant> variants;
};

struct Result {
  std::vector<uint8_t> data;
  Variant variant;
  float mse;
};

template <typename EntropyEncoder>
Result encode(const Image& src, const Params& params);

}  // namespace Encoder

}  // namespace twim

#endif  // TWIM_ENCODER
