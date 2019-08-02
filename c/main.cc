#include <cstdio>

#include "decoder.h"
#include "encoder.h"
#include "io.h"
#include "platform.h"

namespace twim {

int main (int argc, char* argv[]) {
  bool encode = false;
  bool roundtrip = false;
  int32_t target_size = 128;

  for (int i = 1; i < argc; ++i) {
    if (argv[i] == nullptr) {
      continue;
    }
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'e') {
        encode = true;
        continue;
      }
      if (argv[i][1] == 'd') {
        encode = false;
        continue;
      }
      if (argv[i][1] == 'r') {
        roundtrip = true;
        continue;
      }
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    std::string path(argv[i]);
    if (encode) {
      const Image src = Io::readPng(path);
      path = path + ".2im";
      auto data = Encoder::encode(src, target_size);
      Io::writeFile(path, data);
    }
    if (!encode || roundtrip) {
      auto data = Io::readFile(path);
      if (data.size() == 0) {
        fprintf(stderr, "Failed to read [%s].\n", path.c_str());
        continue;
      }
      Image decoded = Decoder::decode(std::move(data));
      if (decoded.height == 0) {
        fprintf(stderr, "Corrupted image [%s].\n", path.c_str());
        continue;
      }
      std::string out = path + ".png";
      if (!Io::writePng(out, decoded)) {
        fprintf(stderr, "Failed encode png [%s].\n", out.c_str());
        continue;
      }
    }
  }

  return 0;
}

}  // namespace

int main (int argc, char* argv[]) {
  return twim::main(argc, argv);
}
