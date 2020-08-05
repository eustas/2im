#ifndef TWIM_PLATFORM
#define TWIM_PLATFORM

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

// TODO(eustas): move to common header
// TODO(eustas): adjust to SIMD implementation
constexpr size_t kDefaultAlign = 32;

static INLINE uint32_t vecSize(uint32_t capacity) {
  // In twim only (u)int32_t and float vectors are used.
  constexpr size_t N = kDefaultAlign / 4;
  return static_cast<uint32_t>((capacity + N - 1) & ~(N - 1));
}

template <typename T>
Owned<Vector<T>> allocVector(uint32_t capacity) {
  // In twim only (u)int32_t and float vectors are used.
  constexpr size_t N = kDefaultAlign / 4;
  static_assert(sizeof(T) == 4, "sizeot(T) must be 4");
  using V = Vector<T>;
  const uint32_t vector_capacity = vecSize(capacity);
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

}  // namespace twim

#endif  // TWIM_PLATFORM
