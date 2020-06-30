#include <cstdio>

#include "decoder.h"
#include "encoder.h"
#include "io.h"
#include "platform.h"
#include "xrange_decoder.h"
#include "xrange_encoder.h"

namespace twim {

bool parseInt(const char* str, uint32_t min, uint32_t max, uint32_t* result) {
  uint32_t val = 0;
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

/* Returns "base file name" or its tail, if it contains '/' or '\'. */
static const char* fileName(const char* path) {
  const char* separatorPosition = strrchr(path, '/');
  if (separatorPosition) path = separatorPosition + 1;
  separatorPosition = strrchr(path, '\\');
  if (separatorPosition) path = separatorPosition + 1;
  return path;
}

const uint32_t kMinTargetSize = 16;
const uint32_t kDefaultTargetSize = 287;
const uint32_t kMaxTargetSize = 65536;

void printHelp(const char* name, bool error) {
  FILE* media = error ? stderr : stdout;
  fprintf(media,
"Usage: %s [OPTION]... [FILE]...\n",
          name);
  fprintf(media,
"Options:\n"
"  -d     decode\n"
"  -e     encode\n"
"  -j     set number of threads (1..256); default: 1\n"
"  -h     display this help and exit\n"
"  -r     decode after encoding\n");
  fprintf(media,
"  -t###  set target encoded size in bytes (%d..%d); default: %d\n",
          kMinTargetSize, kMaxTargetSize, kDefaultTargetSize);
  fprintf(media,
"Options and files could be mixed, e.g '-d a.2im -e b.png -t42 c.png' will\n"
"decode a.2im, endode b.png, set target size to 42 and encode c.png\n");
}

int main(int argc, char* argv[]) {
  bool encode = false;
  bool roundtrip = false;
  Encoder::Params params;
  params.targetSize = kDefaultTargetSize;
  params.numThreads = 1;

  if (argc < 2) {
    printHelp(fileName(argv[0]), false);
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; ++i) {
    if (argv[i] == nullptr) {
      continue;
    }
    if (argv[i][0] == '-') {
      char cmd = argv[i][1];
      const char* val = &argv[i][2];
      if (cmd == 'e') {
        encode = true;
        continue;
      } else if (cmd == 'd') {
        encode = false;
        continue;
      } else if (cmd == 'h') {
        printHelp(fileName(argv[0]), true);
        exit(EXIT_SUCCESS);
      } else if (cmd == 'r') {
        roundtrip = true;
        continue;
      } else if (cmd == 't') {
        bool ok =
            parseInt(val, kMinTargetSize, kMaxTargetSize, &params.targetSize);
        if (ok) continue;
      } else if (cmd == 'j') {
        bool ok = parseInt(val, 1, 256, &params.numThreads);
        if (ok) continue;
      }
      fprintf(stderr, "Unknown / invalid option: %s\n", argv[i]);
      printHelp(fileName(argv[0]), false);
      exit(EXIT_FAILURE);
    }
    std::string path(argv[i]);
    if (encode) {
      const Image src = Io::readPng(path);
      auto data = Encoder::encode<XRangeEncoder>(src, params);
      path += ".2im";
      Io::writeFile(path, data);
    }
    if (!encode || roundtrip) {
      auto data = Io::readFile(path);
      if (data.empty()) {
        fprintf(stderr, "Failed to read [%s].\n", path.c_str());
        continue;
      }
      Image decoded = Decoder::decode<XRangeDecoder>(std::move(data));
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

}  // namespace twim

int main(int argc, char* argv[]) { return twim::main(argc, argv); }
