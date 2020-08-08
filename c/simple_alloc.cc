#include <cstddef>
#include <cstdint>

#if defined(__wasm32__)

extern "C" {

namespace {

static uint32_t freeHeads[32] = {0};
static uint32_t freeTails[32] = {0};
static uint32_t freePages = 0;
static uint32_t freeStart = 0;
static uint32_t pageBuckets[65536];

}  // namespace

void* malloc(size_t size) {
  if (size < 4) size = 4;  // minimal size for reference to the next item
  uint32_t bucket = (__builtin_clz(size - 1) ^ 31) + 1;
  if (freeHeads[bucket] == 0 && freeTails[bucket] == 0) {
    uint32_t wantPages = (bucket <= 16) ? 1 : (1 << (bucket - 16));
    if (freePages < wantPages) {
      uint32_t currentPages = __builtin_wasm_memory_size(0);
      if (freePages == 0) freeStart = currentPages << 16;
      uint32_t plusPages = currentPages;  // Double the memory...
      if (plusPages > 256) plusPages = 256;  // ... not more than +16MB
      if (plusPages < wantPages - freePages) plusPages = wantPages - freePages;
      if (__builtin_wasm_memory_grow(0, plusPages) == -1) __builtin_trap();
      freePages += plusPages;
    }
    pageBuckets[freeStart >> 16] = bucket;
    freeTails[bucket] = freeStart;
    freeStart += wantPages << 16;
    freePages -= wantPages;
  }
  if (freeHeads[bucket] == 0) {
    freeHeads[bucket] = freeTails[bucket];
    freeTails[bucket] += 1 << bucket;
    if ((freeTails[bucket] & 0xFFFF) == 0) freeTails[bucket] = 0;
  }
  uint32_t result = freeHeads[bucket];
  freeHeads[bucket] = reinterpret_cast<uint32_t*>(result)[0];
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