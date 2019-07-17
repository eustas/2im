#include <cstdio>

#include "decoder.h"
#include "io.h"
#include "platform.h"

int main (int argc, char* argv[]) {
  using namespace twim;

  bool encode = false;
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
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    std::string path(argv[i]);
    if (encode) {
      fprintf(stderr, "Encoding is not implemented yet\n");
      exit(EXIT_FAILURE);
    }
    if (!encode) {
      auto data = Io::readFile(path);
      if (data->size() == 0) {
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
