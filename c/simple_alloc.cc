#include <cstddef>
#include <cstdint>

#if defined(__wasm32__)

extern "C" {

namespace {

static uint32_t freeHeads[32] = {0};
static uint32_t freeTails[32] = {0};
static uint32_t freePages = 0;
static uint32_t nextPage = 0;
static uint32_t pageBuckets[65536];

}  // namespace

void* malloc(size_t size) {
  if (size < 4) size = 4;
  uint32_t bucket = (__builtin_clz(size - 1) ^ 31) + 1;
  if (freeHeads[bucket] == 0 && freeTails[bucket] == 0) {
    uint32_t wantPages = (bucket <= 16) ? 1 : (1 << (bucket - 16));
    if (freePages < wantPages) {
      uint32_t currentPages = __builtin_wasm_memory_size(0);
      if (freePages == 0) nextPage = currentPages;
      uint32_t plusPages = currentPages;
      if (plusPages > 256) plusPages = 256;
      if (plusPages < wantPages - freePages) plusPages = wantPages - freePages;
      if (__builtin_wasm_memory_grow(0, plusPages) == -1) __builtin_trap();
      freePages += plusPages;
    }
    pageBuckets[nextPage] = bucket;
    freeTails[bucket] = nextPage << 16;
    nextPage += wantPages;
    freePages -= wantPages;
  }
  uint32_t result;
  if (freeHeads[bucket] == 0) {
    result = freeTails[bucket];
    freeTails[bucket] += 1 << bucket;
    if ((freeTails[bucket] & 0xFFFF) == 0) freeTails[bucket] = 0;
  } else {
    result = freeHeads[bucket];
    freeHeads[bucket] = reinterpret_cast<uint32_t*>(result)[0];
  }
  return reinterpret_cast<void*>(result);
}

void free(void* ptr) {
  uint32_t p = reinterpret_cast<uint32_t>(ptr);
  size_t bucket = pageBuckets[p >> 16];
  reinterpret_cast<uint32_t*>(p)[0] = freeHeads[bucket];
  freeHeads[bucket] = p;
}

}  // extern "C"

#endif  // __wasm32__