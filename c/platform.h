#ifndef TWIM_PLATFORM
#define TWIM_PLATFORM

#include <chrono>
#include <cstddef>
#include <cstdint>

#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#define SIMD_RESTRICT __restrict
#define SIMD_INLINE __forceinline
#else
#define RESTRICT __restrict__
#define INLINE inline __attribute__((always_inline)) __attribute__((flatten))
#endif

#define SIMD __attribute__((target("avx,avx2,fma")))

#define ALIGNED alignas(64)

namespace twim {

typedef std::chrono::time_point<std::chrono::high_resolution_clock> NanoTime;

NanoTime now() { return std::chrono::high_resolution_clock::now(); }

double duration(NanoTime t0, NanoTime t1) {
  return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000000.0;
}

template <typename Lane, size_t kLanes>
struct Desc {
  constexpr Desc() = default;
  using T = Lane;
  static constexpr size_t N = kLanes;
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
};

template <typename T>
using VecTag = Desc<T, 32 / sizeof(T)>;

// load

template <typename T>
SIMD INLINE __m256i load(VecTag<T>, const T* RESTRICT ptr) {
  return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
}
SIMD INLINE __m256 load(VecTag<float>, const float* RESTRICT ptr) {
  return _mm256_load_ps(ptr);
}

// store

template <typename T>
SIMD INLINE void store(const __m256i v, VecTag<T>, T* RESTRICT ptr) {
  _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
}
SIMD INLINE void store(const __m256 v, VecTag<float>, float* RESTRICT ptr) {
  _mm256_store_ps(ptr, v);
}

// set1

SIMD INLINE __m256 set1(VecTag<float>, const float t) {
  return _mm256_set1_ps(t);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
