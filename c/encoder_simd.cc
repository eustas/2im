#if !defined(__wasm__)
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "encoder_simd.cc"
#include "hwy/foreach_target.h"
#endif

#include <algorithm>

#include "codec_params.h"
#include "distance_range.h"
#include "encoder_internal.h"
#include "platform.h"
#include "region.h"
#include "sin_cos.h"

#include "hwy/targets.h"  // SupportedAndGeneratedTargets

/*extern "C" {
void debugInt(uint32_t i);
void debugFloat(float f);
}*/

#include <hwy/before_namespace-inl.h>
namespace twim {
#include <hwy/begin_target-inl.h>

struct Stats {
  /* sum_r, sum_g, sum_b, pixel_count */
  HWY_ALIGN float values[4];
};

INLINE void diff(Stats* diff, const Stats& plus, const Stats& minus) {
#if HWY_TARGET == HWY_SCALAR
  for (size_t i = 0; i < 4; ++i) {
    diff->values[i] = plus.values[i] - minus.values[i];
  }
#else
  constexpr HWY_CAPPED(float, 4) df;
  const auto p = Load(df, plus.values);
  const auto m = Load(df, minus.values);
  Store(p - m, df, diff->values);
#endif
}

void INLINE updateGeHorizontal(Cache* cache, int32_t d) {
  constexpr HWY_FULL(float) df;
  constexpr HWY_FULL(int32_t) di32;

  int32_t ny = SinCos.kCos[0];
  float d_ny = d / (float)ny;  // TODO: precalculate
  size_t count = cache->count;
  int32_t* RESTRICT row_offset = cache->row_offset->data();
  float* RESTRICT region_y = cache->y->data();
  int32_t* RESTRICT region_x0 = cache->x0->data();
  int32_t* RESTRICT region_x1 = cache->x1->data();
  int32_t* RESTRICT region_x = cache->x->data();

  const auto k4 = Set(di32, 4);
  const auto d_ny_ = Set(df, d_ny);
  for (size_t i = 0; i < count; i += Lanes(df)) {
    const auto y = Load(df, region_y + i);
    const auto offset = Load(di32, row_offset + i);
    const auto x0 = Load(di32, region_x0 + i);
    const auto x1 = Load(di32, region_x1 + i);
    const auto selector = MaskFromVec(BitCast(di32, VecFromMask(y < d_ny_)));
    const auto x = IfThenElse(selector, x1, x0);
    const auto x_off = k4 * x + offset;
    Store(x_off, di32, region_x + i);
  }
}

INLINE void updateGeGeneric(Cache* cache, int32_t angle, int32_t d) {
  constexpr HWY_FULL(float) df;
  constexpr HWY_FULL(int32_t) di32;

  float m_ny_nx = SinCos.kMinusCot[angle];
  float d_nx = static_cast<float>(d * SinCos.kInvSin[angle] + 0.5);
  int32_t* RESTRICT row_offset = cache->row_offset->data();
  float* RESTRICT region_y = cache->y->data();
  int32_t* RESTRICT region_x0 = cache->x0->data();
  int32_t* RESTRICT region_x1 = cache->x1->data();
  int32_t* RESTRICT region_x = cache->x->data();

  const auto k4 = Set(di32, 4);
  const auto d_nx_ = Set(df, d_nx);
  const auto m_ny_nx_ = Set(df, m_ny_nx);
  const size_t count = cache->count;
  for (size_t i = 0; i < count; i += Lanes(df)) {
    const auto y = Load(df, region_y + i);
    const auto offset = Load(di32, row_offset + i);
    const auto xf = MulAdd(y, m_ny_nx_, d_nx_);
    const auto xi = ConvertTo(di32, xf);
    const auto x0 = Load(di32, region_x0 + i);
    const auto x1 = Load(di32, region_x1 + i);
    const auto x = Min(x1, Max(xi, x0));
    const auto x_off = k4 * x + offset;
    Store(x_off, di32, region_x + i);
  }
}

INLINE void updateGe(Cache* cache, int angle, int d) {
  if (angle == 0) {
    updateGeHorizontal(cache, d);
  } else {
    updateGeGeneric(cache, angle, d);
  }
}

/*
 * (whole_avg^2 - left_avg^2) * left_pc + 2 * left_pc * (left_avg^2
 * -left_avg*whole_avg) left_pc* (whole_avg^2 - 2 *  whole_avg * left_avg +
 * left_avg^2) = (whole_avg - left_avg) ^ 2 * left_pc
 */
void score(const Stats& whole, size_t n, const float* RESTRICT r,
           const float* RESTRICT g, const float* RESTRICT b,
           const float* RESTRICT c, float* RESTRICT s) {
  constexpr HWY_FULL(float) df;

  const auto k1 = Set(df, 1.0f);

  // TODO(eustas): could be done more energy-efficiently.
  const auto whole_c = Set(df, whole.values[3]);
  const auto whole_inv_c = k1 / whole_c;
  const auto whole_r = Set(df, whole.values[0]);
  const auto whole_g = Set(df, whole.values[1]);
  const auto whole_b = Set(df, whole.values[2]);
  const auto whole_avg_r = whole_r * whole_inv_c;
  const auto whole_avg_g = whole_g * whole_inv_c;
  const auto whole_avg_b = whole_b * whole_inv_c;

  for (size_t i = 0; i < n; i += Lanes(df)) {
    const auto left_c = Load(df, c + i);
    const auto left_inv_c = k1 / left_c;
    const auto right_c = whole_c - left_c;
    const auto right_inv_c = k1 / right_c;

    const auto left_r = Load(df, r + i);
    const auto left_avg_r = left_r * left_inv_c;
    const auto left_diff_r = left_avg_r - whole_avg_r;
    auto sum_left = left_diff_r * left_diff_r;
    const auto right_r = whole_r - left_r;
    const auto right_avg_r = right_r * right_inv_c;
    const auto right_diff_r = right_avg_r - whole_avg_r;
    auto sum_right = right_diff_r * right_diff_r;

    const auto left_g = Load(df, g + i);
    const auto left_avg_g = left_g * left_inv_c;
    const auto left_diff_g = left_avg_g - whole_avg_g;
    sum_left = MulAdd(left_diff_g, left_diff_g, sum_left);
    const auto right_g = whole_g - left_g;
    const auto right_avg_g = right_g * right_inv_c;
    const auto right_diff_g = right_avg_g - whole_avg_g;
    sum_right = MulAdd(right_diff_g, right_diff_g, sum_right);

    const auto left_b = Load(df, b + i);
    const auto left_avg_b = left_b * left_inv_c;
    const auto left_diff_b = left_avg_b - whole_avg_b;
    sum_left = MulAdd(left_diff_b, left_diff_b, sum_left);
    const auto right_b = whole_b - left_b;
    const auto right_avg_b = right_b * right_inv_c;
    const auto right_diff_b = right_avg_b - whole_avg_b;
    sum_right = MulAdd(right_diff_b, right_diff_b, sum_right);

    Store(sum_left * left_c + sum_right * right_c, df, s + i);
  }
}

/* |length| is for scalar mode; vectorized versions look after the end of
   input up to complete vector, where "big" fillers have to be placed. */
template <typename Op>
INLINE uint32_t choose(Op op, const float* RESTRICT values, size_t length) {
  constexpr HWY_FULL(float) df;
  constexpr HWY_FULL(int32_t) di32;

#if HWY_TARGET == HWY_SCALAR
  bool use_scalar = true;
#else
  // TODO(eustas): could be less, but more workaround code is required.
  bool use_scalar = true;//(length < Lanes(df));
#endif
  if (use_scalar) return op.scalarChoose(values, length);

  constexpr const HWY_FULL(int32_t) kDi32;
  HWY_ALIGN static const int32_t kIotaArray[] = {0, 1, 2,  3,  4,  5,  6,  7,
                                                  8, 9, 10, 11, 12, 13, 14, 15};
  static const I32xN kIota = Load(kDi32, kIotaArray);
  static const I32xN kStep = Set(kDi32, static_cast<int32_t>(Lanes(kDi32)));

  auto limit = Set(di32, static_cast<uint32_t>(length));
  auto idx = kIota;
  auto bestIdx = idx;
  auto bestValue = Load(df, values);
  for (size_t i = Lanes(df); i < length; i += Lanes(df)) {
    idx = idx + kStep;
    const auto value = Load(df, values + i);
    const auto selector = BitCast(di32, VecFromMask(op.select(value, bestValue))) & VecFromMask(idx < limit);
    bestValue = IfThenElse(MaskFromVec(BitCast(df, selector)), value, bestValue);
    bestIdx = IfThenElse(MaskFromVec(selector), idx, bestIdx);
  }

#if HWY_TARGET == HWY_AVX3
  {
    const decltype(bestIdx) shuffledIdx1{Blocks2301(bestIdx.raw)};
    const decltype(bestValue) shuffledValue1{Blocks2301(bestValue.raw)};
    const auto selector1 = op.select(shuffledValue1, bestValue);
    bestValue = IfThenElse(selector1, shuffledValue1, bestValue);
    bestIdx = IfThenElse(MaskFromVec(BitCast(di32, VecFromMask(selector1))),
                         shuffledIdx1, bestIdx);
    const decltype(bestIdx) shuffledIdx2{Blocks1032(bestIdx.raw)};
    const decltype(bestValue) shuffledValue2{Blocks1032(bestValue.raw)};
    const auto selector2 = op.select(shuffledValue2, bestValue);
    bestValue = IfThenElse(selector2, shuffledValue2, bestValue);
    bestIdx = IfThenElse(MaskFromVec(BitCast(di32, VecFromMask(selector2))),
                         shuffledIdx2, bestIdx);
  }
#elif HWY_TARGET == HWY_AVX2
  {
    const auto shuffledIdx = ConcatLowerUpper(bestIdx, bestIdx);
    const auto shuffledValue = ConcatLowerUpper(bestValue, bestValue);
    const auto selector = op.select(shuffledValue, bestValue);
    bestValue = IfThenElse(selector, shuffledValue, bestValue);
    bestIdx = IfThenElse(MaskFromVec(BitCast(di32, VecFromMask(selector))),
                         shuffledIdx, bestIdx);
  }
#endif
#if HWY_TARGET != HWY_SCALAR
  {
    const auto shuffledIdx = Shuffle1032(bestIdx);
    const auto shuffledValue = Shuffle1032(bestValue);
    const auto selector = op.select(shuffledValue, bestValue);
    bestValue = IfThenElse(selector, shuffledValue, bestValue);
    bestIdx = IfThenElse(MaskFromVec(BitCast(di32, VecFromMask(selector))),
                         shuffledIdx, bestIdx);
  }
  {
    const auto shuffledIdx = Shuffle0321(bestIdx);
    const auto shuffledValue = Shuffle0321(bestValue);
    const auto selector = op.select(shuffledValue, bestValue);
    // bestValue = IfThenElse(selector, shuffledValue, bestValue);
    bestIdx = IfThenElse(MaskFromVec(BitCast(di32, VecFromMask(selector))),
                         shuffledIdx, bestIdx);
  }
#endif
  return GetLane(bestIdx);
}

struct OpMin {
  template <typename T>
  INLINE auto select(T a, T b) -> decltype(a < b) {
    return a < b;
  }
  INLINE uint32_t scalarChoose(const float* RESTRICT values, size_t length) {
    return std::distance(values, std::min_element(values, values + length));
  }
};

INLINE uint32_t chooseMin(const float* RESTRICT values, size_t length) {
  return choose(OpMin(), values, length);
}

struct OpMax {
  template <typename T>
  INLINE auto select(T a, T b) -> decltype(a > b) {
    return a > b;
  }
  INLINE uint32_t scalarChoose(const float* RESTRICT values, size_t length) {
    return std::distance(values, std::max_element(values, values + length));
  }
};

INLINE uint32_t chooseMax(const float* RESTRICT values, size_t length) {
  return choose(OpMax(), values, length);
}

NOINLINE uint32_t chooseColor(float r, float g, float b,
                              const float* RESTRICT palette_r,
                              const float* RESTRICT palette_g,
                              const float* RESTRICT palette_b,
                              uint32_t palette_size,
                              float* RESTRICT distance2) {
  constexpr HWY_FULL(float) df;
  const size_t m = palette_size;

  HWY_ALIGN float d2[32];

  const auto cr = Set(df, r);
  const auto cg = Set(df, g);
  const auto cb = Set(df, b);

  for (size_t i = 0; i < m; i += Lanes(df)) {
    const auto dr = cr - Load(df, palette_r + i);
    const auto dg = cg - Load(df, palette_g + i);
    const auto db = cb - Load(df, palette_b + i);
    Store(dr * dr + dg * dg + db * db, df, d2 + i);
  }

  uint32_t best = chooseMin(d2, m);
  *distance2 = d2[best];
  return best;
}

INLINE void makePalette(const float* stats, float* RESTRICT palette,
                        float* RESTRICT storage, uint32_t num_patches,
                        uint32_t palette_size) {
  constexpr HWY_FULL(float) df;
  constexpr HWY_FULL(int32_t) di32;

  const uint32_t n = num_patches;
  const uint32_t m = palette_size;
  const size_t centers_step = vecSize(m);
  const size_t stats_step = vecSize(n);
  float* RESTRICT centers_r = palette + 0 * centers_step;
  float* RESTRICT centers_g = palette + 1 * centers_step;
  float* RESTRICT centers_b = palette + 2 * centers_step;
  float* RESTRICT centers_acc_r = storage + 0 * centers_step;
  float* RESTRICT centers_acc_g = storage + 1 * centers_step;
  float* RESTRICT centers_acc_b = storage + 2 * centers_step;
  float* RESTRICT centers_acc_c = storage + 3 * centers_step;
  float* RESTRICT weights = storage + 0 * centers_step;
  const float* RESTRICT stats_r = stats + 0 * stats_step;
  const float* RESTRICT stats_g = stats + 1 * stats_step;
  const float* RESTRICT stats_b = stats + 2 * stats_step;
  const float* RESTRICT stats_c = stats + 3 * stats_step;
  const float* RESTRICT stats_wr = stats + 4 * stats_step;
  const float* RESTRICT stats_wg = stats + 5 * stats_step;
  const float* RESTRICT stats_wb = stats + 6 * stats_step;
  const auto k0 = Zero(df);
  const auto kOne = Set(df, 1.0f);

  uint32_t random = 0x23DE605F;

  {
    // Choose one center uniformly at random from among the data points.
    float total = 0.0f;
    uint32_t i;
    for (i = 0; i < n; ++i) total += stats_c[i];
    float r = (random >> 9) / 8388608.0;
    float target = total * r;
    float partial = 0.0f;
    for (i = 0; i < n; ++i) {
      partial += stats_c[i];
      if (partial >= target) break;
    }
    i = std::min(i, n - 1);
    centers_r[0] = stats_r[i];
    centers_g[0] = stats_g[i];
    centers_b[0] = stats_b[i];
  }

  // Warning: O(MN)
  for (uint32_t j = 1; j < m; ++j) {
    // D(x) is the distance to the nearest center.
    // Choose next with probability proportional to D(x)^2.
    uint32_t i;
    float total = 0.0f;
    for (i = 0; i < n; ++i) {
      float distance;
      chooseColor(stats_r[i], stats_g[i], stats_b[i], centers_r, centers_g,
                  centers_b, j, &distance);
      float weight = distance * stats_c[i];
      weights[i] = weight;
      total += weight;
    }
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
    {
      random ^= random << 13;
      random ^= random >> 17;
      random ^= random << 5;
    }
    float r = (random >> 9) / 8388608.0;
    float target = total * r;
    float partial = 0;
    for (i = 0; i < n; ++i) {
      partial += weights[i];
      if (partial >= target) break;
    }
    i = std::min(i, n - 1);
    centers_r[j] = stats_r[i];
    centers_g[j] = stats_g[i];
    centers_b[j] = stats_b[i];
  }

  float last_score = 1e35f;
  while (true) {
    for (size_t j = 0; j < m; j += Lanes(df)) {
      Store(k0, df, centers_acc_r + j);
      Store(k0, df, centers_acc_g + j);
      Store(k0, df, centers_acc_b + j);
      Store(k0, df, centers_acc_c + j);
    }
    float score = 0.0f;
    for (size_t i = 0; i < n; ++i) {
      float d2;
      uint32_t index = chooseColor(stats_r[i], stats_g[i], stats_b[i],
                                   centers_r, centers_g, centers_b, m, &d2);
      float c = stats_c[i];
      score += d2 * c;
      centers_acc_r[index] += stats_wr[i];
      centers_acc_g[index] += stats_wg[i];
      centers_acc_b[index] += stats_wb[i];
      centers_acc_c[index] += c;
    }
    for (size_t j = 0; j < m; j += Lanes(df)) {
      const auto c = Load(df, centers_acc_c + j);
      const auto inv_c = kOne / Max(c, kOne);
      Store(Load(df, centers_acc_r + j) * inv_c, df, centers_r + j);
      Store(Load(df, centers_acc_g + j) * inv_c, df, centers_g + j);
      Store(Load(df, centers_acc_b + j) * inv_c, df, centers_b + j);
    }
    // if (score != score) break; // TODO(eustas): is NaN possible?
    if (last_score - score < 1.0f) break;
    last_score = score;
  }

  // Round the pallete values.
  const auto k05 = Set(df, 0.5f);
  for (size_t j = 0; j < m; j += Lanes(df)) {
    const auto r =
        ConvertTo(df, ConvertTo(di32, Load(df, centers_r + j) + k05));
    const auto g =
        ConvertTo(df, ConvertTo(di32, Load(df, centers_g + j) + k05));
    const auto b =
        ConvertTo(df, ConvertTo(di32, Load(df, centers_b + j) + k05));
    Store(r, df, centers_r + j);
    Store(g, df, centers_g + j);
    Store(b, df, centers_b + j);
  }
}

NOINLINE Vector<float>* buildPalette(
    const Vector<float>* patches, uint32_t palette_size) {
  uint32_t n = patches->len;
  uint32_t m = palette_size;
  uint32_t padded_m = vecSize(m);
  uint32_t palette_space = 3 * padded_m;
  uint32_t extra_space = std::max(4 * padded_m, ((m > 0) ? n : 1));
  Vector<float>* result = allocVector<float>((m > 0) ? palette_space : 1);
  Vector<float>* extra = allocVector<float>(extra_space);

  result->len = m;
  if (m > 0) makePalette(patches->data(), result->data(), extra->data(), n, m);
  delete extra;

  return result;
}

NOINLINE Vector<float>* gatherPatches(const Array<Fragment*>* partition,
                                      uint32_t num_non_leaf) {
  constexpr HWY_FULL(float) df;
  const auto kOne = Set(df, 1.0f);

  /* In a binary tree the number of leaves is number of nodes plus one. */
  uint32_t n = num_non_leaf + 1;
  uint32_t stats_size = vecSize(n);
  Vector<float>* result = allocVector<float>(7 * stats_size);
  float* RESTRICT stats = result->data();
  float* RESTRICT stats_r = stats + 0 * stats_size;
  float* RESTRICT stats_g = stats + 1 * stats_size;
  float* RESTRICT stats_b = stats + 2 * stats_size;
  float* RESTRICT stats_c = stats + 3 * stats_size;
  float* RESTRICT stats_wr = stats + 4 * stats_size;
  float* RESTRICT stats_wg = stats + 5 * stats_size;
  float* RESTRICT stats_wb = stats + 6 * stats_size;

  size_t pos = 0;
  for (size_t i = 0; i < num_non_leaf; ++i) {
    Fragment* node = partition->data[i];
    for (Fragment* leaf : {node->leftChild, node->rightChild}) {
      if (leaf->ordinal < num_non_leaf) continue;
      stats_wr[pos] = leaf->stats[0];
      stats_wg[pos] = leaf->stats[1];
      stats_wb[pos] = leaf->stats[2];
      stats_c[pos] = leaf->stats[3];
      pos++;
    }
  }

  for (size_t i = 0; i < n; i += Lanes(df)) {
    const auto c = Load(df, stats_c + i);
    const auto inv_c = kOne / c;
    const auto r = Load(df, stats_wr + i);
    const auto g = Load(df, stats_wg + i);
    const auto b = Load(df, stats_wb + i);
    Store(r * inv_c, df, stats_r + i);
    Store(g * inv_c, df, stats_g + i);
    Store(b * inv_c, df, stats_b + i);
  }

  result->len = n;
  return result;
}

float simulateEncode(float imageTax, const Partition& partition_holder,
                     uint32_t target_size, const CodecParams& cp) {
  uint32_t num_non_leaf =
      partition_holder.subpartition(imageTax, cp, target_size);
  if (num_non_leaf <= 1) {
    // Let's deal with a flat image separately.
    return 1e35f;
  }
  // TODO(eustas): why patches storage is not cached?
  Vector<float>* patches =
      gatherPatches(partition_holder.getPartition(), num_non_leaf);
  size_t patches_step = patches->capacity / 7;
  float* RESTRICT stats = patches->data();
  float* RESTRICT patches_r = stats + 0 * patches_step;
  float* RESTRICT patches_g = stats + 1 * patches_step;
  float* RESTRICT patches_b = stats + 2 * patches_step;
  float* RESTRICT patches_c = stats + 3 * patches_step;

  size_t n = patches->len;
  const uint32_t m = cp.palette_size;
  Vector<float>* palette = buildPalette(patches, m);
  const float* RESTRICT palette_r = palette->data();
  const size_t palette_step = vecSize(m);
  const float* RESTRICT palette_g = palette_r + palette_step;
  const float* RESTRICT palette_b = palette_g + palette_step;

  // pixel_score = (orig - color)^2
  // patch_score = sum(pixel_score)
  //             = sum(orig^2) + sum(color^2) - 2 * sum(orig * color)
  //             = sum(orig^2) + area * color ^ 2 - 2 * color * sum(orig)
  //             = sum(orig^2) + area * color * (color - 2 * avg(orig))
  // score = sum(patch_score)
  // sum(sum(orig^2)) is a constant => could be omitted from calculations

  float result = 0.0f;

  int32_t v_max = cp.color_quant - 1;
  const float quant = v_max / 255.0f;
  const float dequant = 255.0f / ((v_max != 0) ? v_max : 1);

  for (size_t i = 0; i < n; ++i) {
    float orig[3] = {patches_r[i], patches_g[i], patches_b[i]};
    float rgb[3] = {orig[0], orig[1], orig[2]};
    float c = patches_c[i];
    // Lambdas were nice, but are fatty in WASM.
    if (m == 0) {
      for (size_t j = 0; j < 3; ++j) {
        int32_t quantized = static_cast<int32_t>(rgb[j] * quant + 0.5f);
        // TODO(eustas): investigate how to use floorf
        rgb[j] = std::floor(quantized * dequant);
      }
    } else {
      float dummy;
      uint32_t index = chooseColor(rgb[0], rgb[1], rgb[2], palette_r, palette_g,
                                   palette_b, m, &dummy);
      rgb[0] = palette_r[index];
      rgb[1] = palette_g[index];
      rgb[2] = palette_b[index];
    }
    for (size_t j = 0; j < 3; ++j) {
      // TODO(eustas): could use non-normalized patch color.
      result += c * rgb[j] * (rgb[j] - 2.0f * orig[j]);
    }
  }
  delete patches;
  delete palette;

  return result;
}

void sumCache(const Cache* c, const int32_t* RESTRICT region_x, Stats* dst) {
  size_t count = c->count;
  const int32_t* RESTRICT row_offset = c->row_offset->data();
  const int32_t* RESTRICT sum = c->uber->sum->data();

#if HWY_TARGET == HWY_SCALAR
  int32_t tmp[4] = {0};
  for (size_t i = 0; i < count; i++) {
    int32_t offset = row_offset[i] + 4 * region_x[i];
    tmp[0] += sum[offset + 0];
    tmp[1] += sum[offset + 1];
    tmp[2] += sum[offset + 2];
    tmp[3] += sum[offset + 3];
  }
  for (size_t j = 0; j < 4; ++j) dst->values[j] = tmp[j];
#else
  constexpr HWY_CAPPED(float, 4) df;
  constexpr HWY_CAPPED(int32_t, 4) di32;
  auto tmp = Zero(di32);
  for (size_t i = 0; i < count; i++) {
    int32_t offset = row_offset[i] + 4 * region_x[i];
    tmp = tmp + Load(di32, sum + offset);
  }
  Store(ConvertTo(df, tmp), df, dst->values);
#endif
}

INLINE void sumCacheAbs(const Cache* c, const int32_t* RESTRICT region_x,
                        Stats* dst) {
  size_t count = c->count;
  const int32_t* RESTRICT sum = c->uber->sum->data();

#if HWY_TARGET == HWY_SCALAR
  int32_t tmp[4] = {0};
  for (size_t i = 0; i < count; i++) {
    int32_t offset = region_x[i];
    tmp[0] += sum[offset + 0];
    tmp[1] += sum[offset + 1];
    tmp[2] += sum[offset + 2];
    tmp[3] += sum[offset + 3];
  }
  for (size_t j = 0; j < 4; ++j) dst->values[j] = tmp[j];
#else
  constexpr HWY_CAPPED(float, 4) df;
  constexpr HWY_CAPPED(int32_t, 4) di32;
  auto tmp1 = Load(di32, sum + region_x[0]);
  auto tmp2 = Load(di32, sum + region_x[1]);
  auto tmp3 = Load(di32, sum + region_x[2]);
  auto tmp4 = Load(di32, sum + region_x[3]);
  for (size_t i = 4; i < count; i += 4) {
    tmp1 = tmp1 + Load(di32, sum + region_x[i + 0]);
    tmp2 = tmp2 + Load(di32, sum + region_x[i + 1]);
    tmp3 = tmp3 + Load(di32, sum + region_x[i + 2]);
    tmp4 = tmp4 + Load(di32, sum + region_x[i + 3]);
  }
  Store(ConvertTo(df, (tmp1 + tmp2) + (tmp3 + tmp4)), df, dst->values);
#endif
}

void INLINE prepareCache(Cache* c, Vector<int32_t>* region) {
  constexpr HWY_FULL(float) df;
  const size_t kStrideMask = Lanes(df) - 1;

  uint32_t count = region->len;
  uint32_t step = region->capacity / 3;
  uint32_t sum_stride = c->uber->stride;
  int32_t* RESTRICT data = region->data();
  float* RESTRICT y = c->y->data();
  int32_t* RESTRICT x0 = c->x0->data();
  int32_t* RESTRICT x1 = c->x1->data();
  int32_t* RESTRICT row_offset = c->row_offset->data();
  for (size_t i = 0; i < count; ++i) {
    int32_t row = data[i];
    y[i] = row;
    x0[i] = data[step + i];
    x1[i] = data[2 * step + i];
    row_offset[i] = row * sum_stride;
  }
  while ((count & kStrideMask) != 0) {
    y[count] = 0;
    x0[count] = 0;
    x1[count] = 0;
    row_offset[count] = 0;
    count++;
  }
  c->count = count;
}

void findBestSubdivision(Fragment* f, Cache* cache, const CodecParams& cp) {
  Vector<int32_t>& region = *f->region;
  Stats stats;
  Stats plus;
  Stats minus;
  float* stats_ = cache->stats->data();
  size_t stats_step = vecSize(CodecParams::kMaxLineLimit * SinCos.kMaxAngle);
  float* RESTRICT stats_r = stats_ + 0 * stats_step;
  float* RESTRICT stats_g = stats_ + 1 * stats_step;
  float* RESTRICT stats_b = stats_ + 2 * stats_step;
  float* RESTRICT stats_c = stats_ + 3 * stats_step;
  float* RESTRICT stats_s = stats_ + 4 * stats_step;
  uint32_t* RESTRICT stats_v =
      reinterpret_cast<uint32_t*>(stats_ + 5 * stats_step);
  uint32_t level = cp.getLevel(region);
  // TODO(eustas): assert level is not kInvalid.
  uint32_t angle_max = 1u << cp.angle_bits[level];
  uint32_t angle_mult = (SinCos.kMaxAngle / angle_max);
  prepareCache(cache, &region);
  sumCache(cache, cache->x1->data(), &plus);
  sumCache(cache, cache->x0->data(), &minus);
  diff(&stats, plus, minus);
  for (size_t i = 0; i < 4; ++i) f->stats[i] = stats.values[i];

  size_t num_subdivisions = 0;
  float min_c = 0.5f;
  float max_c = stats.values[3] - 0.5f;

  // Find subdivision
  for (uint32_t angle_code = 0; angle_code < angle_max; ++angle_code) {
    int32_t angle = angle_code * angle_mult;
    DistanceRange distance_range(region, angle, cp);
    uint32_t num_lines = distance_range.num_lines;
    for (uint32_t line = 0; line < num_lines; ++line) {
      updateGe(cache, angle, distance_range.distance(line));
      sumCacheAbs(cache, cache->x->data(), &minus);
      Stats left;
      diff(&left, plus, minus);
      stats_r[num_subdivisions] = left.values[0];
      stats_g[num_subdivisions] = left.values[1];
      stats_b[num_subdivisions] = left.values[2];
      stats_c[num_subdivisions] = left.values[3];
      stats_v[num_subdivisions] = line * angle_max + angle_code;
      float c = left.values[3];
      num_subdivisions += ((c > min_c) && (c < max_c)) ? 1 : 0;
    }
  }

  if (num_subdivisions == 0) {
    f->best_cost = -1.0f;
    f->best_score = -1.0f;
    return;
  }

  uint32_t step = vecSize(1);
  while ((num_subdivisions % step) != 0) {
    stats_r[num_subdivisions] = stats_r[num_subdivisions - 1];
    stats_g[num_subdivisions] = stats_g[num_subdivisions - 1];
    stats_b[num_subdivisions] = stats_b[num_subdivisions - 1];
    stats_c[num_subdivisions] = stats_c[num_subdivisions - 1];
    stats_v[num_subdivisions] = stats_v[num_subdivisions - 1];
    num_subdivisions++;
  }
  score(stats, num_subdivisions, stats_r, stats_g, stats_b, stats_c, stats_s);
  uint32_t index = chooseMax(stats_s, num_subdivisions);
  uint32_t best_angle_code = stats_v[index] % angle_max;
  uint32_t best_line = stats_v[index] / angle_max;
  float best_score = stats_s[index];

  f->level = level;
  f->best_score = best_score;

  if (best_score < 0.5f) {
    f->best_score = -1.0f;
    f->best_cost = -1.0f;
    // TODO(eustas): why not unreachable?
  } else {
    DistanceRange distance_range(region, best_angle_code * angle_mult, cp);
    f->leftChild = new Fragment(region.len);
    f->rightChild = new Fragment(region.len);
    Region::splitLine(region, best_angle_code * angle_mult,
                      distance_range.distance(best_line),
                      f->leftChild->region,
                      f->rightChild->region);

    f->best_angle_code = best_angle_code;
    f->best_num_lines = distance_range.num_lines;
    f->best_line = best_line;
    f->best_cost = SinCos.kLog2[NodeType::COUNT] + cp.angle_bits[level] +
                   SinCos.kLog2[distance_range.num_lines];
  }
}

#include <hwy/end_target-inl.h>
}  // namespace twim
#include <hwy/after_namespace-inl.h>

