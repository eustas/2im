#include "codec_params.h"
#include "encoder.h"
#include "image.h"
#include "platform.h"

extern "C" {

static uint32_t freeHeads[32] = {0};
static uint32_t freePages = 0;
static uint32_t freeStart = 0;

NOINLINE void* malloc(size_t size) {
  size_t storageSize = (size < 65535) ? size : (size + 4);
  uint32_t bucket = (__builtin_clz(storageSize) ^ 31) + 1;
  if (bucket < 3) bucket = 3;  // 4..7 bytes
  if (freeHeads[bucket] == 0) {
    uint32_t wantPages = (bucket <= 16) ? 1 : (1 << (bucket - 16));
    if (freePages < wantPages) {
      uint32_t currentPages = __builtin_wasm_memory_size(0);
      if (freePages == 0) freeStart = currentPages << 16;
      uint32_t plusPages = currentPages;  // Double the memory...
      if (plusPages < 32) plusPages = 32;  // ... not less than +2MB
      if (plusPages > 256) plusPages = 256;  // ... not more than +16MB
      if (plusPages < wantPages - freePages) plusPages = wantPages - freePages;
      if (__builtin_wasm_memory_grow(0, plusPages) == -1) __builtin_trap();
      freePages += plusPages;
    }
    if (bucket <= 16) {
      size_t step = 1 << bucket;
      size_t count = 1 << (16 - bucket);
      size_t address = freeStart;
      for (size_t i = 0; i < count - 1; ++i) {
        reinterpret_cast<uint32_t*>(address)[0] = address + step;
        address += step;
      }
      freeHeads[bucket] = freeStart;
      reinterpret_cast<uint32_t*>(freeStart + (count - 1) * step)[0] = 0;
      reinterpret_cast<uint8_t*>(freeStart | 0xFFFF)[0] = bucket;
    } else {
      freeHeads[bucket] = freeStart + 4;
      reinterpret_cast<uint8_t*>(freeStart + 3)[0] = bucket;
    }
    freeStart += wantPages << 16;
    freePages -= wantPages;
  }
  uint32_t result = freeHeads[bucket];
  freeHeads[bucket] = reinterpret_cast<uint32_t*>(result)[0];
  return reinterpret_cast<void*>(result);
}

NOINLINE void free(void* ptr) {
  uint32_t p = reinterpret_cast<uint32_t>(ptr);
  size_t bucket;
  if ((p & 0xFFFF) != 4) {
    // Small bucket
    bucket = reinterpret_cast<uint8_t*>(p | 0xFFFF)[0];
  } else {
    // Big bucket
    bucket = reinterpret_cast<uint8_t*>(p - 1)[0];
  }
  reinterpret_cast<uint32_t*>(p)[0] = freeHeads[bucket];
  freeHeads[bucket] = p;
}

const uint8_t* twimEncode(uint32_t width, uint32_t height, uint8_t* rgba,
                          uint32_t targetSize, uint32_t partitionCode,
                          uint32_t lineLimit, uint32_t colorQuantOptions,
                          uint32_t colorPaletteOptions) {
  ::twim::Image src = ::twim::Image::fromRgba(rgba, width, height);
  if (!src.ok) return nullptr;
  ::twim::Encoder::Params params;
  ::twim::Encoder::Variant variant;
  params.targetSize = targetSize;
  params.numThreads = 1;
  params.variants = &variant;
  params.numVariants = 1;
  variant.partitionCode = partitionCode;
  variant.lineLimit = lineLimit;
  variant.colorOptions = colorQuantOptions & 0x1FFFF |
                         ((colorPaletteOptions & 0xFFFF)
                          << ::twim::CodecParams::kNumColorQuantOptions);
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