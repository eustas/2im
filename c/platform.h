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

#define HWY_ALIGN ALIGNED_16

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
using SseVecTag = Desc<T, 16 / sizeof(T)>;

#define HWY_FULL(T) SseVecTag<T>();
#define Lanes(D) (D.N)

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

SIMD INLINE __m128i Load(const Desc<int32_t, 4>& /* tag */,
                         const int32_t* RESTRICT ptr) {
  return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}

SIMD INLINE __m128 Load(const Desc<float, 4>& /* tag */,
                        const float* RESTRICT ptr) {
  return _mm_load_ps(ptr);
}

// store

SIMD INLINE void Store(const __m128 v, const Desc<float, 4>& /* tag */,
                       float* RESTRICT ptr) {
  _mm_store_ps(ptr, v);
}

SIMD INLINE void Store(const __m128i v, const Desc<int32_t, 4>& /* tag */,
                       int32_t* RESTRICT ptr) {
  _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
}

// zero

SIMD INLINE __m128 Zero(const Desc<float, 4>& /* tag */) {
  return _mm_setzero_ps();
}

SIMD INLINE __m128i Zero(const Desc<int32_t, 4>& /* tag */) {
  return _mm_setzero_si128();
}

// set1

SIMD INLINE __m128 Set(const Desc<float, 4>& /* tag */, const float t) {
  return _mm_set1_ps(t);
}

SIMD INLINE __m128i Set(const Desc<int32_t, 4>& /* tag */, const int32_t t) {
  return _mm_set1_epi32(t);
}

// add

SIMD INLINE __m128 Add(const __m128 a,
                       const __m128 b) {
  return _mm_add_ps(a, b);
}

SIMD INLINE __m128i AddI(const __m128i a,
                        const __m128i b) {
  return _mm_add_epi32(a, b);
}

// sub

SIMD INLINE __m128 Sub(const __m128 a,
                       const __m128 b) {
  return _mm_sub_ps(a, b);
}

// div

SIMD INLINE __m128 Div(const __m128 a,
                       const __m128 b) {
  return _mm_div_ps(a, b);
}

// mul

SIMD INLINE __m128 Mul(const __m128 a,
                       const __m128 b) {
  return _mm_mul_ps(a, b);
}

SIMD INLINE __m128i Mul(const __m128i a,
                        const __m128i b) {
  return _mm_mullo_epi32(a, b);
}

// mul_add

SIMD INLINE __m128 MulAdd(const __m128 a,
                         const __m128 b, const __m128 c) {
  return Add(Mul(a, b), c);
}

// cast

SIMD INLINE __m128i ConvertTo(const Desc<int32_t, 4>&, const __m128 a) {
  return _mm_cvttps_epi32(a);
}

SIMD INLINE __m128 ConvertTo(const Desc<float, 4>&, const __m128i a) {
  return _mm_cvtepi32_ps(a);
}

SIMD INLINE __m128 BitCast(const Desc<float, 4>&, const __m128i a) {
  return (__m128)a;
}

// max

SIMD INLINE __m128 Max(const __m128 a,
                       const __m128 b) {
  return _mm_max_ps(a, b);
}

SIMD INLINE __m128i Max(const __m128i a,
                        const __m128i b) {
  return _mm_max_epi32(a, b);
}

// min

SIMD INLINE __m128 Min(const __m128 a,
                       const __m128 b) {
  return _mm_min_ps(a, b);
}

SIMD INLINE __m128i Min(const __m128i a,
                        const __m128i b) {
  return _mm_min_epi32(a, b);
}

// and

SIMD INLINE __m128i And(const __m128i a,
                        const __m128i b) {
  return _mm_and_si128(a, b);
}

// broadcast

// a0 a1 a2 a3 $ a0 a1 a2 a3 -> ai ai ai ai
template <int Lane>
SIMD INLINE __m128 Broadcast(const __m128 a) {
  return _mm_shuffle_ps(a, a, Lane * 0x55);
}

// reduce

SIMD INLINE float SumOfLanes(const __m128 a_b_c_d) {
  const auto b_b_d_d = _mm_movehdup_ps(a_b_c_d);
  const auto ab_x_cd_x = _mm_add_ps(a_b_c_d, b_b_d_d);
  const auto cd_x_x_x = _mm_movehl_ps(b_b_d_d, ab_x_cd_x);
  const auto abcd_x_x_x = _mm_add_ss(ab_x_cd_x, cd_x_x_x);
  return _mm_cvtss_f32(abcd_x_x_x);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
