#include <cstdio>

#include "decoder.h"
#include "encoder.h"
#include "io.h"
#include "platform.h"

namespace twim {

bool parseInt(char* str, int min, int max, int* result) {
  int val = 0;
  size_t len = 0;
  while (true) {
    char c = str[len++];
    if (c == 0) break;
    if ((c >= '0') && (c <= '9')) {
      int d = c - '0';
      if ((len == 0) && (d == 0)) return false;
      val = 10 * val + d;
      if (val > max) return false;
    }
  }
  if (val < min) return false;
  *result = val;
  return true;
}

int main (int argc, char* argv[]) {
  bool encode = false;
  bool roundtrip = false;
  bool append_size = false;
  int32_t target_size = 200;

  for (int i = 1; i < argc; ++i) {
    if (argv[i] == nullptr) {
      continue;
    }
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'e') {
        encode = true;
        continue;
      } else if (argv[i][1] == 'd') {
        encode = false;
        continue;
      } else if (argv[i][1] == 'r') {
        roundtrip = true;
        continue;
      } else if (argv[i][1] == 't') {
        if (parseInt(&argv[i][2], 8, 16384, &target_size)) continue;
      } else if (argv[i][1] == 's') {
        append_size = true;
        continue;
      }
      fprintf(stderr, "Unknown / invalid option: %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    std::string path(argv[i]);
    if (encode) {
      const Image src = Io::readPng(path);
      auto data = Encoder::encode(src, target_size);
      if (append_size) {
        path = path + "." + std::to_string(data.size());
      }
      path = path + ".2im";
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
