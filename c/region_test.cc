#include "region.h"

#include "codec_params.h"
#include "distance_range.h"
#include "gtest/gtest.h"
#include "platform.h"
#include "sin_cos.h"

namespace twim {

template <size_t N>
void Set(Vector<int32_t>* to, const std::array<int32_t, N>& from) {
  size_t count = N / 3;
  size_t step = to->capacity / 3;
  int32_t* RESTRICT y = to->data();
  int32_t* RESTRICT x0 = y + step;
  int32_t* RESTRICT x1 = x0 + step;
  for (size_t i = 0; i < count; i++) {
    y[i] = from.cbegin()[3 * i];
    x0[i] = from.cbegin()[3 * i + 1];
    x1[i] = from.cbegin()[3 * i + 2];
  }
  to->len = count;
}

template <size_t N>
void ExpectEq(const std::array<int32_t, N>& expected,
              const Vector<int32_t>* actual) {
  size_t count = N / 3;
  size_t step = actual->capacity / 3;
  const int32_t* RESTRICT y = actual->data();
  const int32_t* RESTRICT x0 = y + step;
  const int32_t* RESTRICT x1 = x0 + step;
  EXPECT_EQ(count, actual->len);
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(expected.data()[3 * i], y[i]);
    EXPECT_EQ(expected.data()[3 * i + 1], x0[i]);
    EXPECT_EQ(expected.data()[3 * i + 2], x1[i]);
  }
}

TEST(RegionTest, HorizontalSplit) {
  int32_t step1 = vecSize(1);
  auto region = allocVector<int32_t>(3 * step1);
  Set<3>(region.get(), {0, 0, 4});

  int32_t angle = SinCos.kMaxAngle / 2;
  CodecParams cp(4, 4);
  DistanceRange distanceRange(*region.get(), angle, cp);
  EXPECT_EQ(3, distanceRange.num_lines);
  auto left = allocVector<int32_t>(3 * step1);
  auto right = allocVector<int32_t>(3 * step1);

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
  int32_t step1 = vecSize(1);
  int32_t step2 = vecSize(2);
  int32_t step3 = vecSize(3);
  int32_t step4 = vecSize(4);
  auto region = allocVector<int32_t>(3 * step4);
  Set<12>(region.get(), {0, 0, 1, /**/ 1, 0, 1, /**/ 2, 0, 1, /**/ 3, 0, 1});
  int32_t angle = 0;
  CodecParams cp(4, 4);
  cp.line_limit = 63;
  DistanceRange distanceRange(*region.get(), angle, cp);
  EXPECT_EQ(3, distanceRange.num_lines);

  // 1/3
  auto left = allocVector<int32_t>(3 * step3);
  auto right = allocVector<int32_t>(3 * step1);
  Region::splitLine(*region.get(), angle, distanceRange.distance(0), left.get(),
                    right.get());
  ExpectEq<9>({1, 0, 1, /**/ 2, 0, 1, /**/ 3, 0, 1}, left.get());
  ExpectEq<3>({0, 0, 1}, right.get());

  // 2/3
  left = allocVector<int32_t>(3 * step2);
  right = allocVector<int32_t>(3 * step2);
  Region::splitLine(*region.get(), angle, distanceRange.distance(1), left.get(),
                    right.get());
  ExpectEq<6>({2, 0, 1, /**/ 3, 0, 1}, left.get());
  ExpectEq<6>({0, 0, 1, /**/ 1, 0, 1}, right.get());

  // 3/3
  left = allocVector<int32_t>(3 * step1);
  right = allocVector<int32_t>(3 * step3);
  Region::splitLine(*region.get(), angle, distanceRange.distance(2), left.get(),
                    right.get());
  ExpectEq<3>({3, 0, 1}, left.get());
  ExpectEq<9>({0, 0, 1, /**/ 1, 0, 1, /**/ 2, 0, 1}, right.get());
}

}  // namespace twim
