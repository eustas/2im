#ifndef TWIM_ENCODER_INTERNAL
#define TWIM_ENCODER_INTERNAL

#include <vector>

#include "codec_params.h"
#include "image.h"
#include "platform.h"

namespace twim {

namespace internal {

struct Stats {
  /* sum_r, sum_g, sum_b, pixel_count */
  ALIGNED_16 float values[4];
};

class UberCache {
 public:
  const uint32_t width;
  const uint32_t height;
  const uint32_t stride;
  /* Cumulative sums. Extra column with total sum. */
  Owned<Vector<float>> sum;

  float rgb2[3] = {0.0f};

  explicit UberCache(const Image& src);
};

class Cache {
 public:
  const UberCache* uber;
  Stats plus;
  Stats minus;
  Stats stats[CodecParams::kMaxLineLimit + 3];

  uint32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<int32_t>> x0;
  Owned<Vector<int32_t>> x1;
  Owned<Vector<int32_t>> x;

  explicit Cache(const UberCache& uber);

  template <bool ABS>
  void NOINLINE SIMD sum(const int32_t* RESTRICT region_x, Stats* dst);

  void INLINE SIMD prepare(Vector<int32_t>* region);
};

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Stats stats;
  Stats stats2;

  // Subdivision.
  uint32_t ordinal = 0x7FFFFFFF;
  int32_t level;
  uint32_t best_angle_code;
  uint32_t best_line;
  float best_score;
  uint32_t best_num_lines;
  float best_cost;

  Fragment(Fragment&&) = default;
  Fragment& operator=(Fragment&&) = default;
  Fragment(const Fragment&) = delete;
  Fragment& operator=(const Fragment&) = delete;

  explicit Fragment(Owned<Vector<int32_t>>&& region);

  template<typename EntropyEncoder>
  SIMD NOINLINE void encode(EntropyEncoder* dst, const CodecParams& cp,
                            bool is_leaf, const float* RESTRICT palette,
                            std::vector<Fragment*>* children);

  void SIMD NOINLINE findBestSubdivision(Cache* cache, CodecParams cp);
};

class Partition {
 public:
  Partition(const UberCache& uber, const CodecParams& cp, size_t target_size);

  const std::vector<Fragment*>* getPartition() const;

  // CodecParams should be the same as passed to constructor; only color code
  // is allowed to be different.
  uint32_t subpartition(const CodecParams& cp, uint32_t target_size) const;

 private:
  Cache cache;
  Fragment root;
  std::vector<Fragment*> partition;
};

SIMD NOINLINE Owned<Vector<float>> gatherPatches(
    const std::vector<Fragment*>* partition, uint32_t num_non_leaf);

SIMD NOINLINE Owned<Vector<float>> buildPalette(
    const Owned<Vector<float>>& patches, uint32_t palette_size);

template<typename EntropyEncoder>
std::vector<uint8_t> doEncode(uint32_t num_non_leaf,
                              const std::vector<Fragment*>* partition,
                              const CodecParams& cp,
                              const float* RESTRICT palette);

}  // namespace internal

}  // namespace twim

#endif  // TWIM_ENCODER_INTERNAL
