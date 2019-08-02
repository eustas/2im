#include "region.h"

#include "codec_params.h"
#include "distance_range.h"
#include "gtest/gtest.h"
#include "platform.h"
#include "sincos.h"

namespace twim {

template <size_t N>
void Set(Vector<int32_t>* to, const std::array<int32_t, N>& from) {
  std::copy(from.cbegin(), from.cend(), to->data());
  to->len = N / 3;
}

template <size_t N>
void ExpectEq(const std::array<int32_t, N>& expected, Vector<int32_t>* actual) {
  EXPECT_EQ(N / 3, actual->len);
  EXPECT_TRUE(0 == memcmp(expected.data(), actual->data(), N * sizeof(int)));
}

TEST(RegionTest, HorizontalSplit) {
  auto region = allocVector<int>(3);
  Set<3>(region.get(), {0, 0, 4});

  int32_t angle = SinCos::kMaxAngle / 2;
  CodecParams cp(4, 4);
  DistanceRange distanceRange;
  distanceRange.update(*region.get(), angle, cp);
  EXPECT_EQ(3, distanceRange.num_lines);
  auto left = allocVector<int32_t>(3);
  auto right = allocVector<int32_t>(3);

  // 1/3
  Region::splitLine(*region.get(), angle, distanceRange.distance(0), left.get(),
                    right.get());
  ExpectEq<3>({0, 1, 4}, left.get());
  ExpectEq<3>({0, 0, 1}, right.get());

  // 2/3
  Region::splitLine(*region.get(), angle, distanceRange.distance(1), left.get(),
                    right.get());
  ExpectEq<3>({0, 2, 4}, left.get());
  ExpectEq<3>({0, 0, 2}, right.get());

  // 3/3
  Region::splitLine(*region.get(), angle, distanceRange.distance(2), left.get(),
                    right.get());
  ExpectEq<3>({0, 3, 4}, left.get());
  ExpectEq<3>({0, 0, 3}, right.get());
}

TEST(RegionTest, VerticalSplit) {
  auto region = allocVector<int32_t>(12);
  Set<12>(region.get(), {0, 1, 2, 3, 0, 0, 0, 0, 1, 1, 1, 1});
  int32_t angle = 0;
  CodecParams cp(4, 4);
  cp.line_limit = 63;
  DistanceRange distanceRange;
  distanceRange.update(*region.get(), angle, cp);
  EXPECT_EQ(3, distanceRange.num_lines);

  // 1/3
  auto left = allocVector<int32_t>(9);
  auto right = allocVector<int32_t>(3);
  Region::splitLine(*region.get(), angle, distanceRange.distance(0), left.get(),
                    right.get());
  ExpectEq<9>({1, 2, 3, 0, 0, 0, 1, 1, 1}, left.get());
  ExpectEq<3>({0, 0, 1}, right.get());

  // 2/3
  left = allocVector<int32_t>(6);
  right = allocVector<int32_t>(6);
  Region::splitLine(*region.get(), angle, distanceRange.distance(1), left.get(),
                    right.get());
  ExpectEq<6>({2, 3, 0, 0, 1, 1}, left.get());
  ExpectEq<6>({0, 1, 0, 0, 1, 1}, right.get());

  // 3/3
  left = allocVector<int32_t>(3);
  right = allocVector<int32_t>(9);
  Region::splitLine(*region.get(), angle, distanceRange.distance(2), left.get(),
                    right.get());
  ExpectEq<3>({3, 0, 1}, left.get());
  ExpectEq<9>({0, 1, 2, 0, 0, 0, 1, 1, 1}, right.get());
}

}  // namespace twim
