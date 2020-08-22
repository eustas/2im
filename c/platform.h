#ifndef TWIM_PLATFORM
#define TWIM_PLATFORM

#include <cstddef>
#include <cstdint>
#include <cstdlib>

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

#if defined(__wasm__)
// malloc from simple_alloc.cc will already die on failure
#define mallocOrDie malloc
#else
static INLINE void* mallocOrDie(size_t size) {
  void* result = malloc(size);
  if (!result) __builtin_trap();
  return result;
}
#endif

namespace twim {

template <typename T>
struct Array {
  Array(size_t capacity) : capacity(capacity), size(0) {
    data = reinterpret_cast<T*>(mallocOrDie(capacity * sizeof(T)));
  }

  INLINE ~Array() { free(data); }

  static INLINE size_t elementSize() { return sizeof(T); }

  T* data;
  size_t capacity;
  size_t size;
};

void growArray(void** data, size_t* capacity, size_t elementSize);

#define MAYBE_GROW_ARRAY(A) \
  if ((A).size == (A).capacity) growArray(reinterpret_cast<void**>(&(A).data), \
                                          &(A).capacity, (A).elementSize());

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

uint32_t vecSize(uint32_t capacity);

template <typename T>
NOINLINE Vector<T>* allocVector(uint32_t capacity) {
  // In twim only (u)int32_t and float vectors are used.
  constexpr size_t N = kDefaultAlign / 4;
  static_assert(sizeof(T) == 4, "sizeot(T) must be 4");
  using V = Vector<T>;
  const uint32_t vector_capacity = vecSize(capacity);
  const uint32_t size = sizeof(V) + vector_capacity * sizeof(T);
  constexpr const size_t alignment = N * sizeof(T);
  uintptr_t memory = reinterpret_cast<uintptr_t>(mallocOrDie(size + alignment));
  uintptr_t aligned_memory =
      (memory + sizeof(V) + alignment - 1) & ~(alignment - 1);
  V* v = reinterpret_cast<V*>(aligned_memory - sizeof(V));
  v->offset = static_cast<uint32_t>(aligned_memory - memory);
  v->capacity = vector_capacity;
  return v;
}

}  // namespace twim

#endif  // TWIM_PLATFORM