#if HWY_ONCE

/*extern "C" {
#if defined(__wasm__)
extern void debugInt(uint32_t i);
extern void debugFloat(float f);
#else
void debugInt(uint32_t i) { fprintf(stderr, "i %u\n", i); }
void debugFloat(float f) { fprintf(stderr, "f %f\n", f); }
#endif
}*/

namespace twim {

#if defined(__wasm__)
#define CALL HWY_STATIC_DISPATCH
#else
#define CALL HWY_DYNAMIC_DISPATCH
HWY_EXPORT(simulateEncode)
HWY_EXPORT(chooseColor)
HWY_EXPORT(findBestSubdivision)
HWY_EXPORT(gatherPatches)
HWY_EXPORT(buildPalette)
#endif

float simulateEncode(float imageTax, const Partition& partition_holder,
                     uint32_t target_size, const CodecParams& cp) {
  return CALL(simulateEncode)(imageTax, partition_holder, target_size, cp);
}

uint32_t chooseColor(float r, float g, float b, const float* RESTRICT palette_r,
                     const float* RESTRICT palette_g,
                     const float* RESTRICT palette_b, uint32_t palette_size,
                     float* RESTRICT distance2) {
  return CALL(chooseColor)(
      r, g, b, palette_r, palette_g, palette_b, palette_size, distance2);
}

void findBestSubdivision(Fragment* f, Cache* cache, const CodecParams& cp) {
  return CALL(findBestSubdivision)(f, cache, cp);
}

Vector<float>* gatherPatches(const Array<Fragment*>* partition,
                             uint32_t num_non_leaf) {
  return CALL(gatherPatches)(partition, num_non_leaf);
}

Vector<float>* buildPalette(const Vector<float>* patches,
                            uint32_t palette_size) {
  return CALL(buildPalette)(patches, palette_size);
}

}  // namespace twim
#endif  // HWY_ONCE
