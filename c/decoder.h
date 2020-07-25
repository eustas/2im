#ifndef TWIM_DECODER
#define TWIM_DECODER

#include <vector>

#include "image.h"
#include "platform.h"

namespace twim {

class Decoder {
 public:
  static Image decode(std::vector<uint8_t>&& encoded);
};

}  // namespace twim

#endif  // TWIM_DECODER