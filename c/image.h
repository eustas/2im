#ifndef TWIM_IMAGE
#define TWIM_IMAGE

#include <memory>
#include <string>
#include <vector>

#include "platform.h"

namespace twim {

struct Image {
  size_t width = 0;
  size_t height = 0;
  std::vector<uint8_t> r;
  std::vector<uint8_t> g;
  std::vector<uint8_t> b;
};

}  // namespace twim

#endif  // TWIM_IMAGE
