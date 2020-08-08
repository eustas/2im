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

namespace twim {

template <typename T>
struct Array {
  inline Array(size_t capacity) : capacity(capacity), size(0) {
    data = reinterpret_cast<T*>(malloc(capacity * sizeof(T)));
  }

  INLINE ~Array() { free(data); }

  T* data;
  size_t capacity;
  size_t size;
};

#define CHECK_ARRAY_CAN_GROW(A)
//#define CHECK_ARRAY_CAN_GROW(A) if ((A).size >= (A).capacity) __builtin_trap();

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

  NOINLINE void operator delete(void* ptr) {
    Vector<T>* v = reinterpret_cast<Vector<T>*>(ptr);
    uintptr_t memory = (uintptr_t)v->data() - v->offset;
    free((void*)memory);
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
NOINLINE std::unique_ptr<Vector<T>> allocVector(uint32_t capacity) {
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
  return std::unique_ptr<Vector<T>>(v);
}

}  // namespace twim

#endif  // TWIM_PLATFORM
