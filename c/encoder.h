#ifndef TWIM_ENCODER
#define TWIM_ENCODER

#include "image.h"
#include "platform.h"

namespace twim {

class Encoder {
 public:
  static std::vector<uint8_t> encode(const Image& src, int32_t target_size);
};

}  // namespace twim

#endif  // TWIM_ENCODER
