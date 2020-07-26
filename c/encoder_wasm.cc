#include "codec_params.h"
#include "encoder.h"
#include "image.h"
#include "platform.h"

extern "C" {

const uint8_t* twimEncode(uint32_t width, uint32_t height, uint8_t* rgba,
                          uint32_t targetSize, uint32_t partitionCode,
                          uint32_t lineLimit, uint32_t colorQuantOptions,
                          uint32_t colorPaletteOptions) {
  ::twim::Image src = ::twim::Image::fromRgba(rgba, width, height);
  ::twim::Encoder::Params params;
  params.targetSize = targetSize;
  params.numThreads = 1;
  params.variants.resize(1);
  params.variants[0].partitionCode = partitionCode;
  params.variants[0].lineLimit = lineLimit;
  params.variants[0].colorOptions =
      colorQuantOptions & 0x1FFFF |
      ((colorPaletteOptions & 0xFFFF) << ::twim::CodecParams::kNumColorQuantOptions);
  ::twim::Encoder::Result result = ::twim::Encoder::encode(src, params);

  size_t size = result.data.size();
  if (size == 0) return nullptr;
  void* output = malloc(8 + size);
  if (output == nullptr) return nullptr;
  uint8_t* output_bytes = reinterpret_cast<uint8_t*>(output);
  reinterpret_cast<uint32_t*>(output)[0] = static_cast<uint32_t>(size);
  reinterpret_cast<float*>(output)[1] = result.mse;
  memcpy(output_bytes + 8, result.data.data(), size);
  return output_bytes;
}

}  // extern "C"