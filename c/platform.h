#ifndef TWIM_PLATFORM
#define TWIM_PLATFORM

#include <immintrin.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

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

namespace {

constexpr const int32_t kAlign = 64;

template <typename T>
using Deleter = void (*)(T*);

template <typename V>
void destroyVector(V* v) {
  void* memory = v->memory;
  free(memory);
}

}  // namespace

template <typename T>
using Owned = std::unique_ptr<T, Deleter<T>>;

template <typename T>
struct Vector {
  // System (const) fields.
  void* memory;
  T* data;
  uint32_t capacity;

  // User fields.
  uint32_t len;
};

template <typename Lane, size_t kLanes>
struct Desc {
  constexpr Desc() = default;
  using T = Lane;
  static constexpr size_t N = kLanes;
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
};

template <typename T>
using VecTag = Desc<T, 32 / sizeof(T)>;

template <typename T>
static size_t vecSize(size_t capacity) {
  constexpr auto vi32 = VecTag<T>();
  int32_t N = vi32.N;
  return (capacity + N - 1) & ~(N - 1);
}

template <typename T>
Owned<Vector<T>> allocVector(size_t capacity) {
  using V = Vector<T>;
  size_t size = capacity * sizeof(T);
  static_assert(sizeof(V) < (kAlign / 2), "V is too long");
  uintptr_t memory = reinterpret_cast<uintptr_t>(malloc(size + kAlign));
  uintptr_t aligned_memory = (memory + kAlign - 1) & ~(kAlign - 1);
  int32_t before = aligned_memory - memory;
  V* v;
  if (before >= sizeof(V)) {
    v = reinterpret_cast<V*>(aligned_memory - sizeof(V));
  } else {
    v = reinterpret_cast<V*>(aligned_memory + size);
  }
  v->memory = reinterpret_cast<void*>(memory);
  v->data = reinterpret_cast<T*>(aligned_memory);
  v->capacity = capacity;
  return {v, destroyVector};
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> NanoTime;

INLINE NanoTime now() { return std::chrono::high_resolution_clock::now(); }

INLINE double duration(NanoTime t0, NanoTime t1) {
  const auto delta =
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
  return delta.count() / 1000000.0;
}

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
