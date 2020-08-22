#include "codec_params.h"
#include "encoder.h"
#include "image.h"
#include "platform.h"

extern "C" {

// Slower?... Not really.
void* memcpy(void* destination, const void* source, size_t num) {
  const uint8_t* src = reinterpret_cast<const uint8_t*>(source);
  uint8_t* dst = reinterpret_cast<uint8_t*>(destination);
  for (size_t i = 0; i < num; ++i) dst[i] = src[i];
  return destination;
}

/**
 * |variants| should point to 9 * |numVariants| bytes; each byte position in
 * 5-byte group corresponds to:
 *   0: partitionCode
 *   1: lineLimit
 *   2..8: (little-endian) color palette / quantization bits
 */
const uint8_t* twimEncode(uint32_t width, uint32_t height, uint8_t* rgba,
                          uint32_t targetSize, size_t numVariants,
                          const uint8_t* variants) {
  using namespace ::twim;
  using namespace ::twim::Encoder;
  Image src = Image::fromRgba(rgba, width, height);
  if (!src.ok) return nullptr;
  Variant* parsedVariants =
      reinterpret_cast<Variant*>(mallocOrDie(numVariants * sizeof(Variant)));
  Params params;
  params.targetSize = targetSize;
  params.numThreads = 1;
  params.variants = parsedVariants;
  params.numVariants = numVariants;
  for (size_t i = 0; i < numVariants; ++i) {
    Variant& variant = parsedVariants[i];
    const uint8_t* record = variants + 9 * i;
    variant.partitionCode = record[0];
    variant.lineLimit = record[1];
    uint64_t colorOptions = 0;
    for (size_t j = 0; j < 7; ++j) {
      colorOptions |= uint64_t(record[2 + j]) << (8 * j);
    }
    variant.colorOptions = colorOptions;
  }
  Result result = encode(src, params);
  free(parsedVariants);

  size_t size = result.data.size;
  if (size == 0) return nullptr;
  void* output = mallocOrDie(8 + size);
  uint8_t* output_bytes = reinterpret_cast<uint8_t*>(output);
  reinterpret_cast<uint32_t*>(output)[0] = static_cast<uint32_t>(size);
  reinterpret_cast<float*>(output)[1] = result.mse;
  memcpy(output_bytes + 8, result.data.data, size);
  return output_bytes;
}

}  // extern "C"
