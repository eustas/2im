#ifndef TWIM_ENCODER
#define TWIM_ENCODER

#include <vector>

#include "image.h"
#include "platform.h"

namespace twim {

namespace Encoder {

struct Params {
  uint32_t targetSize;
  uint32_t numThreads;
};

template<typename EntropyEncoder>
std::vector<uint8_t> encode(const Image& src, const Params& params);

}  // namespace Encoder

}  // namespace twim

#endif  // TWIM_ENCODER
