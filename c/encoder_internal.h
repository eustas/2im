#ifndef TWIM_ENCODER_INTERNAL
#define TWIM_ENCODER_INTERNAL

#include <cmath>
#include <memory>
#include <vector>

#include "platform.h"

namespace twim {

class CodecParams;
struct Image;
class XRangeEncoder;

class UberCache {
 public:
  const uint32_t width;
  const uint32_t height;
  const uint32_t stride;
  /* Cumulative sums. Extra column with total sum. */
  std::unique_ptr<Vector<float>> sum;

  float rgb2[3] = {0.0f};

  explicit UberCache(const Image& src);
};

class Cache {
 public:
  const UberCache* uber;

  uint32_t count;
  std::unique_ptr<Vector<int32_t>> row_offset;
  std::unique_ptr<Vector<float>> y;
  std::unique_ptr<Vector<int32_t>> x0;
  std::unique_ptr<Vector<int32_t>> x1;
  std::unique_ptr<Vector<int32_t>> x;
  std::unique_ptr<Vector<float>> stats;

  explicit Cache(const UberCache& uber);
};

class Fragment {
 public:
  std::unique_ptr<Vector<int32_t>> region;
  Fragment* leftChild = nullptr;
  Fragment* rightChild = nullptr;

  float stats[4];

  // Subdivision.
  uint32_t ordinal = 0x7FFFFFFF;
  int32_t level;
  uint32_t best_angle_code;
  uint32_t best_line;
  float best_score;
  uint32_t best_num_lines;
  float best_cost;

  Fragment(Fragment&&) = delete;
  Fragment& operator=(Fragment&&) = delete;
  Fragment(const Fragment&) = delete;
  Fragment& operator=(const Fragment&) = delete;
  ~Fragment() {
    delete leftChild;
    delete rightChild;
  }

  NOINLINE explicit Fragment(uint32_t height)
      : region(allocVector<int32_t>(3 * vecSize(height))) {}

  void encode(XRangeEncoder* dst, const CodecParams& cp, bool is_leaf,
              const float* RESTRICT palette, Fragment** children,
              size_t* numChildren);
};

class Partition {
 public:
  Partition(Cache* cache, const CodecParams& cp, size_t target_size);

  const std::vector<Fragment*>* getPartition() const;

  // CodecParams should be the same as passed to constructor; only color code
  // is allowed to be different.
  uint32_t subpartition(const CodecParams& cp, uint32_t target_size) const;

 private:
  Fragment root;
  std::vector<Fragment*> partition;
};

}  // namespace twim

#endif  // TWIM_ENCODER_INTERNAL