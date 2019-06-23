#include <cstdio>

#include "platform.h"

namespace twim {

SIMD void main() {
  auto t0 = now();
  float xxx[16] = {};
  constexpr auto vf = VecTag<float>();
  const auto a = load(vf, xxx);
  store(a, vf, xxx + 8);
  auto t1 = now();
  fprintf(stderr, "%0.3fs\n", duration(t0, t1));
}

}  // namespace twim

int main() {
  twim::main();
  return 0;
}
