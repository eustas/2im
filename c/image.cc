#include "image.h"

namespace twim {

Image::~Image() {
  if (this->r != nullptr) free(this->r);
  if (this->g != nullptr) free(this->g);
  if (this->b != nullptr) free(this->b);
}

void Image::init(uint32_t width, uint32_t height) {

  this->width = width;
  this->height = height;

  this->r = static_cast<uint8_t*>(malloc(width * height));
  this->g = static_cast<uint8_t*>(malloc(width * height));
  this->b = static_cast<uint8_t*>(malloc(width * height));

  this->ok =
      (this->r != nullptr) && (this->g != nullptr) && (this->b != nullptr);
}

Image Image::fromRgba(const uint8_t* src, uint32_t width, uint32_t height) {
  Image result;
  result.init(width, height);
  if (result.ok) {
    for (size_t y = 0; y < height; ++y) {
      const uint8_t* from = src + y * 4 * width;
      uint8_t* to_r = result.r + y * width;
      uint8_t* to_g = result.g + y * width;
      uint8_t* to_b = result.b + y * width;
      for (size_t x = 0; x < width; ++x) {
        to_r[x] = from[4 * x];
        to_g[x] = from[4 * x + 1];
        to_b[x] = from[4 * x + 2];
      }
    }
  }

  return result;
}

}  // namespace twim
