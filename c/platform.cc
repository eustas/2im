#include "platform.h"

#include <cstring>

namespace twim {

void growArray(void** data, size_t* capacity, size_t elementSize) {
  size_t initialCapacity = *capacity;
  void* initialData = *data;
  size_t oldSize = initialCapacity * elementSize;
  void* newData = mallocOrDie(2 * oldSize);
  memcpy(newData, initialData, oldSize);
  free(initialData);
  *capacity = 2 * initialCapacity;
  *data = newData;
}

uint32_t vecSize(uint32_t capacity) {
  // In twim only (u)int32_t and float vectors are used.
  constexpr size_t N = kDefaultAlign / 4;
  return static_cast<uint32_t>((capacity + N - 1) & ~(N - 1));
}

}  // namespace twim
