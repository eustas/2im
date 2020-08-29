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

}  // namespace twim
