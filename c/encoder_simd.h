#ifndef TWIM_ENCODER_SIMD
#define TWIM_ENCODER_SIMD

#include "platform.h"

namespace twim {

class Cache;
class CodecParams;
class Fragment;
class Partition;

float simulateEncode(float imageTax, const Partition& partition_holder,
                     uint32_t target_size, const CodecParams& cp);

uint32_t chooseColor(float r, float g, float b, const float* RESTRICT palette_r,
                     const float* RESTRICT palette_g,
                     const float* RESTRICT palette_b, uint32_t palette_size,
                     float* RESTRICT distance2);

void findBestSubdivision(Fragment* f, Cache* cache, const CodecParams& cp);

Vector<float>* gatherPatches(
    const Array<Fragment*>* partition, uint32_t num_non_leaf);

Vector<float>* buildPalette(
    const Vector<float>* patches, uint32_t palette_size);

}  // namespace twim

#endif  // TWIM_ENCODER_SIMD
