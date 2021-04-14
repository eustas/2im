#include <vector>

#include "decoder.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> input(data, data + size);
  twim::Decoder::decode(std::move(input));
  return 0;
}
