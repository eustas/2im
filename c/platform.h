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

#define TRAP()                     \
  {                                \
    uint8_t* _trap_ptr_ = nullptr; \
    _trap_ptr_[0] = 0;             \
  }

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
static uint32_t vecSize(const Desc<T, N>& /* tag */, uint32_t capacity) {
  return static_cast<uint32_t>((capacity + N - 1) & ~(N - 1));
}

template <typename T, size_t kAlign>
static uint32_t vecSize(uint32_t capacity) {
  constexpr const Desc<T, kAlign / sizeof(T)> tag;
  return vecSize(tag, capacity);
}

template <typename T, size_t N>
Owned<Vector<T>> allocVector(const Desc<T, N>& tag, uint32_t capacity) {
  using V = Vector<T>;
  const uint32_t vector_capacity = vecSize(tag, capacity);
  const uint32_t size = sizeof(V) + vector_capacity * sizeof(T);
  constexpr const size_t alignment = N * sizeof(T);
  uintptr_t memory = reinterpret_cast<uintptr_t>(malloc(size + alignment));
  uintptr_t aligned_memory =
      (memory + sizeof(V) + alignment - 1) & ~(alignment - 1);
  V* v = reinterpret_cast<V*>(aligned_memory - sizeof(V));
  v->offset = static_cast<uint32_t>(aligned_memory - memory);
  v->capacity = vector_capacity;
  return {v, destroyVector};
}

template <typename T, size_t kAlign>
Owned<Vector<T>> allocVector(uint32_t capacity) {
  constexpr const Desc<T, kAlign / sizeof(T)> tag;
  return allocVector(tag, capacity);
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> NanoTime;

INLINE NanoTime now() { return std::chrono::high_resolution_clock::now(); }

INLINE double duration(NanoTime t0, NanoTime t1) {
  const auto delta =
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
  return delta.count() / 1000000.0;
}

// load

SIMD INLINE __m128i load(const Desc<int32_t, 4>& /* tag */,
                         const int32_t* RESTRICT ptr) {
  return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}

SIMD INLINE __m128 load(const Desc<float, 4>& /* tag */,
                        const float* RESTRICT ptr) {
  return _mm_load_ps(ptr);
}

// store

SIMD INLINE void store(const __m128 v, const Desc<float, 4>& /* tag */,
                       float* RESTRICT ptr) {
  _mm_store_ps(ptr, v);
}

SIMD INLINE void store(const __m128i v, const Desc<int32_t, 4>& /* tag */,
                       int32_t* RESTRICT ptr) {
  _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
}

// zero

SIMD INLINE __m128 zero(const Desc<float, 4>& /* tag */) {
  return _mm_setzero_ps();
}

SIMD INLINE __m128i zero(const Desc<int32_t, 4>& /* tag */) {
  return _mm_setzero_si128();
}

// set1

SIMD INLINE __m128 set1(const Desc<float, 4>& /* tag */, const float t) {
  return _mm_set1_ps(t);
}

SIMD INLINE __m128i set1(const Desc<int32_t, 4>& /* tag */, const int32_t t) {
  return _mm_set1_epi32(t);
}

// add

SIMD INLINE __m128 add(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_add_ps(a, b);
}

SIMD INLINE __m128i add(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                        const __m128i b) {
  return _mm_add_epi32(a, b);
}

// sub

SIMD INLINE __m128i sub(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                        const __m128i b) {
  return _mm_sub_epi32(a, b);
}

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

SIMD INLINE __m128i mul(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                        const __m128i b) {
  return _mm_mullo_epi32(a, b);
}

// mul_add

SIMD INLINE __m128 mul_add(const Desc<float, 4>& vf, const __m128 a,
                           const __m128 b, const __m128 c) {
  return add(vf, mul(vf, a, b), c);
}

// cast

SIMD INLINE __m128i cast(const Desc<float, 4>& /* from */,
                         const Desc<int32_t, 4>& /* to */, const __m128 a) {
  return _mm_cvttps_epi32(a);
}

// max

SIMD INLINE __m128 max(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_max_ps(a, b);
}

SIMD INLINE __m128i max(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                        const __m128i b) {
  return _mm_max_epi32(a, b);
}

// min

SIMD INLINE __m128 min(const Desc<float, 4>& /* tag */, const __m128 a,
                       const __m128 b) {
  return _mm_min_ps(a, b);
}

SIMD INLINE __m128i min(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                        const __m128i b) {
  return _mm_min_epi32(a, b);
}

// and

SIMD INLINE __m128i bit_and(const Desc<int32_t, 4>& /* tag */, const __m128i a,
                            const __m128i b) {
  return _mm_and_si128(a, b);
}

// round

SIMD INLINE __m128 round(const Desc<float, 4>& /* tag */, const __m128 a) {
  return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

// hadd

// a1 a2 a3 a4 $ b1 b2 b3 b4 -> a1+a2 a3+a4 b1+b2 b3+b4
SIMD INLINE __m128 hadd(const Desc<float, 4>& /* tag */, const __m128 a,
                        const __m128 b) {
  return _mm_hadd_ps(a, b);
}

// blend

// a1 a2 a3 a4 $ b1 b2 b3 b4 -> a1?b1 a2?b2 a3?b3 a4?b4
template <int Selector>
SIMD INLINE __m128 blend(const Desc<float, 4>& /* tag */, const __m128 a,
                         const __m128 b) {
  return _mm_blend_ps(a, b, Selector);
}

// broadcast

// a0 a1 a2 a3 $ a0 a1 a2 a3 -> ai ai ai ai
template <int Lane>
SIMD INLINE __m128 broadcast(const Desc<float, 4>& /* tag */, const __m128 a) {
  return _mm_shuffle_ps(a, a, Lane * 0x55);
}

// reduce

SIMD INLINE float reduce(const Desc<float, 4>& /* tag */, const __m128 a_b_c_d) {
  const auto b_b_d_d = _mm_movehdup_ps(a_b_c_d);
  const auto ab_x_cd_x = _mm_add_ps(a_b_c_d, b_b_d_d);
  const auto cd_x_x_x = _mm_movehl_ps(b_b_d_d, ab_x_cd_x);
  const auto abcd_x_x_x = _mm_add_ss(ab_x_cd_x, cd_x_x_x);
  return _mm_cvtss_f32(abcd_x_x_x);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
