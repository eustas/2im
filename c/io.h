#ifndef TWIM_IO
#define TWIM_IO

#include <string>
#include <vector>

#include "image.h"
#include "platform.h"

namespace twim {

class Io {
 public:
  static std::unique_ptr<std::vector<uint8_t>> readFile(
      const std::string& path);
  static bool writeFile(const std::string& path,
                        const std::vector<uint8_t>& data);

  static Image readPng(const std::string& path);
  static bool writePng(const std::string& path, const Image& img);
};

}  // namespace twim

#endif  // TWIM_IO
