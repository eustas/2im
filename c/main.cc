#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "codec_params.h"
#include "decoder.h"
#include "encoder.h"
#include "io.h"
#include "platform.h"
#include "xrange_decoder.h"
#include "xrange_encoder.h"

namespace twim {

using ::twim::Encoder::Result;
using ::twim::Encoder::Variant;

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

size_t parseHex(const char* str, uint64_t* result, char t1, char t2) {
  uint64_t val = 0;
  size_t len = 0;
  while (true) {
    char c = str[len++];
    if ((c == t1) || (c == t2)) break;
    if (len > 13) return 0;
    bool ok = false;
    uint64_t d = 0;
    if ((c >= '0') && (c <= '9')) { d = c - '0'; ok = true; }
    if ((c >= 'a') && (c <= 'f')) { d = c - 'a' + 10; ok = true; }
    if ((c >= 'A') && (c <= 'F')) { d = c - 'A' + 10; ok = true; }
    if (!ok) return 0;
    val = (val << 4) + d;
  }
  *result = val;
  return len;
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
const uint32_t kMaxTargetSize = 1024 * 1024;

void printHelp(const char* name, bool error) {
  FILE* media = error ? stderr : stdout;
  fprintf(media,
"Usage: %s [OPTION]... [FILE]...\n",
          name);
  fprintf(media,
"Options:\n"
"  -d     decode\n"
"  -e     encode\n"
"  -j###  set number of threads (1..256); default: 1\n"
"  -h     display this help and exit\n"
"  -p###  encoding parameters (see below); default: all possible combinations\n"
"  -r     decode after encoding\n");
  fprintf(media,
"  -t###  set target encoded size in bytes (%d..%d); default: %d\n",
          kMinTargetSize, kMaxTargetSize, kDefaultTargetSize);
  fprintf(media,
"\n"
"Encoding parameters (-p option) should match the following pattern:\n"
"  (PARTITION_CODE:LINE_LIMIT:COLOR_SCHEMES,)+\n"
"where each part is a hexadecimal number; encoder dumps chosen triplet when\n"
"encoding is done; COLOR_SCHEMES is a bitfield: multiple schemes could be\n"
"combined via bitwise \"or\"; trailing comma could be omitted.\n"
"\n"
"Options and files could be mixed, e.g '-d a.2im -e b.png -t42 c.png' will\n"
"decode a.2im, endode b.png, set target size to 42 and encode c.png\n");
}

void fillAllVariants(std::vector<Variant>* variants) {
  uint64_t allColorOptions = ((uint64_t)1 << CodecParams::kMaxColorCode) - 1;
  variants->resize(CodecParams::kMaxPartitionCode * CodecParams::kMaxLineLimit);
  size_t idx = 0;
  for (uint32_t p = 0; p < CodecParams::kMaxPartitionCode; ++p) {
    for (uint32_t l = 0; l < CodecParams::kMaxLineLimit; ++l) {
      Variant& v = variants->at(idx++);
      v.partitionCode = p;
      v.lineLimit = l;
      v.colorOptions = allColorOptions;
    }
  }
  variants->resize(idx);
}

bool parseVariants(std::vector<Variant>* variants, const char* params) {
  variants->resize(CodecParams::kMaxPartitionCode * CodecParams::kMaxLineLimit);
  size_t idx = 0;
  size_t offset = 0;
  while (true) {
    uint64_t partitionCode;
    uint64_t lineLimit;
    uint64_t colorOptions;
    size_t plus = parseHex(params + offset, &partitionCode, ':', ':');
    if (plus < 2) return false;
    if (partitionCode >= CodecParams::kMaxPartitionCode) return false;
    offset += plus;
    plus = parseHex(params + offset, &lineLimit, ':', ':');
    if (plus < 2) return false;
    if (lineLimit >= CodecParams::kMaxLineLimit) return false;
    offset += plus;
    plus = parseHex(params + offset, &colorOptions, ',', 0);
    if (plus < 2) return false;
    if ((colorOptions >> CodecParams::kMaxColorCode) > 0) return false;
    offset += plus;
    Variant& v = variants->at(idx++);
    v.partitionCode = static_cast<uint32_t>(partitionCode);
    v.lineLimit = static_cast<uint32_t>(lineLimit);
    v.colorOptions = colorOptions;
    if (params[offset - 1] == 0) break;
  }
  variants->resize(idx);
  return true;
}

int main(int argc, char* argv[]) {
  bool encode = false;
  bool roundtrip = false;
  Encoder::Params params;
  params.targetSize = kDefaultTargetSize;
  params.numThreads = 1;
  std::vector<Variant> variants;
  fillAllVariants(&variants);
  params.variants = variants.data();

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
      } else if (cmd == 'p') {
        bool ok = parseVariants(&variants, val);
        if (ok) continue;
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
      params.numVariants = variants.size();
      const Image src = Io::readPng(path);
      if (!src.ok) {
        fprintf(stderr, "Failed to read PNG image [%s].\n", path.c_str());
        continue;
      }
      Result result = Encoder::encode(src, params);
      Variant variant = result.variant;
      uint32_t partitionCode = variant.partitionCode;
      uint32_t lineLimit = variant.lineLimit;
      uint64_t colorCode = variant.colorOptions;
      float psnr = INFINITY;
      if (result.mse > 0.01f) {
        psnr = 10.0 * std::log10(255.0f * 255.0f / result.mse);
      }
      std::stringstream out;
      out << std::fixed << std::setprecision(2) << std::setfill('0');
      out << "size=" << result.data.size << ", PSNR=" << psnr << std::uppercase
          << std::hex << ", variant=" << std::setw(3) << partitionCode << ":"
          << std::setw(2) << lineLimit << ":" << std::setw(13) << colorCode;
      fprintf(stderr, "%s\n", out.str().c_str());
      path += ".2im";
      Io::writeFile(path, result.data.data, result.data.size);
    }
    if (!encode || roundtrip) {
      auto data = Io::readFile(path);
      if (data.empty()) {
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

}  // namespace twim

int main(int argc, char* argv[]) { return twim::main(argc, argv); }
