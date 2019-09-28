#ifndef TWIM_ENCODER
#define TWIM_ENCODER

#include <vector>

#include "image.h"
#include "platform.h"

namespace twim {

namespace Encoder {

std::vector<uint8_t> encode(const Image& src, int32_t target_size);

}  // namespace Encoder

}  // namespace twim

#endif  // TWIM_ENCODER
