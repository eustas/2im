#ifndef TWIM_IMAGE
#define TWIM_IMAGE

#include <memory>
#include <string>
#include <vector>

#include "platform.h"

namespace twim {

struct Image {
  uint32_t width = 0;
  uint32_t height = 0;
  uint8_t* r = nullptr;
  uint8_t* g = nullptr;
  uint8_t* b = nullptr;
  bool ok = false;

  std::vector<uint8_t> r_;
  std::vector<uint8_t> g_;
  std::vector<uint8_t> b_;

  void init(uint32_t width, uint32_t height);

  static Image fromRgba(const uint8_t* src, uint32_t width, uint32_t height);
};

}  // namespace twim

#endif  // TWIM_IMAGE
