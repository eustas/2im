#ifndef TWIM_PLATFORM
#define TWIM_PLATFORM

#include <immintrin.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

#ifdef _MSC_VER
#include <intrin.h>
#define RESTRICT __restrict
#define INLINE __forceinline
#define NOINLINE
#else
#define RESTRICT __restrict__
#define INLINE inline __attribute__((always_inline)) __attribute__((flatten))
#define NOINLINE __attribute__((noinline))
#endif

#define SIMD __attribute__((target("avx,avx2,fma")))

// AVX: 256 bits = 32 bytes = 8 floats / int32_t
#define ALIGNMENT 32
#define ALIGNED alignas(ALIGNMENT)

namespace twim {

namespace {

template <typename T>
using Deleter = void (*)(T*);

template <typename V>
void destroyVector(V* v) {
  uintptr_t memory = (uintptr_t)v->data() - v->offset;
  free((void*)memory);
}

}  // namespace

template <typename T>
using Owned = std::unique_ptr<T, Deleter<T>>;

template <typename T>
struct Vector {
  uint32_t offset;
  uint32_t capacity;
  uint32_t len;

  INLINE T* RESTRICT data() {
    return (T*)((uintptr_t)this + sizeof(Vector<T>));
  }
  INLINE const T* RESTRICT data() const {
    return (const T*)((uintptr_t)this + sizeof(Vector<T>));
  }
};

template <typename Lane, size_t kLanes>
struct Desc {
  constexpr Desc() = default;
  using T = Lane;
  static constexpr size_t N = kLanes;
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
};

template <typename T>
using VecTag = Desc<T, ALIGNMENT / sizeof(T)>;

template <typename T>
static size_t vecSize(size_t capacity) {
  constexpr auto vt = VecTag<T>();
  int32_t N = vt.N;
  return (capacity + N - 1) & ~(N - 1);
}

template <typename T>
Owned<Vector<T>> allocVector(size_t capacity) {
  using V = Vector<T>;
  size_t size = sizeof(V) + capacity * sizeof(T);
  uintptr_t memory = reinterpret_cast<uintptr_t>(malloc(size + ALIGNMENT));
  uintptr_t aligned_memory =
      (memory + sizeof(V) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
  V* v = reinterpret_cast<V*>(aligned_memory - sizeof(V));
  v->offset = static_cast<uint32_t>(aligned_memory - memory);
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

// zero

template <typename T>
SIMD INLINE __m256i zero(VecTag<T>) {
  return _mm256_setzero_si256();
}

SIMD INLINE __m256 zero(VecTag<float>) {
  return _mm256_setzero_ps();
}

// set1

SIMD INLINE __m256 set1(VecTag<float>, const float t) {
  return _mm256_set1_ps(t);
}

// add

SIMD INLINE __m256i add(VecTag<int32_t>, const __m256i a, const __m256i b) {
  return _mm256_add_epi32(a, b);
}

SIMD INLINE __m256 add(VecTag<float>, const __m256 a, const __m256 b) {
  return _mm256_add_ps(a, b);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
