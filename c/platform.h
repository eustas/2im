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

// SSE: 128 bits = 16 bytes = 4 floats / int32_t
#define ALIGNED_16 alignas(16)

// AVX: 256 bits = 32 bytes = 8 floats / int32_t
#define ALIGNED_32 alignas(32)

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

#define TRAP() { uint8_t* _trap_ptr_ = nullptr; _trap_ptr_[0] = 0; }

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
using AvxVecTag = Desc<T, 32 / sizeof(T)>;

template <typename T>
using SseVecTag = Desc<T, 16 / sizeof(T)>;

template <typename T, size_t N>
static size_t vecSize(const Desc<T, N>& /* tag */, size_t capacity) {
  return (capacity + N - 1) & ~(N - 1);
}

template <typename T, size_t N>
Owned<Vector<T>> allocVector(const Desc<T, N>& tag, size_t capacity) {
  using V = Vector<T>;
  const size_t vector_capacity = vecSize(tag, capacity);
  const size_t size = sizeof(V) + vector_capacity * sizeof(T);
  constexpr const size_t alignment = N * sizeof(T);
  uintptr_t memory = reinterpret_cast<uintptr_t>(malloc(size + alignment));
  uintptr_t aligned_memory =
      (memory + sizeof(V) + alignment - 1) & ~(alignment - 1);
  V* v = reinterpret_cast<V*>(aligned_memory - sizeof(V));
  v->offset = static_cast<uint32_t>(aligned_memory - memory);
  v->capacity = vector_capacity;
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
SIMD INLINE __m256i load(const Desc<T, 8>& /* tag */, const T* RESTRICT ptr) {
  return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
}

SIMD INLINE __m256 load(const Desc<float, 8>& /* tag */,
                        const float* RESTRICT ptr) {
  return _mm256_load_ps(ptr);
}

SIMD INLINE __m128 load(const Desc<float, 4>& /* tag */,
                        const float* RESTRICT ptr) {
  return _mm_load_ps(ptr);
}

// store

template <typename T>
SIMD INLINE void store(const __m256i v, const Desc<T, 8>& /* tag */,
                       T* RESTRICT ptr) {
  _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
}

SIMD INLINE void store(const __m256 v, const Desc<float, 8>& /* tag */,
                       float* RESTRICT ptr) {
  _mm256_store_ps(ptr, v);
}

SIMD INLINE void store(const __m128 v, const Desc<float, 4>& /* tag */,
                       float* RESTRICT ptr) {
  _mm_store_ps(ptr, v);
}

// zero

template <typename T>
SIMD INLINE __m256i zero(const Desc<T, 8>& /* tag */) {
  return _mm256_setzero_si256();
}

SIMD INLINE __m256 zero(const Desc<float, 8>& /* tag */) {
  return _mm256_setzero_ps();
}

SIMD INLINE __m128 zero(const Desc<float, 4>& /* tag */) {
  return _mm_setzero_ps();
}

// set1

SIMD INLINE __m256 set1(const Desc<float, 8>& /* tag */, const float t) {
  return _mm256_set1_ps(t);
}

SIMD INLINE __m128 set1(const Desc<float, 4>& /* tag */, const float t) {
  return _mm_set1_ps(t);
}

// add

SIMD INLINE __m256i add(const Desc<int32_t, 8>& /* tag */, const __m256i a,
                        const __m256i b) {
  return _mm256_add_epi32(a, b);
}

SIMD INLINE __m256 add(const Desc<float, 8>& /* tag */, const __m256 a,
                       const __m256 b) {
  return _mm256_add_ps(a, b);
}

SIMD INLINE __m128 add(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_add_ps(a, b);
}

// sub

SIMD INLINE __m128 sub(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_sub_ps(a, b);
}

// div

SIMD INLINE __m128 div(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_div_ps(a, b);
}

// mul

SIMD INLINE __m128 mul(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_mul_ps(a, b);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
