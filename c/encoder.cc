#include "encoder.h"

#include "platform.h"

namespace twim {

class Cache;

class StatsCache {
 public:
  int32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<float>> x0;
  Owned<Vector<float>> x1;
  Owned<Vector<float>> x;

  int32_t plus[8];
  int32_t minus[8];

  template <typename T>
  static Owned<Vector<T>> alloc(size_t len) {
    VecTag<int32_t> d_i32;
    constexpr size_t n_i32 = d_i32.N;
    return allocVector<T>((len + n_i32 - 1) & ~(n_i32 - 1));
  }

  StatsCache(int height)
      : row_offset(alloc<int32_t>(height)),
        y(alloc<float>(height)),
        x0(alloc<float>(height)),
        x1(alloc<float>(height)),
        x(alloc<float>(height)) {}

  StatsCache(const StatsCache& other)
      : StatsCache(other.row_offset->capacity) {}

  static void sum(const Cache& cache, float* RESTRICT regionX, int dst[8]) {
    /*
    StatsCache statsCache = cache.statsCache;
    int count = statsCache.count;
    int[] rowOffset = statsCache.rowOffset;
    int[] sumR = cache.sumR;
    int[] sumG = cache.sumG;
    int[] sumB = cache.sumB;
    int[] sumR2 = cache.sumR2;
    int[] sumG2 = cache.sumG2;
    int[] sumB2 = cache.sumB2;
    int pixelCount = 0, r = 0, g = 0, b = 0, r2 = 0, g2 = 0, b2 = 0;

    for (int i = 0; i < count; i++) {
      int x = (int) regionX[i];
      int offset = rowOffset[i] + x;
      pixelCount += x;
      r += sumR[offset];
      g += sumG[offset];
      b += sumB[offset];
      r2 += sumR2[offset];
      g2 += sumG2[offset];
      b2 += sumB2[offset];
    }

    dst[0] = pixelCount;
    dst[1] = r;
    dst[2] = g;
    dst[3] = b;
    dst[4] = r2;
    dst[5] = g2;
    dst[6] = b2;
    */
  }

  void prepare(const Cache& cache, Vector<int32_t>* region) {
    size_t count = region->len;
    size_t step = region->capacity / 3;
    size_t sum_stride = cache.sum_stride;
    int32_t* RESTRICT data = region->data;
    float* RESTRICT y = this->y->data;
    float* RESTRICT x0 = this->x0->data;
    float* RESTRICT x1 = this->x1->data;
    int32_t* RESTRICT row_offset = this->row_offset->data;
    for (size_t i = 0; i < count; ++i) {
      int32_t row = data[i];
      y[i] = row;
      x0[i] = data[step + i];
      x1[i] = data[2 * step + i];
      row_offset[i] = row * sum_stride;
    }
    this->count = count;
    sum(cache, x1, plus);
  }
};

class Cache {
 public:
  const int32_t sum_stride;
};

}  // namespace twim
