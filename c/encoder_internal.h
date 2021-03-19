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
  ~UberCache() {
    delete sum;
  }

  const uint32_t width;
  const uint32_t height;
  const uint32_t stride;
  /* Cumulative sums. Extra column with total sum. */
  Vector<int32_t>* sum;

  float imageTax;
  float sqeBase = 0.0f;

  explicit UberCache(const Image& src);
};

class Cache {
 public:
  INLINE ~Cache() {
    delete row_offset;
    delete y;
    delete x0;
    delete x1;
    delete x;
    delete stats;
  }
  const UberCache* uber;

  uint32_t count;
  Vector<int32_t>* row_offset;
  Vector<float>* y;
  Vector<int32_t>* x0;
  Vector<int32_t>* x1;
  Vector<int32_t>* x;
  Vector<float>* stats;

  explicit Cache(const UberCache& uber);
};

class Fragment {
 public:
  Vector<int32_t>* region;
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
    delete region;
    delete leftChild;
    delete rightChild;
  }

  static void* operator new(size_t sz) {return mallocOrDie(sz);}

  NOINLINE explicit Fragment(uint32_t height)
      : region(allocVector<int32_t>(3 * vecSize(height))) {}

  void encode(XRangeEncoder* dst, const CodecParams& cp, bool is_leaf,
              const float* RESTRICT palette, Array<Fragment*>* children);
};

class Partition {
 public:
  static void* operator new(size_t sz) {return mallocOrDie(sz);}
  Partition(Cache* cache, const CodecParams& cp, size_t targetSize);

  const Array<Fragment*>* getPartition() const;

  // CodecParams should be the same as passed to constructor; only color code
  // is allowed to be different.
  uint32_t subpartition(float imageTax, const CodecParams& cp,
                        uint32_t target_size) const;

 private:
  Fragment root;
  Array<Fragment*> partition;
};

}  // namespace twim

#endif  // TWIM_ENCODER_INTERNAL
