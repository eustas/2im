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

/*
 * https://xkcd.com/221/
 *
 * Chosen by fair dice roll. Guaranteed to be random.
 */
constexpr float kRandom[64] = {
    0.196761043301f, 0.735187971367f, 0.240889315713f, 0.322607869281f,
    0.042645685482f, 0.100931237742f, 0.436030039105f, 0.949386184145f,
    0.391385426476f, 0.693787004045f, 0.957348258524f, 0.401165324581f,
    0.200612435526f, 0.774156296613f, 0.702329904898f, 0.794307087431f,
    0.913297998115f, 0.418554329021f, 0.305256297017f, 0.447050968372f,
    0.275901615626f, 0.423281943403f, 0.222083232637f, 0.259557717968f,
    0.134780340092f, 0.624659579778f, 0.459554804245f, 0.629270576873f,
    0.728531198516f, 0.270863443275f, 0.730693946899f, 0.958482839910f,
    0.597229071250f, 0.020570415805f, 0.876483717523f, 0.734546837759f,
    0.548824137588f, 0.628979430442f, 0.813059781398f, 0.145038174534f,
    0.174453058546f, 0.195531684379f, 0.127489363209f, 0.878269052109f,
    0.990909412408f, 0.109277869329f, 0.295625366456f, 0.247012273577f,
    0.508121083167f, 0.875779718247f, 0.863389034205f, 0.663539415689f,
    0.069178093049f, 0.859564180486f, 0.560775815455f, 0.039552534504f,
    0.989061239776f, 0.815374917179f, 0.951061519055f, 0.211362121050f,
    0.255234747636f, 0.047947586972f, 0.520984579718f, 0.399461090480f};

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

  uint32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<int32_t>> x0;
  Owned<Vector<int32_t>> x1;
  Owned<Vector<int32_t>> x;
  Owned<Vector<float>> stats;

  explicit Cache(const UberCache& uber);
};

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

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

  NOINLINE explicit Fragment(uint32_t height)
      : region(allocVector<int32_t>(3 * vecSize(height))) {}

  void encode(XRangeEncoder* dst, const CodecParams& cp, bool is_leaf,
              const float* RESTRICT palette, std::vector<Fragment*>* children);
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