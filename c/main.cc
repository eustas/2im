#include <cstdio>

#include "platform.h"

namespace twim {

SIMD void main() {
  constexpr auto vf = VecTag<float>();
  fprintf(stderr, "Lanes: %zu\n", vf.N);
  auto t0 = now();
  auto v = allocVector<float>(vf.N * 2);
  float* RESTRICT vd = v->data;
  const auto a = load(vf, vd);
  store(a, vf, vd + vf.N);
  auto t1 = now();
  fprintf(stderr, "%0.3fs\n", duration(t0, t1));
}

}  // namespace twim

int32_t main() {
  twim::main();
  return 0;
}
