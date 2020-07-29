// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// 512-bit AVX512 vectors and operations. External include guard.
// WARNING: most operations do not cross 128-bit block boundaries. In
// particular, "Broadcast", pack and zip behavior may be surprising.

// This header is included by begin_target-inl.h, possibly inside a namespace,
// so do not include system headers (already done by highway.h). HWY_ALIGN is
// already defined unless an IDE is only parsing this file, in which case we
// include headers to avoid warnings (before including ops/x86_256-inl.h so it
// doesn't overwrite HWY_TARGET) .
#ifndef HWY_ALIGN
#include <stddef.h>
#include <stdint.h>

#include "hwy/highway.h"

#define HWY_NESTED_BEGIN  // prevent re-including this header
#undef HWY_TARGET
#define HWY_TARGET HWY_AVX3
#include "hwy/begin_target-inl.h"
#endif  // HWY_ALIGN

// Required for promotion/demotion and users of HWY_CAPPED.
#include "hwy/ops/x86_256-inl.h"

template <typename T>
struct Raw512 {
  using type = __m512i;
};
template <>
struct Raw512<float> {
  using type = __m512;
};
template <>
struct Raw512<double> {
  using type = __m512d;
};

template <typename T>
using Full512 = hwy::Simd<T, 64 / sizeof(T)>;

template <typename T, size_t N>
using Simd = hwy::Simd<T, N>;

template <typename T>
class Vec512 {
  using Raw = typename Raw512<T>::type;

 public:
  // Compound assignment. Only usable if there is a corresponding non-member
  // binary operator overload. For example, only f32 and f64 support division.
  HWY_API Vec512& operator*=(const Vec512 other) {
    return *this = (*this * other);
  }
  HWY_API Vec512& operator/=(const Vec512 other) {
    return *this = (*this / other);
  }
  HWY_API Vec512& operator+=(const Vec512 other) {
    return *this = (*this + other);
  }
  HWY_API Vec512& operator-=(const Vec512 other) {
    return *this = (*this - other);
  }
  HWY_API Vec512& operator&=(const Vec512 other) {
    return *this = (*this & other);
  }
  HWY_API Vec512& operator|=(const Vec512 other) {
    return *this = (*this | other);
  }
  HWY_API Vec512& operator^=(const Vec512 other) {
    return *this = (*this ^ other);
  }

  Raw raw;
};

// Template arg: sizeof(lane type)
template <size_t size>
struct RawMask512 {};
template <>
struct RawMask512<1> {
  using type = __mmask64;
};
template <>
struct RawMask512<2> {
  using type = __mmask32;
};
template <>
struct RawMask512<4> {
  using type = __mmask16;
};
template <>
struct RawMask512<8> {
  using type = __mmask8;
};

// Mask register: one bit per lane.
template <typename T>
class Mask512 {
  using Raw = typename RawMask512<sizeof(T)>::type;

 public:
  Raw raw;
};

// ------------------------------ Cast

HWY_API __m512i BitCastToInteger(__m512i v) { return v; }
HWY_API __m512i BitCastToInteger(__m512 v) { return _mm512_castps_si512(v); }
HWY_API __m512i BitCastToInteger(__m512d v) { return _mm512_castpd_si512(v); }

template <typename T>
HWY_API Vec512<uint8_t> cast_to_u8(Vec512<T> v) {
  return Vec512<uint8_t>{BitCastToInteger(v.raw)};
}

// Cannot rely on function overloading because return types differ.
template <typename T>
struct BitCastFromInteger512 {
  HWY_API __m512i operator()(__m512i v) { return v; }
};
template <>
struct BitCastFromInteger512<float> {
  HWY_API __m512 operator()(__m512i v) { return _mm512_castsi512_ps(v); }
};
template <>
struct BitCastFromInteger512<double> {
  HWY_API __m512d operator()(__m512i v) { return _mm512_castsi512_pd(v); }
};

template <typename T>
HWY_API Vec512<T> cast_u8_to(Full512<T> /* tag */, Vec512<uint8_t> v) {
  return Vec512<T>{BitCastFromInteger512<T>()(v.raw)};
}

template <typename T, typename FromT>
HWY_API Vec512<T> BitCast(Full512<T> d, Vec512<FromT> v) {
  return cast_u8_to(d, cast_to_u8(v));
}

// ------------------------------ Set

// Returns an all-zero vector.
template <typename T>
HWY_API Vec512<T> Zero(Full512<T> /* tag */) {
  return Vec512<T>{_mm512_setzero_si512()};
}
HWY_API Vec512<float> Zero(Full512<float> /* tag */) {
  return Vec512<float>{_mm512_setzero_ps()};
}
HWY_API Vec512<double> Zero(Full512<double> /* tag */) {
  return Vec512<double>{_mm512_setzero_pd()};
}

// Returns a vector with all lanes set to "t".
HWY_API Vec512<uint8_t> Set(Full512<uint8_t> /* tag */, const uint8_t t) {
  return Vec512<uint8_t>{_mm512_set1_epi8(static_cast<char>(t))};  // NOLINT
}
HWY_API Vec512<uint16_t> Set(Full512<uint16_t> /* tag */, const uint16_t t) {
  return Vec512<uint16_t>{_mm512_set1_epi16(static_cast<short>(t))};  // NOLINT
}
HWY_API Vec512<uint32_t> Set(Full512<uint32_t> /* tag */, const uint32_t t) {
  return Vec512<uint32_t>{_mm512_set1_epi32(static_cast<int>(t))};  // NOLINT
}
HWY_API Vec512<uint64_t> Set(Full512<uint64_t> /* tag */, const uint64_t t) {
  return Vec512<uint64_t>{
      _mm512_set1_epi64(static_cast<long long>(t))};  // NOLINT
}
HWY_API Vec512<int8_t> Set(Full512<int8_t> /* tag */, const int8_t t) {
  return Vec512<int8_t>{_mm512_set1_epi8(t)};
}
HWY_API Vec512<int16_t> Set(Full512<int16_t> /* tag */, const int16_t t) {
  return Vec512<int16_t>{_mm512_set1_epi16(t)};
}
HWY_API Vec512<int32_t> Set(Full512<int32_t> /* tag */, const int32_t t) {
  return Vec512<int32_t>{_mm512_set1_epi32(t)};
}
HWY_API Vec512<int64_t> Set(Full512<int64_t> /* tag */, const int64_t t) {
  return Vec512<int64_t>{_mm512_set1_epi64(t)};
}
HWY_API Vec512<float> Set(Full512<float> /* tag */, const float t) {
  return Vec512<float>{_mm512_set1_ps(t)};
}
HWY_API Vec512<double> Set(Full512<double> /* tag */, const double t) {
  return Vec512<double>{_mm512_set1_pd(t)};
}

HWY_DIAGNOSTICS(push)
HWY_DIAGNOSTICS_OFF(disable : 4700, ignored "-Wuninitialized")

// Returns a vector with uninitialized elements.
template <typename T>
HWY_API Vec512<T> Undefined(Full512<T> /* tag */) {
#ifdef __clang__
  return Vec512<T>{_mm512_undefined_epi32()};
#else
  __m512i raw;
  return Vec512<T>{raw};
#endif
}
HWY_API Vec512<float> Undefined(Full512<float> /* tag */) {
#ifdef __clang__
  return Vec512<float>{_mm512_undefined_ps()};
#else
  __m512 raw;
  return Vec512<float>{raw};
#endif
}
HWY_API Vec512<double> Undefined(Full512<double> /* tag */) {
#ifdef __clang__
  return Vec512<double>{_mm512_undefined_pd()};
#else
  __m512d raw;
  return Vec512<double>{raw};
#endif
}

HWY_DIAGNOSTICS(pop)

// ================================================== LOGICAL

// ------------------------------ Bitwise AND

template <typename T>
HWY_API Vec512<T> And(const Vec512<T> a, const Vec512<T> b) {
  return Vec512<T>{_mm512_and_si512(a.raw, b.raw)};
}

HWY_API Vec512<float> And(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_and_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> And(const Vec512<double> a, const Vec512<double> b) {
  return Vec512<double>{_mm512_and_pd(a.raw, b.raw)};
}

// ------------------------------ Bitwise AND-NOT

// Returns ~not_mask & mask.
template <typename T>
HWY_API Vec512<T> AndNot(const Vec512<T> not_mask, const Vec512<T> mask) {
  return Vec512<T>{_mm512_andnot_si512(not_mask.raw, mask.raw)};
}
HWY_API Vec512<float> AndNot(const Vec512<float> not_mask,
                             const Vec512<float> mask) {
  return Vec512<float>{_mm512_andnot_ps(not_mask.raw, mask.raw)};
}
HWY_API Vec512<double> AndNot(const Vec512<double> not_mask,
                              const Vec512<double> mask) {
  return Vec512<double>{_mm512_andnot_pd(not_mask.raw, mask.raw)};
}

// ------------------------------ Bitwise OR

template <typename T>
HWY_API Vec512<T> Or(const Vec512<T> a, const Vec512<T> b) {
  return Vec512<T>{_mm512_or_si512(a.raw, b.raw)};
}

HWY_API Vec512<float> Or(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_or_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> Or(const Vec512<double> a, const Vec512<double> b) {
  return Vec512<double>{_mm512_or_pd(a.raw, b.raw)};
}

// ------------------------------ Bitwise XOR

template <typename T>
HWY_API Vec512<T> Xor(const Vec512<T> a, const Vec512<T> b) {
  return Vec512<T>{_mm512_xor_si512(a.raw, b.raw)};
}

HWY_API Vec512<float> Xor(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_xor_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> Xor(const Vec512<double> a, const Vec512<double> b) {
  return Vec512<double>{_mm512_xor_pd(a.raw, b.raw)};
}

// ------------------------------ Operator overloads (internal-only if float)

template <typename T>
HWY_API Vec512<T> operator&(const Vec512<T> a, const Vec512<T> b) {
  return And(a, b);
}

template <typename T>
HWY_API Vec512<T> operator|(const Vec512<T> a, const Vec512<T> b) {
  return Or(a, b);
}

template <typename T>
HWY_API Vec512<T> operator^(const Vec512<T> a, const Vec512<T> b) {
  return Xor(a, b);
}

// ------------------------------ Select/blend

// Returns mask ? b : a.

namespace detail {

// Templates for signed/unsigned integer of a particular size.
template <typename T>
HWY_API Vec512<T> IfThenElse(hwy::SizeTag<1> /* tag */, const Mask512<T> mask,
                             const Vec512<T> yes, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_mov_epi8(no.raw, mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElse(hwy::SizeTag<2> /* tag */, const Mask512<T> mask,
                             const Vec512<T> yes, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_mov_epi16(no.raw, mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElse(hwy::SizeTag<4> /* tag */, const Mask512<T> mask,
                             const Vec512<T> yes, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_mov_epi32(no.raw, mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElse(hwy::SizeTag<8> /* tag */, const Mask512<T> mask,
                             const Vec512<T> yes, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_mov_epi64(no.raw, mask.raw, yes.raw)};
}

}  // namespace detail

template <typename T>
HWY_API Vec512<T> IfThenElse(const Mask512<T> mask, const Vec512<T> yes,
                             const Vec512<T> no) {
  return detail::IfThenElse(hwy::SizeTag<sizeof(T)>(), mask, yes, no);
}
template <>
HWY_API Vec512<float> IfThenElse(const Mask512<float> mask,
                                 const Vec512<float> yes,
                                 const Vec512<float> no) {
  return Vec512<float>{_mm512_mask_mov_ps(no.raw, mask.raw, yes.raw)};
}
template <>
HWY_API Vec512<double> IfThenElse(const Mask512<double> mask,
                                  const Vec512<double> yes,
                                  const Vec512<double> no) {
  return Vec512<double>{_mm512_mask_mov_pd(no.raw, mask.raw, yes.raw)};
}

namespace detail {

template <typename T>
HWY_API Vec512<T> IfThenElseZero(hwy::SizeTag<1> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> yes) {
  return Vec512<T>{_mm512_maskz_mov_epi8(mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElseZero(hwy::SizeTag<2> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> yes) {
  return Vec512<T>{_mm512_maskz_mov_epi16(mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElseZero(hwy::SizeTag<4> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> yes) {
  return Vec512<T>{_mm512_maskz_mov_epi32(mask.raw, yes.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenElseZero(hwy::SizeTag<8> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> yes) {
  return Vec512<T>{_mm512_maskz_mov_epi64(mask.raw, yes.raw)};
}

}  // namespace detail

template <typename T>
HWY_API Vec512<T> IfThenElseZero(const Mask512<T> mask, const Vec512<T> yes) {
  return detail::IfThenElseZero(hwy::SizeTag<sizeof(T)>(), mask, yes);
}
template <>
HWY_API Vec512<float> IfThenElseZero(const Mask512<float> mask,
                                     const Vec512<float> yes) {
  return Vec512<float>{_mm512_maskz_mov_ps(mask.raw, yes.raw)};
}
template <>
HWY_API Vec512<double> IfThenElseZero(const Mask512<double> mask,
                                      const Vec512<double> yes) {
  return Vec512<double>{_mm512_maskz_mov_pd(mask.raw, yes.raw)};
}

namespace detail {

template <typename T>
HWY_API Vec512<T> IfThenZeroElse(hwy::SizeTag<1> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> no) {
  // xor_epi8/16 are missing, but we have sub, which is just as fast for u8/16.
  return Vec512<T>{_mm512_mask_sub_epi8(no.raw, mask.raw, no.raw, no.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenZeroElse(hwy::SizeTag<2> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_sub_epi16(no.raw, mask.raw, no.raw, no.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenZeroElse(hwy::SizeTag<4> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_xor_epi32(no.raw, mask.raw, no.raw, no.raw)};
}
template <typename T>
HWY_API Vec512<T> IfThenZeroElse(hwy::SizeTag<8> /* tag */,
                                 const Mask512<T> mask, const Vec512<T> no) {
  return Vec512<T>{_mm512_mask_xor_epi64(no.raw, mask.raw, no.raw, no.raw)};
}

}  // namespace detail

template <typename T>
HWY_API Vec512<T> IfThenZeroElse(const Mask512<T> mask, const Vec512<T> no) {
  return detail::IfThenZeroElse(hwy::SizeTag<sizeof(T)>(), mask, no);
}
template <>
HWY_API Vec512<float> IfThenZeroElse(const Mask512<float> mask,
                                     const Vec512<float> no) {
  return Vec512<float>{_mm512_mask_xor_ps(no.raw, mask.raw, no.raw, no.raw)};
}
template <>
HWY_API Vec512<double> IfThenZeroElse(const Mask512<double> mask,
                                      const Vec512<double> no) {
  return Vec512<double>{_mm512_mask_xor_pd(no.raw, mask.raw, no.raw, no.raw)};
}

template <typename T, HWY_IF_FLOAT(T)>
HWY_API Vec512<T> ZeroIfNegative(const Vec512<T> v) {
  const auto zero = Zero(Full512<T>());
  return IfThenElse(MaskFromVec(v), zero, v);
}

// ================================================== ARITHMETIC

// ------------------------------ Addition

// Unsigned
HWY_API Vec512<uint8_t> operator+(const Vec512<uint8_t> a,
                                  const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_add_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> operator+(const Vec512<uint16_t> a,
                                   const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_add_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> operator+(const Vec512<uint32_t> a,
                                   const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_add_epi32(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> operator+(const Vec512<uint64_t> a,
                                   const Vec512<uint64_t> b) {
  return Vec512<uint64_t>{_mm512_add_epi64(a.raw, b.raw)};
}

// Signed
HWY_API Vec512<int8_t> operator+(const Vec512<int8_t> a,
                                 const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_add_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> operator+(const Vec512<int16_t> a,
                                  const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_add_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> operator+(const Vec512<int32_t> a,
                                  const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_add_epi32(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> operator+(const Vec512<int64_t> a,
                                  const Vec512<int64_t> b) {
  return Vec512<int64_t>{_mm512_add_epi64(a.raw, b.raw)};
}

// Float
HWY_API Vec512<float> operator+(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_add_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> operator+(const Vec512<double> a,
                                 const Vec512<double> b) {
  return Vec512<double>{_mm512_add_pd(a.raw, b.raw)};
}

// ------------------------------ Subtraction

// Unsigned
HWY_API Vec512<uint8_t> operator-(const Vec512<uint8_t> a,
                                  const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_sub_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> operator-(const Vec512<uint16_t> a,
                                   const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_sub_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> operator-(const Vec512<uint32_t> a,
                                   const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_sub_epi32(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> operator-(const Vec512<uint64_t> a,
                                   const Vec512<uint64_t> b) {
  return Vec512<uint64_t>{_mm512_sub_epi64(a.raw, b.raw)};
}

// Signed
HWY_API Vec512<int8_t> operator-(const Vec512<int8_t> a,
                                 const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_sub_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> operator-(const Vec512<int16_t> a,
                                  const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_sub_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> operator-(const Vec512<int32_t> a,
                                  const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_sub_epi32(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> operator-(const Vec512<int64_t> a,
                                  const Vec512<int64_t> b) {
  return Vec512<int64_t>{_mm512_sub_epi64(a.raw, b.raw)};
}

// Float
HWY_API Vec512<float> operator-(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_sub_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> operator-(const Vec512<double> a,
                                 const Vec512<double> b) {
  return Vec512<double>{_mm512_sub_pd(a.raw, b.raw)};
}

// ------------------------------ Saturating addition

// Returns a + b clamped to the destination range.

// Unsigned
HWY_API Vec512<uint8_t> SaturatedAdd(const Vec512<uint8_t> a,
                                     const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_adds_epu8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> SaturatedAdd(const Vec512<uint16_t> a,
                                      const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_adds_epu16(a.raw, b.raw)};
}

// Signed
HWY_API Vec512<int8_t> SaturatedAdd(const Vec512<int8_t> a,
                                    const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_adds_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> SaturatedAdd(const Vec512<int16_t> a,
                                     const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_adds_epi16(a.raw, b.raw)};
}

// ------------------------------ Saturating subtraction

// Returns a - b clamped to the destination range.

// Unsigned
HWY_API Vec512<uint8_t> SaturatedSub(const Vec512<uint8_t> a,
                                     const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_subs_epu8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> SaturatedSub(const Vec512<uint16_t> a,
                                      const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_subs_epu16(a.raw, b.raw)};
}

// Signed
HWY_API Vec512<int8_t> SaturatedSub(const Vec512<int8_t> a,
                                    const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_subs_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> SaturatedSub(const Vec512<int16_t> a,
                                     const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_subs_epi16(a.raw, b.raw)};
}

// ------------------------------ Average

// Returns (a + b + 1) / 2

// Unsigned
HWY_API Vec512<uint8_t> AverageRound(const Vec512<uint8_t> a,
                                     const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_avg_epu8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> AverageRound(const Vec512<uint16_t> a,
                                      const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_avg_epu16(a.raw, b.raw)};
}

// ------------------------------ Absolute value

// Returns absolute value, except that LimitsMin() maps to LimitsMax() + 1.
HWY_API Vec512<int8_t> Abs(const Vec512<int8_t> v) {
  return Vec512<int8_t>{_mm512_abs_epi8(v.raw)};
}
HWY_API Vec512<int16_t> Abs(const Vec512<int16_t> v) {
  return Vec512<int16_t>{_mm512_abs_epi16(v.raw)};
}
HWY_API Vec512<int32_t> Abs(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_abs_epi32(v.raw)};
}

// These aren't native instructions, they also involve AND with constant.
HWY_API Vec512<float> Abs(const Vec512<float> v) {
  return Vec512<float>{_mm512_abs_ps(v.raw)};
}
HWY_API Vec512<double> Abs(const Vec512<double> v) {
  return Vec512<double>{_mm512_abs_pd(v.raw)};
}

// ------------------------------ Shift lanes by constant #bits

// Unsigned
template <int kBits>
HWY_API Vec512<uint16_t> ShiftLeft(const Vec512<uint16_t> v) {
  return Vec512<uint16_t>{_mm512_slli_epi16(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<uint16_t> ShiftRight(const Vec512<uint16_t> v) {
  return Vec512<uint16_t>{_mm512_srli_epi16(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<uint32_t> ShiftLeft(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_slli_epi32(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<uint32_t> ShiftRight(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_srli_epi32(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<uint64_t> ShiftLeft(const Vec512<uint64_t> v) {
  return Vec512<uint64_t>{_mm512_slli_epi64(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<uint64_t> ShiftRight(const Vec512<uint64_t> v) {
  return Vec512<uint64_t>{_mm512_srli_epi64(v.raw, kBits)};
}

// Signed (no i64 ShiftRight)
template <int kBits>
HWY_API Vec512<int16_t> ShiftLeft(const Vec512<int16_t> v) {
  return Vec512<int16_t>{_mm512_slli_epi16(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<int16_t> ShiftRight(const Vec512<int16_t> v) {
  return Vec512<int16_t>{_mm512_srai_epi16(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<int32_t> ShiftLeft(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_slli_epi32(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<int32_t> ShiftRight(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_srai_epi32(v.raw, kBits)};
}
template <int kBits>
HWY_API Vec512<int64_t> ShiftLeft(const Vec512<int64_t> v) {
  return Vec512<int64_t>{_mm512_slli_epi64(v.raw, kBits)};
}

// ------------------------------ Shift lanes by independent variable #bits

// Unsigned (no u8,u16)
HWY_API Vec512<uint32_t> operator<<(const Vec512<uint32_t> v,
                                    const Vec512<uint32_t> bits) {
  return Vec512<uint32_t>{_mm512_sllv_epi32(v.raw, bits.raw)};
}
HWY_API Vec512<uint32_t> operator>>(const Vec512<uint32_t> v,
                                    const Vec512<uint32_t> bits) {
  return Vec512<uint32_t>{_mm512_srlv_epi32(v.raw, bits.raw)};
}
HWY_API Vec512<uint64_t> operator<<(const Vec512<uint64_t> v,
                                    const Vec512<uint64_t> bits) {
  return Vec512<uint64_t>{_mm512_sllv_epi64(v.raw, bits.raw)};
}
HWY_API Vec512<uint64_t> operator>>(const Vec512<uint64_t> v,
                                    const Vec512<uint64_t> bits) {
  return Vec512<uint64_t>{_mm512_srlv_epi64(v.raw, bits.raw)};
}

// Signed (no i8,i16,i64)
HWY_API Vec512<int32_t> operator<<(const Vec512<int32_t> v,
                                   const Vec512<int32_t> bits) {
  return Vec512<int32_t>{_mm512_sllv_epi32(v.raw, bits.raw)};
}
HWY_API Vec512<int32_t> operator>>(const Vec512<int32_t> v,
                                   const Vec512<int32_t> bits) {
  return Vec512<int32_t>{_mm512_srav_epi32(v.raw, bits.raw)};
}
HWY_API Vec512<int64_t> operator<<(const Vec512<int64_t> v,
                                   const Vec512<int64_t> bits) {
  return Vec512<int64_t>{_mm512_sllv_epi64(v.raw, bits.raw)};
}

// ------------------------------ Minimum

// Unsigned (no u64)
HWY_API Vec512<uint8_t> Min(const Vec512<uint8_t> a, const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_min_epu8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> Min(const Vec512<uint16_t> a,
                             const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_min_epu16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> Min(const Vec512<uint32_t> a,
                             const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_min_epu32(a.raw, b.raw)};
}

// Signed (no i64)
HWY_API Vec512<int8_t> Min(const Vec512<int8_t> a, const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_min_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> Min(const Vec512<int16_t> a, const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_min_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> Min(const Vec512<int32_t> a, const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_min_epi32(a.raw, b.raw)};
}

// Float
HWY_API Vec512<float> Min(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_min_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> Min(const Vec512<double> a, const Vec512<double> b) {
  return Vec512<double>{_mm512_min_pd(a.raw, b.raw)};
}

// ------------------------------ Maximum

// Unsigned (no u64)
HWY_API Vec512<uint8_t> Max(const Vec512<uint8_t> a, const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_max_epu8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> Max(const Vec512<uint16_t> a,
                             const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_max_epu16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> Max(const Vec512<uint32_t> a,
                             const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_max_epu32(a.raw, b.raw)};
}

// Signed (no i64)
HWY_API Vec512<int8_t> Max(const Vec512<int8_t> a, const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_max_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> Max(const Vec512<int16_t> a, const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_max_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> Max(const Vec512<int32_t> a, const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_max_epi32(a.raw, b.raw)};
}

// Float
HWY_API Vec512<float> Max(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_max_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> Max(const Vec512<double> a, const Vec512<double> b) {
  return Vec512<double>{_mm512_max_pd(a.raw, b.raw)};
}

// ------------------------------ Integer multiplication

// Unsigned
HWY_API Vec512<uint16_t> operator*(const Vec512<uint16_t> a,
                                   const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_mullo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> operator*(const Vec512<uint32_t> a,
                                   const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_mullo_epi32(a.raw, b.raw)};
}

// Signed
HWY_API Vec512<int16_t> operator*(const Vec512<int16_t> a,
                                  const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_mullo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> operator*(const Vec512<int32_t> a,
                                  const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_mullo_epi32(a.raw, b.raw)};
}

// Returns the upper 16 bits of a * b in each lane.
HWY_API Vec512<uint16_t> MulHigh(const Vec512<uint16_t> a,
                                 const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_mulhi_epu16(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> MulHigh(const Vec512<int16_t> a,
                                const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_mulhi_epi16(a.raw, b.raw)};
}

// Multiplies even lanes (0, 2 ..) and places the double-wide result into
// even and the upper half into its odd neighbor lane.
HWY_API Vec512<int64_t> MulEven(const Vec512<int32_t> a,
                                const Vec512<int32_t> b) {
  return Vec512<int64_t>{_mm512_mul_epi32(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> MulEven(const Vec512<uint32_t> a,
                                 const Vec512<uint32_t> b) {
  return Vec512<uint64_t>{_mm512_mul_epu32(a.raw, b.raw)};
}

// ------------------------------ Floating-point negate

HWY_API Vec512<float> Neg(const Vec512<float> v) {
  const Full512<float> df;
  const Full512<uint32_t> du;
  const auto sign = BitCast(df, Set(du, 0x80000000u));
  return v ^ sign;
}

HWY_API Vec512<double> Neg(const Vec512<double> v) {
  const Full512<double> df;
  const Full512<uint64_t> du;
  const auto sign = BitCast(df, Set(du, 0x8000000000000000ull));
  return v ^ sign;
}

// ------------------------------ Floating-point mul / div

HWY_API Vec512<float> operator*(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_mul_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> operator*(const Vec512<double> a,
                                 const Vec512<double> b) {
  return Vec512<double>{_mm512_mul_pd(a.raw, b.raw)};
}

HWY_API Vec512<float> operator/(const Vec512<float> a, const Vec512<float> b) {
  return Vec512<float>{_mm512_div_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> operator/(const Vec512<double> a,
                                 const Vec512<double> b) {
  return Vec512<double>{_mm512_div_pd(a.raw, b.raw)};
}

// Approximate reciprocal
HWY_API Vec512<float> ApproximateReciprocal(const Vec512<float> v) {
  return Vec512<float>{_mm512_rcp14_ps(v.raw)};
}

// Absolute value of difference.
HWY_API Vec512<float> AbsDiff(const Vec512<float> a, const Vec512<float> b) {
  return Abs(a - b);
}

// ------------------------------ Floating-point multiply-add variants

// Returns mul * x + add
HWY_API Vec512<float> MulAdd(const Vec512<float> mul, const Vec512<float> x,
                             const Vec512<float> add) {
  return Vec512<float>{_mm512_fmadd_ps(mul.raw, x.raw, add.raw)};
}
HWY_API Vec512<double> MulAdd(const Vec512<double> mul, const Vec512<double> x,
                              const Vec512<double> add) {
  return Vec512<double>{_mm512_fmadd_pd(mul.raw, x.raw, add.raw)};
}

// Returns add - mul * x
HWY_API Vec512<float> NegMulAdd(const Vec512<float> mul, const Vec512<float> x,
                                const Vec512<float> add) {
  return Vec512<float>{_mm512_fnmadd_ps(mul.raw, x.raw, add.raw)};
}
HWY_API Vec512<double> NegMulAdd(const Vec512<double> mul,
                                 const Vec512<double> x,
                                 const Vec512<double> add) {
  return Vec512<double>{_mm512_fnmadd_pd(mul.raw, x.raw, add.raw)};
}

// Returns mul * x - sub
HWY_API Vec512<float> MulSub(const Vec512<float> mul, const Vec512<float> x,
                             const Vec512<float> sub) {
  return Vec512<float>{_mm512_fmsub_ps(mul.raw, x.raw, sub.raw)};
}
HWY_API Vec512<double> MulSub(const Vec512<double> mul, const Vec512<double> x,
                              const Vec512<double> sub) {
  return Vec512<double>{_mm512_fmsub_pd(mul.raw, x.raw, sub.raw)};
}

// Returns -mul * x - sub
HWY_API Vec512<float> NegMulSub(const Vec512<float> mul, const Vec512<float> x,
                                const Vec512<float> sub) {
  return Vec512<float>{_mm512_fnmsub_ps(mul.raw, x.raw, sub.raw)};
}
HWY_API Vec512<double> NegMulSub(const Vec512<double> mul,
                                 const Vec512<double> x,
                                 const Vec512<double> sub) {
  return Vec512<double>{_mm512_fnmsub_pd(mul.raw, x.raw, sub.raw)};
}

// ------------------------------ Floating-point square root

// Full precision square root
HWY_API Vec512<float> Sqrt(const Vec512<float> v) {
  return Vec512<float>{_mm512_sqrt_ps(v.raw)};
}
HWY_API Vec512<double> Sqrt(const Vec512<double> v) {
  return Vec512<double>{_mm512_sqrt_pd(v.raw)};
}

// Approximate reciprocal square root
HWY_API Vec512<float> ApproximateReciprocalSqrt(const Vec512<float> v) {
  return Vec512<float>{_mm512_rsqrt14_ps(v.raw)};
}

// ------------------------------ Floating-point rounding

// Toward nearest integer, tie to even
HWY_API Vec512<float> Round(const Vec512<float> v) {
  return Vec512<float>{_mm512_roundscale_ps(
      v.raw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)};
}
HWY_API Vec512<double> Round(const Vec512<double> v) {
  return Vec512<double>{_mm512_roundscale_pd(
      v.raw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)};
}

// Toward zero, aka truncate
HWY_API Vec512<float> Trunc(const Vec512<float> v) {
  return Vec512<float>{
      _mm512_roundscale_ps(v.raw, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)};
}
HWY_API Vec512<double> Trunc(const Vec512<double> v) {
  return Vec512<double>{
      _mm512_roundscale_pd(v.raw, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)};
}

// Toward +infinity, aka ceiling
HWY_API Vec512<float> Ceil(const Vec512<float> v) {
  return Vec512<float>{
      _mm512_roundscale_ps(v.raw, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)};
}
HWY_API Vec512<double> Ceil(const Vec512<double> v) {
  return Vec512<double>{
      _mm512_roundscale_pd(v.raw, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)};
}

// Toward -infinity, aka floor
HWY_API Vec512<float> Floor(const Vec512<float> v) {
  return Vec512<float>{
      _mm512_roundscale_ps(v.raw, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)};
}
HWY_API Vec512<double> Floor(const Vec512<double> v) {
  return Vec512<double>{
      _mm512_roundscale_pd(v.raw, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)};
}

// ================================================== COMPARE

// Comparisons fill a lane with 1-bits if the condition is true, else 0.

namespace detail {

template <typename T>
HWY_API Mask512<T> TestBit(hwy::SizeTag<1> /*tag*/, const Vec512<T> v,
                           const Vec512<T> bit) {
  return Mask512<T>{_mm512_test_epi8_mask(v.raw, bit.raw)};
}
template <typename T>
HWY_API Mask512<T> TestBit(hwy::SizeTag<2> /*tag*/, const Vec512<T> v,
                           const Vec512<T> bit) {
  return Mask512<T>{_mm512_test_epi16_mask(v.raw, bit.raw)};
}
template <typename T>
HWY_API Mask512<T> TestBit(hwy::SizeTag<4> /*tag*/, const Vec512<T> v,
                           const Vec512<T> bit) {
  return Mask512<T>{_mm512_test_epi32_mask(v.raw, bit.raw)};
}
template <typename T>
HWY_API Mask512<T> TestBit(hwy::SizeTag<8> /*tag*/, const Vec512<T> v,
                           const Vec512<T> bit) {
  return Mask512<T>{_mm512_test_epi64_mask(v.raw, bit.raw)};
}

}  // namespace detail

template <typename T>
HWY_API Mask512<T> TestBit(const Vec512<T> v, const Vec512<T> bit) {
  static_assert(!hwy::IsFloat<T>(), "Only integer vectors supported");
  return detail::TestBit(hwy::SizeTag<sizeof(T)>(), v, bit);
}

// ------------------------------ Equality

// Unsigned
HWY_API Mask512<uint8_t> operator==(const Vec512<uint8_t> a,
                                    const Vec512<uint8_t> b) {
  return Mask512<uint8_t>{_mm512_cmpeq_epi8_mask(a.raw, b.raw)};
}
HWY_API Mask512<uint16_t> operator==(const Vec512<uint16_t> a,
                                     const Vec512<uint16_t> b) {
  return Mask512<uint16_t>{_mm512_cmpeq_epi16_mask(a.raw, b.raw)};
}
HWY_API Mask512<uint32_t> operator==(const Vec512<uint32_t> a,
                                     const Vec512<uint32_t> b) {
  return Mask512<uint32_t>{_mm512_cmpeq_epi32_mask(a.raw, b.raw)};
}
HWY_API Mask512<uint64_t> operator==(const Vec512<uint64_t> a,
                                     const Vec512<uint64_t> b) {
  return Mask512<uint64_t>{_mm512_cmpeq_epi64_mask(a.raw, b.raw)};
}

// Signed
HWY_API Mask512<int8_t> operator==(const Vec512<int8_t> a,
                                   const Vec512<int8_t> b) {
  return Mask512<int8_t>{_mm512_cmpeq_epi8_mask(a.raw, b.raw)};
}
HWY_API Mask512<int16_t> operator==(const Vec512<int16_t> a,
                                    const Vec512<int16_t> b) {
  return Mask512<int16_t>{_mm512_cmpeq_epi16_mask(a.raw, b.raw)};
}
HWY_API Mask512<int32_t> operator==(const Vec512<int32_t> a,
                                    const Vec512<int32_t> b) {
  return Mask512<int32_t>{_mm512_cmpeq_epi32_mask(a.raw, b.raw)};
}
HWY_API Mask512<int64_t> operator==(const Vec512<int64_t> a,
                                    const Vec512<int64_t> b) {
  return Mask512<int64_t>{_mm512_cmpeq_epi64_mask(a.raw, b.raw)};
}

// Float
HWY_API Mask512<float> operator==(const Vec512<float> a,
                                  const Vec512<float> b) {
  return Mask512<float>{_mm512_cmp_ps_mask(a.raw, b.raw, _CMP_EQ_OQ)};
}
HWY_API Mask512<double> operator==(const Vec512<double> a,
                                   const Vec512<double> b) {
  return Mask512<double>{_mm512_cmp_pd_mask(a.raw, b.raw, _CMP_EQ_OQ)};
}

// ------------------------------ Strict inequality

// Signed/float <
HWY_API Mask512<int8_t> operator<(const Vec512<int8_t> a,
                                  const Vec512<int8_t> b) {
  return Mask512<int8_t>{_mm512_cmpgt_epi8_mask(b.raw, a.raw)};
}
HWY_API Mask512<int16_t> operator<(const Vec512<int16_t> a,
                                   const Vec512<int16_t> b) {
  return Mask512<int16_t>{_mm512_cmpgt_epi16_mask(b.raw, a.raw)};
}
HWY_API Mask512<int32_t> operator<(const Vec512<int32_t> a,
                                   const Vec512<int32_t> b) {
  return Mask512<int32_t>{_mm512_cmpgt_epi32_mask(b.raw, a.raw)};
}
HWY_API Mask512<int64_t> operator<(const Vec512<int64_t> a,
                                   const Vec512<int64_t> b) {
  return Mask512<int64_t>{_mm512_cmpgt_epi64_mask(b.raw, a.raw)};
}
HWY_API Mask512<float> operator<(const Vec512<float> a, const Vec512<float> b) {
  return Mask512<float>{_mm512_cmp_ps_mask(a.raw, b.raw, _CMP_LT_OQ)};
}
HWY_API Mask512<double> operator<(const Vec512<double> a,
                                  const Vec512<double> b) {
  return Mask512<double>{_mm512_cmp_pd_mask(a.raw, b.raw, _CMP_LT_OQ)};
}

// Signed/float >
HWY_API Mask512<int8_t> operator>(const Vec512<int8_t> a,
                                  const Vec512<int8_t> b) {
  return Mask512<int8_t>{_mm512_cmpgt_epi8_mask(a.raw, b.raw)};
}
HWY_API Mask512<int16_t> operator>(const Vec512<int16_t> a,
                                   const Vec512<int16_t> b) {
  return Mask512<int16_t>{_mm512_cmpgt_epi16_mask(a.raw, b.raw)};
}
HWY_API Mask512<int32_t> operator>(const Vec512<int32_t> a,
                                   const Vec512<int32_t> b) {
  return Mask512<int32_t>{_mm512_cmpgt_epi32_mask(a.raw, b.raw)};
}
HWY_API Mask512<int64_t> operator>(const Vec512<int64_t> a,
                                   const Vec512<int64_t> b) {
  return Mask512<int64_t>{_mm512_cmpgt_epi64_mask(a.raw, b.raw)};
}
HWY_API Mask512<float> operator>(const Vec512<float> a, const Vec512<float> b) {
  return Mask512<float>{_mm512_cmp_ps_mask(a.raw, b.raw, _CMP_GT_OQ)};
}
HWY_API Mask512<double> operator>(const Vec512<double> a,
                                  const Vec512<double> b) {
  return Mask512<double>{_mm512_cmp_pd_mask(a.raw, b.raw, _CMP_GT_OQ)};
}

// ------------------------------ Weak inequality

// Float <= >=
HWY_API Mask512<float> operator<=(const Vec512<float> a,
                                  const Vec512<float> b) {
  return Mask512<float>{_mm512_cmp_ps_mask(a.raw, b.raw, _CMP_LE_OQ)};
}
HWY_API Mask512<double> operator<=(const Vec512<double> a,
                                   const Vec512<double> b) {
  return Mask512<double>{_mm512_cmp_pd_mask(a.raw, b.raw, _CMP_LE_OQ)};
}
HWY_API Mask512<float> operator>=(const Vec512<float> a,
                                  const Vec512<float> b) {
  return Mask512<float>{_mm512_cmp_ps_mask(a.raw, b.raw, _CMP_GE_OQ)};
}
HWY_API Mask512<double> operator>=(const Vec512<double> a,
                                   const Vec512<double> b) {
  return Mask512<double>{_mm512_cmp_pd_mask(a.raw, b.raw, _CMP_GE_OQ)};
}

// ------------------------------ Mask

namespace detail {

template <typename T>
HWY_API Mask512<T> MaskFromVec(hwy::SizeTag<1> /*tag*/, const Vec512<T> v) {
  return Mask512<T>{_mm512_movepi8_mask(v.raw)};
}
template <typename T>
HWY_API Mask512<T> MaskFromVec(hwy::SizeTag<2> /*tag*/, const Vec512<T> v) {
  return Mask512<T>{_mm512_movepi16_mask(v.raw)};
}
template <typename T>
HWY_API Mask512<T> MaskFromVec(hwy::SizeTag<4> /*tag*/, const Vec512<T> v) {
  return Mask512<T>{_mm512_movepi32_mask(v.raw)};
}
template <typename T>
HWY_API Mask512<T> MaskFromVec(hwy::SizeTag<8> /*tag*/, const Vec512<T> v) {
  return Mask512<T>{_mm512_movepi64_mask(v.raw)};
}

}  // namespace detail

template <typename T>
HWY_API Mask512<T> MaskFromVec(const Vec512<T> v) {
  return detail::MaskFromVec(hwy::SizeTag<sizeof(T)>(), v);
}
// There do not seem to be native floating-point versions of these instructions.
HWY_API Mask512<float> MaskFromVec(const Vec512<float> v) {
  return Mask512<float>{MaskFromVec(BitCast(Full512<int32_t>(), v)).raw};
}
HWY_API Mask512<double> MaskFromVec(const Vec512<double> v) {
  return Mask512<double>{MaskFromVec(BitCast(Full512<int64_t>(), v)).raw};
}

HWY_API Vec512<uint8_t> VecFromMask(const Mask512<uint8_t> v) {
  return Vec512<uint8_t>{_mm512_movm_epi8(v.raw)};
}
HWY_API Vec512<int8_t> VecFromMask(const Mask512<int8_t> v) {
  return Vec512<int8_t>{_mm512_movm_epi8(v.raw)};
}

HWY_API Vec512<uint16_t> VecFromMask(const Mask512<uint16_t> v) {
  return Vec512<uint16_t>{_mm512_movm_epi16(v.raw)};
}
HWY_API Vec512<int16_t> VecFromMask(const Mask512<int16_t> v) {
  return Vec512<int16_t>{_mm512_movm_epi16(v.raw)};
}

HWY_API Vec512<uint32_t> VecFromMask(const Mask512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_movm_epi32(v.raw)};
}
HWY_API Vec512<int32_t> VecFromMask(const Mask512<int32_t> v) {
  return Vec512<int32_t>{_mm512_movm_epi32(v.raw)};
}
HWY_API Vec512<float> VecFromMask(const Mask512<float> v) {
  return Vec512<float>{_mm512_castsi512_ps(_mm512_movm_epi32(v.raw))};
}

HWY_API Vec512<uint64_t> VecFromMask(const Mask512<uint64_t> v) {
  return Vec512<uint64_t>{_mm512_movm_epi64(v.raw)};
}
HWY_API Vec512<int64_t> VecFromMask(const Mask512<int64_t> v) {
  return Vec512<int64_t>{_mm512_movm_epi64(v.raw)};
}
HWY_API Vec512<double> VecFromMask(const Mask512<double> v) {
  return Vec512<double>{_mm512_castsi512_pd(_mm512_movm_epi64(v.raw))};
}

// ================================================== MEMORY

// ------------------------------ Load

template <typename T>
HWY_API Vec512<T> Load(Full512<T> /* tag */, const T* HWY_RESTRICT aligned) {
  return Vec512<T>{
      _mm512_load_si512(reinterpret_cast<const __m512i*>(aligned))};
}
HWY_API Vec512<float> Load(Full512<float> /* tag */,
                           const float* HWY_RESTRICT aligned) {
  return Vec512<float>{_mm512_load_ps(aligned)};
}
HWY_API Vec512<double> Load(Full512<double> /* tag */,
                            const double* HWY_RESTRICT aligned) {
  return Vec512<double>{_mm512_load_pd(aligned)};
}

template <typename T>
HWY_API Vec512<T> LoadU(Full512<T> /* tag */, const T* HWY_RESTRICT p) {
  return Vec512<T>{_mm512_loadu_si512(reinterpret_cast<const __m512i*>(p))};
}
HWY_API Vec512<float> LoadU(Full512<float> /* tag */,
                            const float* HWY_RESTRICT p) {
  return Vec512<float>{_mm512_loadu_ps(p)};
}
HWY_API Vec512<double> LoadU(Full512<double> /* tag */,
                             const double* HWY_RESTRICT p) {
  return Vec512<double>{_mm512_loadu_pd(p)};
}

// Loads 128 bit and duplicates into both 128-bit halves. This avoids the
// 3-cycle cost of moving data between 128-bit halves and avoids port 5.
template <typename T>
HWY_API Vec512<T> LoadDup128(Full512<T> /* tag */,
                             const T* const HWY_RESTRICT p) {
  // Clang 3.9 generates VINSERTF128 which is slower, but inline assembly leads
  // to "invalid output size for constraint" without -mavx512:
  // https://gcc.godbolt.org/z/-Jt_-F
#if HWY_LOADDUP_ASM
  __m512i out;
  asm("vbroadcasti128 %1, %[reg]" : [ reg ] "=x"(out) : "m"(p[0]));
  return Vec512<T>{out};
#else
  const auto x4 = LoadU(Full128<T>(), p);
  return Vec512<T>{_mm512_broadcast_i32x4(x4.raw)};
#endif
}
HWY_API Vec512<float> LoadDup128(Full512<float> /* tag */,
                                 const float* const HWY_RESTRICT p) {
#if HWY_LOADDUP_ASM
  __m512 out;
  asm("vbroadcastf128 %1, %[reg]" : [ reg ] "=x"(out) : "m"(p[0]));
  return Vec512<float>{out};
#else
  const __m128 x4 = _mm_loadu_ps(p);
  return Vec512<float>{_mm512_broadcast_f32x4(x4)};
#endif
}

HWY_API Vec512<double> LoadDup128(Full512<double> /* tag */,
                                  const double* const HWY_RESTRICT p) {
#if HWY_LOADDUP_ASM
  __m512d out;
  asm("vbroadcastf128 %1, %[reg]" : [ reg ] "=x"(out) : "m"(p[0]));
  return Vec512<double>{out};
#else
  const __m128d x2 = _mm_loadu_pd(p);
  return Vec512<double>{_mm512_broadcast_f64x2(x2)};
#endif
}

// ------------------------------ Store

template <typename T>
HWY_API void Store(const Vec512<T> v, Full512<T> /* tag */,
                   T* HWY_RESTRICT aligned) {
  _mm512_store_si512(reinterpret_cast<__m512i*>(aligned), v.raw);
}
HWY_API void Store(const Vec512<float> v, Full512<float> /* tag */,
                   float* HWY_RESTRICT aligned) {
  _mm512_store_ps(aligned, v.raw);
}
HWY_API void Store(const Vec512<double> v, Full512<double> /* tag */,
                   double* HWY_RESTRICT aligned) {
  _mm512_store_pd(aligned, v.raw);
}

template <typename T>
HWY_API void StoreU(const Vec512<T> v, Full512<T> /* tag */,
                    T* HWY_RESTRICT p) {
  _mm512_storeu_si512(reinterpret_cast<__m512i*>(p), v.raw);
}
HWY_API void StoreU(const Vec512<float> v, Full512<float> /* tag */,
                    float* HWY_RESTRICT p) {
  _mm512_storeu_ps(p, v.raw);
}
HWY_API void StoreU(const Vec512<double> v, Full512<double>,
                    double* HWY_RESTRICT p) {
  _mm512_storeu_pd(p, v.raw);
}

// ------------------------------ Non-temporal stores

template <typename T>
HWY_API void Stream(const Vec512<T> v, Full512<T> /* tag */,
                    T* HWY_RESTRICT aligned) {
  _mm512_stream_si512(reinterpret_cast<__m512i*>(aligned), v.raw);
}
HWY_API void Stream(const Vec512<float> v, Full512<float> /* tag */,
                    float* HWY_RESTRICT aligned) {
  _mm512_stream_ps(aligned, v.raw);
}
HWY_API void Stream(const Vec512<double> v, Full512<double>,
                    double* HWY_RESTRICT aligned) {
  _mm512_stream_pd(aligned, v.raw);
}

// ------------------------------ Gather

namespace detail {

template <typename T>
HWY_API Vec512<T> GatherOffset(hwy::SizeTag<4> /* tag */, Full512<T> /* tag */,
                               const T* HWY_RESTRICT base,
                               const Vec512<int32_t> offset) {
  return Vec512<T>{_mm512_i32gather_epi32(offset.raw, base, 1)};
}
template <typename T>
HWY_API Vec512<T> GatherIndex(hwy::SizeTag<4> /* tag */, Full512<T> /* tag */,
                              const T* HWY_RESTRICT base,
                              const Vec512<int32_t> index) {
  return Vec512<T>{_mm512_i32gather_epi32(index.raw, base, 4)};
}

template <typename T>
HWY_API Vec512<T> GatherOffset(hwy::SizeTag<8> /* tag */, Full512<T> /* tag */,
                               const T* HWY_RESTRICT base,
                               const Vec512<int64_t> offset) {
  return Vec512<T>{_mm512_i64gather_epi64(offset.raw, base, 1)};
}
template <typename T>
HWY_API Vec512<T> GatherIndex(hwy::SizeTag<8> /* tag */, Full512<T> /* tag */,
                              const T* HWY_RESTRICT base,
                              const Vec512<int64_t> index) {
  return Vec512<T>{_mm512_i64gather_epi64(index.raw, base, 8)};
}

}  // namespace detail

template <typename T, typename Offset>
HWY_API Vec512<T> GatherOffset(Full512<T> d, const T* HWY_RESTRICT base,
                               const Vec512<Offset> offset) {
  static_assert(sizeof(T) == sizeof(Offset), "SVE requires same size base/ofs");
  return detail::GatherOffset(hwy::SizeTag<sizeof(T)>(), d, base, offset);
}
template <typename T, typename Index>
HWY_API Vec512<T> GatherIndex(Full512<T> d, const T* HWY_RESTRICT base,
                              const Vec512<Index> index) {
  static_assert(sizeof(T) == sizeof(Index), "SVE requires same size base/idx");
  return detail::GatherIndex(hwy::SizeTag<sizeof(T)>(), d, base, index);
}

template <>
HWY_API Vec512<float> GatherOffset<float>(Full512<float> /* tag */,
                                          const float* HWY_RESTRICT base,
                                          const Vec512<int32_t> offset) {
  return Vec512<float>{_mm512_i32gather_ps(offset.raw, base, 1)};
}
template <>
HWY_API Vec512<float> GatherIndex<float>(Full512<float> /* tag */,
                                         const float* HWY_RESTRICT base,
                                         const Vec512<int32_t> index) {
  return Vec512<float>{_mm512_i32gather_ps(index.raw, base, 4)};
}

template <>
HWY_API Vec512<double> GatherOffset<double>(Full512<double> /* tag */,
                                            const double* HWY_RESTRICT base,
                                            const Vec512<int64_t> offset) {
  return Vec512<double>{_mm512_i64gather_pd(offset.raw, base, 1)};
}
template <>
HWY_API Vec512<double> GatherIndex<double>(Full512<double> /* tag */,
                                           const double* HWY_RESTRICT base,
                                           const Vec512<int64_t> index) {
  return Vec512<double>{_mm512_i64gather_pd(index.raw, base, 8)};
}

// ================================================== SWIZZLE

template <typename T>
HWY_API T GetLane(const Vec512<T> v) {
  return GetLane(LowerHalf(v));
}

// ------------------------------ Extract half

template <typename T>
HWY_API Vec256<T> LowerHalf(Vec512<T> v) {
  return Vec256<T>{_mm512_castsi512_si256(v.raw)};
}
template <>
HWY_API Vec256<float> LowerHalf(Vec512<float> v) {
  return Vec256<float>{_mm512_castps512_ps256(v.raw)};
}
template <>
HWY_API Vec256<double> LowerHalf(Vec512<double> v) {
  return Vec256<double>{_mm512_castpd512_pd256(v.raw)};
}

template <typename T>
HWY_API Vec256<T> UpperHalf(Vec512<T> v) {
  return Vec256<T>{_mm512_extracti32x8_epi32(v.raw, 1)};
}
template <>
HWY_API Vec256<float> UpperHalf(Vec512<float> v) {
  return Vec256<float>{_mm512_extractf32x8_ps(v.raw, 1)};
}
template <>
HWY_API Vec256<double> UpperHalf(Vec512<double> v) {
  return Vec256<double>{_mm512_extractf64x4_pd(v.raw, 1)};
}

// ------------------------------ Shift vector by constant #bytes

// 0x01..0F, kBytes = 1 => 0x02..0F00
template <int kBytes, typename T>
HWY_API Vec512<T> ShiftLeftBytes(const Vec512<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  return Vec512<T>{_mm512_bslli_epi128(v.raw, kBytes)};
}

template <int kLanes, typename T>
HWY_API Vec512<T> ShiftLeftLanes(const Vec512<T> v) {
  return ShiftLeftBytes<kLanes * sizeof(T)>(v);
}

// 0x01..0F, kBytes = 1 => 0x0001..0E
template <int kBytes, typename T>
HWY_API Vec512<T> ShiftRightBytes(const Vec512<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  return Vec512<T>{_mm512_bsrli_epi128(v.raw, kBytes)};
}

template <int kLanes, typename T>
HWY_API Vec512<T> ShiftRightLanes(const Vec512<T> v) {
  return ShiftRightBytes<kLanes * sizeof(T)>(v);
}

// ------------------------------ Extract from 2x 128-bit at constant offset

// Extracts 128 bits from <hi, lo> by skipping the least-significant kBytes.
template <int kBytes, typename T>
HWY_API Vec512<T> CombineShiftRightBytes(const Vec512<T> hi,
                                         const Vec512<T> lo) {
  const Full512<uint8_t> d8;
  const Vec512<uint8_t> extracted_bytes{
      _mm512_alignr_epi8(BitCast(d8, hi).raw, BitCast(d8, lo).raw, kBytes)};
  return BitCast(Full512<T>(), extracted_bytes);
}

// ------------------------------ Broadcast/splat any lane

// Unsigned
template <int kLane>
HWY_API Vec512<uint16_t> Broadcast(const Vec512<uint16_t> v) {
  static_assert(0 <= kLane && kLane < 8, "Invalid lane");
  if (kLane < 4) {
    const __m512i lo = _mm512_shufflelo_epi16(v.raw, (0x55 * kLane) & 0xFF);
    return Vec512<uint16_t>{_mm512_unpacklo_epi64(lo, lo)};
  } else {
    const __m512i hi =
        _mm512_shufflehi_epi16(v.raw, (0x55 * (kLane - 4)) & 0xFF);
    return Vec512<uint16_t>{_mm512_unpackhi_epi64(hi, hi)};
  }
}
template <int kLane>
HWY_API Vec512<uint32_t> Broadcast(const Vec512<uint32_t> v) {
  static_assert(0 <= kLane && kLane < 4, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = static_cast<_MM_PERM_ENUM>(0x55 * kLane);
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, perm)};
}
template <int kLane>
HWY_API Vec512<uint64_t> Broadcast(const Vec512<uint64_t> v) {
  static_assert(0 <= kLane && kLane < 2, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = kLane ? _MM_PERM_DCDC : _MM_PERM_BABA;
  return Vec512<uint64_t>{_mm512_shuffle_epi32(v.raw, perm)};
}

// Signed
template <int kLane>
HWY_API Vec512<int16_t> Broadcast(const Vec512<int16_t> v) {
  static_assert(0 <= kLane && kLane < 8, "Invalid lane");
  if (kLane < 4) {
    const __m512i lo = _mm512_shufflelo_epi16(v.raw, (0x55 * kLane) & 0xFF);
    return Vec512<int16_t>{_mm512_unpacklo_epi64(lo, lo)};
  } else {
    const __m512i hi =
        _mm512_shufflehi_epi16(v.raw, (0x55 * (kLane - 4)) & 0xFF);
    return Vec512<int16_t>{_mm512_unpackhi_epi64(hi, hi)};
  }
}
template <int kLane>
HWY_API Vec512<int32_t> Broadcast(const Vec512<int32_t> v) {
  static_assert(0 <= kLane && kLane < 4, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = static_cast<_MM_PERM_ENUM>(0x55 * kLane);
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, perm)};
}
template <int kLane>
HWY_API Vec512<int64_t> Broadcast(const Vec512<int64_t> v) {
  static_assert(0 <= kLane && kLane < 2, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = kLane ? _MM_PERM_DCDC : _MM_PERM_BABA;
  return Vec512<int64_t>{_mm512_shuffle_epi32(v.raw, perm)};
}

// Float
template <int kLane>
HWY_API Vec512<float> Broadcast(const Vec512<float> v) {
  static_assert(0 <= kLane && kLane < 4, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = static_cast<_MM_PERM_ENUM>(0x55 * kLane);
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, perm)};
}
template <int kLane>
HWY_API Vec512<double> Broadcast(const Vec512<double> v) {
  static_assert(0 <= kLane && kLane < 2, "Invalid lane");
  constexpr _MM_PERM_ENUM perm = static_cast<_MM_PERM_ENUM>(0xFF * kLane);
  return Vec512<double>{_mm512_shuffle_pd(v.raw, v.raw, perm)};
}

// ------------------------------ Hard-coded shuffles

// Notation: let Vec512<int32_t> have lanes 7,6,5,4,3,2,1,0 (0 is
// least-significant). Shuffle0321 rotates four-lane blocks one lane to the
// right (the previous least-significant lane is now most-significant =>
// 47650321). These could also be implemented via CombineShiftRightBytes but
// the shuffle_abcd notation is more convenient.

// Swap 32-bit halves in 64-bit halves.
HWY_API Vec512<uint32_t> Shuffle2301(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_CDAB)};
}
HWY_API Vec512<int32_t> Shuffle2301(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_CDAB)};
}
HWY_API Vec512<float> Shuffle2301(const Vec512<float> v) {
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, _MM_PERM_CDAB)};
}

// Swap 64-bit halves
HWY_API Vec512<uint32_t> Shuffle1032(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_BADC)};
}
HWY_API Vec512<int32_t> Shuffle1032(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_BADC)};
}
HWY_API Vec512<float> Shuffle1032(const Vec512<float> v) {
  // Shorter encoding than _mm512_permute_ps.
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, _MM_PERM_BADC)};
}
HWY_API Vec512<uint64_t> Shuffle01(const Vec512<uint64_t> v) {
  return Vec512<uint64_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_BADC)};
}
HWY_API Vec512<int64_t> Shuffle01(const Vec512<int64_t> v) {
  return Vec512<int64_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_BADC)};
}
HWY_API Vec512<double> Shuffle01(const Vec512<double> v) {
  // Shorter encoding than _mm512_permute_pd.
  return Vec512<double>{_mm512_shuffle_pd(v.raw, v.raw, _MM_PERM_BBBB)};
}

// Rotate right 32 bits
HWY_API Vec512<uint32_t> Shuffle0321(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_ADCB)};
}
HWY_API Vec512<int32_t> Shuffle0321(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_ADCB)};
}
HWY_API Vec512<float> Shuffle0321(const Vec512<float> v) {
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, _MM_PERM_ADCB)};
}
// Rotate left 32 bits
HWY_API Vec512<uint32_t> Shuffle2103(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_CBAD)};
}
HWY_API Vec512<int32_t> Shuffle2103(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_CBAD)};
}
HWY_API Vec512<float> Shuffle2103(const Vec512<float> v) {
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, _MM_PERM_CBAD)};
}

// Reverse
HWY_API Vec512<uint32_t> Shuffle0123(const Vec512<uint32_t> v) {
  return Vec512<uint32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_ABCD)};
}
HWY_API Vec512<int32_t> Shuffle0123(const Vec512<int32_t> v) {
  return Vec512<int32_t>{_mm512_shuffle_epi32(v.raw, _MM_PERM_ABCD)};
}
HWY_API Vec512<float> Shuffle0123(const Vec512<float> v) {
  return Vec512<float>{_mm512_shuffle_ps(v.raw, v.raw, _MM_PERM_ABCD)};
}

// ------------------------------ Permute (runtime variable)

// Returned by SetTableIndices for use by TableLookupLanes.
template <typename T>
struct Permute512 {
  __m512i raw;
};

template <typename T>
HWY_API Permute512<T> SetTableIndices(const Full512<T>, const int32_t* idx) {
#if !defined(NDEBUG) || defined(ADDRESS_SANITIZER)
  const size_t N = 64 / sizeof(T);
  for (size_t i = 0; i < N; ++i) {
    HWY_DASSERT(0 <= idx[i] && idx[i] < static_cast<int32_t>(N));
  }
#endif
  return Permute512<T>{LoadU(Full512<int32_t>(), idx).raw};
}

HWY_API Vec512<uint32_t> TableLookupLanes(const Vec512<uint32_t> v,
                                          const Permute512<uint32_t> idx) {
  return Vec512<uint32_t>{_mm512_permutexvar_epi32(idx.raw, v.raw)};
}
HWY_API Vec512<int32_t> TableLookupLanes(const Vec512<int32_t> v,
                                         const Permute512<int32_t> idx) {
  return Vec512<int32_t>{_mm512_permutexvar_epi32(idx.raw, v.raw)};
}
HWY_API Vec512<float> TableLookupLanes(const Vec512<float> v,
                                       const Permute512<float> idx) {
  return Vec512<float>{_mm512_permutexvar_ps(idx.raw, v.raw)};
}

// ------------------------------ Interleave lanes

// Interleaves lanes from halves of the 128-bit blocks of "a" (which provides
// the least-significant lane) and "b". To concatenate two half-width integers
// into one, use ZipLower/Upper instead (also works with scalar).

HWY_API Vec512<uint8_t> InterleaveLower(const Vec512<uint8_t> a,
                                        const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> InterleaveLower(const Vec512<uint16_t> a,
                                         const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> InterleaveLower(const Vec512<uint32_t> a,
                                         const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_unpacklo_epi32(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> InterleaveLower(const Vec512<uint64_t> a,
                                         const Vec512<uint64_t> b) {
  return Vec512<uint64_t>{_mm512_unpacklo_epi64(a.raw, b.raw)};
}

HWY_API Vec512<int8_t> InterleaveLower(const Vec512<int8_t> a,
                                       const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> InterleaveLower(const Vec512<int16_t> a,
                                        const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> InterleaveLower(const Vec512<int32_t> a,
                                        const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_unpacklo_epi32(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> InterleaveLower(const Vec512<int64_t> a,
                                        const Vec512<int64_t> b) {
  return Vec512<int64_t>{_mm512_unpacklo_epi64(a.raw, b.raw)};
}

HWY_API Vec512<float> InterleaveLower(const Vec512<float> a,
                                      const Vec512<float> b) {
  return Vec512<float>{_mm512_unpacklo_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> InterleaveLower(const Vec512<double> a,
                                       const Vec512<double> b) {
  return Vec512<double>{_mm512_unpacklo_pd(a.raw, b.raw)};
}

HWY_API Vec512<uint8_t> InterleaveUpper(const Vec512<uint8_t> a,
                                        const Vec512<uint8_t> b) {
  return Vec512<uint8_t>{_mm512_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint16_t> InterleaveUpper(const Vec512<uint16_t> a,
                                         const Vec512<uint16_t> b) {
  return Vec512<uint16_t>{_mm512_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> InterleaveUpper(const Vec512<uint32_t> a,
                                         const Vec512<uint32_t> b) {
  return Vec512<uint32_t>{_mm512_unpackhi_epi32(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> InterleaveUpper(const Vec512<uint64_t> a,
                                         const Vec512<uint64_t> b) {
  return Vec512<uint64_t>{_mm512_unpackhi_epi64(a.raw, b.raw)};
}

HWY_API Vec512<int8_t> InterleaveUpper(const Vec512<int8_t> a,
                                       const Vec512<int8_t> b) {
  return Vec512<int8_t>{_mm512_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int16_t> InterleaveUpper(const Vec512<int16_t> a,
                                        const Vec512<int16_t> b) {
  return Vec512<int16_t>{_mm512_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> InterleaveUpper(const Vec512<int32_t> a,
                                        const Vec512<int32_t> b) {
  return Vec512<int32_t>{_mm512_unpackhi_epi32(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> InterleaveUpper(const Vec512<int64_t> a,
                                        const Vec512<int64_t> b) {
  return Vec512<int64_t>{_mm512_unpackhi_epi64(a.raw, b.raw)};
}

HWY_API Vec512<float> InterleaveUpper(const Vec512<float> a,
                                      const Vec512<float> b) {
  return Vec512<float>{_mm512_unpackhi_ps(a.raw, b.raw)};
}
HWY_API Vec512<double> InterleaveUpper(const Vec512<double> a,
                                       const Vec512<double> b) {
  return Vec512<double>{_mm512_unpackhi_pd(a.raw, b.raw)};
}

// ------------------------------ Zip lanes

// Same as interleave_*, except that the return lanes are double-width integers;
// this is necessary because the single-lane scalar cannot return two values.

HWY_API Vec512<uint16_t> ZipLower(const Vec512<uint8_t> a,
                                  const Vec512<uint8_t> b) {
  return Vec512<uint16_t>{_mm512_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> ZipLower(const Vec512<uint16_t> a,
                                  const Vec512<uint16_t> b) {
  return Vec512<uint32_t>{_mm512_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> ZipLower(const Vec512<uint32_t> a,
                                  const Vec512<uint32_t> b) {
  return Vec512<uint64_t>{_mm512_unpacklo_epi32(a.raw, b.raw)};
}

HWY_API Vec512<int16_t> ZipLower(const Vec512<int8_t> a,
                                 const Vec512<int8_t> b) {
  return Vec512<int16_t>{_mm512_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> ZipLower(const Vec512<int16_t> a,
                                 const Vec512<int16_t> b) {
  return Vec512<int32_t>{_mm512_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> ZipLower(const Vec512<int32_t> a,
                                 const Vec512<int32_t> b) {
  return Vec512<int64_t>{_mm512_unpacklo_epi32(a.raw, b.raw)};
}

HWY_API Vec512<uint16_t> ZipUpper(const Vec512<uint8_t> a,
                                  const Vec512<uint8_t> b) {
  return Vec512<uint16_t>{_mm512_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec512<uint32_t> ZipUpper(const Vec512<uint16_t> a,
                                  const Vec512<uint16_t> b) {
  return Vec512<uint32_t>{_mm512_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec512<uint64_t> ZipUpper(const Vec512<uint32_t> a,
                                  const Vec512<uint32_t> b) {
  return Vec512<uint64_t>{_mm512_unpackhi_epi32(a.raw, b.raw)};
}

HWY_API Vec512<int16_t> ZipUpper(const Vec512<int8_t> a,
                                 const Vec512<int8_t> b) {
  return Vec512<int16_t>{_mm512_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec512<int32_t> ZipUpper(const Vec512<int16_t> a,
                                 const Vec512<int16_t> b) {
  return Vec512<int32_t>{_mm512_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec512<int64_t> ZipUpper(const Vec512<int32_t> a,
                                 const Vec512<int32_t> b) {
  return Vec512<int64_t>{_mm512_unpackhi_epi32(a.raw, b.raw)};
}

// ------------------------------ Blocks

// hiH,hiL loH,loL |-> hiL,loL (= lower halves)
template <typename T>
HWY_API Vec512<T> ConcatLowerLower(const Vec512<T> hi, const Vec512<T> lo) {
  return Vec512<T>{_mm512_shuffle_i32x4(lo.raw, hi.raw, _MM_PERM_BABA)};
}
template <>
HWY_API Vec512<float> ConcatLowerLower(const Vec512<float> hi,
                                       const Vec512<float> lo) {
  return Vec512<float>{_mm512_shuffle_f32x4(lo.raw, hi.raw, _MM_PERM_BABA)};
}
template <>
HWY_API Vec512<double> ConcatLowerLower(const Vec512<double> hi,
                                        const Vec512<double> lo) {
  return Vec512<double>{_mm512_shuffle_f64x2(lo.raw, hi.raw, _MM_PERM_BABA)};
}

// hiH,hiL loH,loL |-> hiH,loH (= upper halves)
template <typename T>
HWY_API Vec512<T> ConcatUpperUpper(const Vec512<T> hi, const Vec512<T> lo) {
  return Vec512<T>{_mm512_shuffle_i32x4(lo.raw, hi.raw, _MM_PERM_DCDC)};
}
template <>
HWY_API Vec512<float> ConcatUpperUpper(const Vec512<float> hi,
                                       const Vec512<float> lo) {
  return Vec512<float>{_mm512_shuffle_f32x4(lo.raw, hi.raw, _MM_PERM_DCDC)};
}
template <>
HWY_API Vec512<double> ConcatUpperUpper(const Vec512<double> hi,
                                        const Vec512<double> lo) {
  return Vec512<double>{_mm512_shuffle_f64x2(lo.raw, hi.raw, _MM_PERM_DCDC)};
}

// hiH,hiL loH,loL |-> hiL,loH (= inner halves / swap blocks)
template <typename T>
HWY_API Vec512<T> ConcatLowerUpper(const Vec512<T> hi, const Vec512<T> lo) {
  return Vec512<T>{_mm512_shuffle_i32x4(lo.raw, hi.raw, 0x4E)};
}
template <>
HWY_API Vec512<float> ConcatLowerUpper(const Vec512<float> hi,
                                       const Vec512<float> lo) {
  return Vec512<float>{_mm512_shuffle_f32x4(lo.raw, hi.raw, 0x4E)};
}
template <>
HWY_API Vec512<double> ConcatLowerUpper(const Vec512<double> hi,
                                        const Vec512<double> lo) {
  return Vec512<double>{_mm512_shuffle_f64x2(lo.raw, hi.raw, 0x4E)};
}

// hiH,hiL loH,loL |-> hiH,loL (= outer halves)
template <typename T>
HWY_API Vec512<T> ConcatUpperLower(const Vec512<T> hi, const Vec512<T> lo) {
  // There are no imm8 blend in AVX512. Use blend16 because 32-bit masks
  // are efficiently loaded from 32-bit regs.
  const __mmask32 mask = /*_cvtu32_mask32 */ (0x0000FFFF);
  return Vec512<T>{_mm512_mask_blend_epi16(mask, hi.raw, lo.raw)};
}
template <>
HWY_API Vec512<float> ConcatUpperLower(const Vec512<float> hi,
                                       const Vec512<float> lo) {
  const __mmask16 mask = /*_cvtu32_mask16 */ (0x00FF);
  return Vec512<float>{_mm512_mask_blend_ps(mask, hi.raw, lo.raw)};
}
template <>
HWY_API Vec512<double> ConcatUpperLower(const Vec512<double> hi,
                                        const Vec512<double> lo) {
  const __mmask8 mask = /*_cvtu32_mask8 */ (0x0F);
  return Vec512<double>{_mm512_mask_blend_pd(mask, hi.raw, lo.raw)};
}

// ------------------------------ Odd/even lanes

template <typename T>
HWY_API Vec512<T> OddEven(const Vec512<T> a, const Vec512<T> b) {
  constexpr size_t s = sizeof(T);
  constexpr int shift = s == 1 ? 0 : s == 2 ? 32 : s == 4 ? 48 : 56;
  return IfThenElse(Mask512<T>{0x5555555555555555ull >> shift}, b, a);
}

// ------------------------------ Shuffle bytes with variable indices

// Returns vector of bytes[from[i]]. "from" is also interpreted as bytes:
// either valid indices in [0, 16) or >= 0x80 to zero the i-th output byte.
template <typename T, typename TI>
HWY_API Vec512<T> TableLookupBytes(const Vec512<T> bytes,
                                   const Vec512<TI> from) {
  return Vec512<T>{_mm512_shuffle_epi8(bytes.raw, from.raw)};
}

// ================================================== CONVERT

// ------------------------------ Promotions (part w/ narrow lanes -> full)

HWY_API Vec512<double> PromoteTo(Full512<double> /* tag */, Vec256<float> v) {
  return Vec512<double>{_mm512_cvtps_pd(v.raw)};
}

// Unsigned: zero-extend.
// Note: these have 3 cycle latency; if inputs are already split across the
// 128 bit blocks (in their upper/lower halves), then Zip* would be faster.
HWY_API Vec512<uint16_t> PromoteTo(Full512<uint16_t> /* tag */,
                                   Vec256<uint8_t> v) {
  return Vec512<uint16_t>{_mm512_cvtepu8_epi16(v.raw)};
}
HWY_API Vec512<uint32_t> PromoteTo(Full512<uint32_t> /* tag */,
                                   Vec128<uint8_t> v) {
  return Vec512<uint32_t>{_mm512_cvtepu8_epi32(v.raw)};
}
HWY_API Vec512<int16_t> PromoteTo(Full512<int16_t> /* tag */,
                                  Vec256<uint8_t> v) {
  return Vec512<int16_t>{_mm512_cvtepu8_epi16(v.raw)};
}
HWY_API Vec512<int32_t> PromoteTo(Full512<int32_t> /* tag */,
                                  Vec128<uint8_t> v) {
  return Vec512<int32_t>{_mm512_cvtepu8_epi32(v.raw)};
}
HWY_API Vec512<uint32_t> PromoteTo(Full512<uint32_t> /* tag */,
                                   Vec256<uint16_t> v) {
  return Vec512<uint32_t>{_mm512_cvtepu16_epi32(v.raw)};
}
HWY_API Vec512<int32_t> PromoteTo(Full512<int32_t> /* tag */,
                                  Vec256<uint16_t> v) {
  return Vec512<int32_t>{_mm512_cvtepu16_epi32(v.raw)};
}
HWY_API Vec512<uint64_t> PromoteTo(Full512<uint64_t> /* tag */,
                                   Vec256<uint32_t> v) {
  return Vec512<uint64_t>{_mm512_cvtepu32_epi64(v.raw)};
}

// Special case for "v" with all blocks equal (e.g. from LoadDup128):
// single-cycle latency instead of 3.
HWY_API Vec512<uint32_t> U32FromU8(const Vec512<uint8_t> v) {
  const Full512<uint32_t> d32;
  alignas(64) static constexpr uint32_t k32From8[16] = {
      0xFFFFFF00UL, 0xFFFFFF01UL, 0xFFFFFF02UL, 0xFFFFFF03UL,
      0xFFFFFF04UL, 0xFFFFFF05UL, 0xFFFFFF06UL, 0xFFFFFF07UL,
      0xFFFFFF08UL, 0xFFFFFF09UL, 0xFFFFFF0AUL, 0xFFFFFF0BUL,
      0xFFFFFF0CUL, 0xFFFFFF0DUL, 0xFFFFFF0EUL, 0xFFFFFF0FUL};
  return TableLookupBytes(BitCast(d32, v), Load(d32, k32From8));
}

// Signed: replicate sign bit.
// Note: these have 3 cycle latency; if inputs are already split across the
// 128 bit blocks (in their upper/lower halves), then ZipUpper/lo followed by
// signed shift would be faster.
HWY_API Vec512<int16_t> PromoteTo(Full512<int16_t> /* tag */,
                                  Vec256<int8_t> v) {
  return Vec512<int16_t>{_mm512_cvtepi8_epi16(v.raw)};
}
HWY_API Vec512<int32_t> PromoteTo(Full512<int32_t> /* tag */,
                                  Vec128<int8_t> v) {
  return Vec512<int32_t>{_mm512_cvtepi8_epi32(v.raw)};
}
HWY_API Vec512<int32_t> PromoteTo(Full512<int32_t> /* tag */,
                                  Vec256<int16_t> v) {
  return Vec512<int32_t>{_mm512_cvtepi16_epi32(v.raw)};
}
HWY_API Vec512<int64_t> PromoteTo(Full512<int64_t> /* tag */,
                                  Vec256<int32_t> v) {
  return Vec512<int64_t>{_mm512_cvtepi32_epi64(v.raw)};
}

// ------------------------------ Demotions (full -> part w/ narrow lanes)

HWY_API Vec256<uint16_t> DemoteTo(Full256<uint16_t> /* tag */,
                                  const Vec512<int32_t> v) {
  const Vec512<uint16_t> u16{_mm512_packus_epi32(v.raw, v.raw)};

  // Compress even u64 lanes into 256 bit.
  alignas(64) static constexpr uint64_t kLanes[8] = {0, 2, 4, 6, 0, 2, 4, 6};
  const auto idx64 = Load(Full512<uint64_t>(), kLanes);
  const Vec512<uint16_t> even{_mm512_permutexvar_epi64(idx64.raw, u16.raw)};
  return LowerHalf(even);
}

HWY_API Vec256<int16_t> DemoteTo(Full256<int16_t> /* tag */,
                                 const Vec512<int32_t> v) {
  const Vec512<int16_t> i16{_mm512_packs_epi32(v.raw, v.raw)};

  // Compress even u64 lanes into 256 bit.
  alignas(64) static constexpr uint64_t kLanes[8] = {0, 2, 4, 6, 0, 2, 4, 6};
  const auto idx64 = Load(Full512<uint64_t>(), kLanes);
  const Vec512<int16_t> even{_mm512_permutexvar_epi64(idx64.raw, i16.raw)};
  return LowerHalf(even);
}

HWY_API Vec128<uint8_t, 16> DemoteTo(Full128<uint8_t> /* tag */,
                                     const Vec512<int32_t> v) {
  const Vec512<uint16_t> u16{_mm512_packus_epi32(v.raw, v.raw)};
  const Vec512<uint8_t> u8{_mm512_packus_epi16(u16.raw, u16.raw)};

  alignas(64) static constexpr uint32_t kLanes[16] = {0, 4, 8, 12, 0, 4, 8, 12,
                                                      0, 4, 8, 12, 0, 4, 8, 12};
  const auto idx32 = Load(Full512<uint32_t>(), kLanes);
  const Vec512<uint8_t> fixed{_mm512_permutexvar_epi32(idx32.raw, u8.raw)};
  return LowerHalf(LowerHalf(fixed));
}

HWY_API Vec256<uint8_t> DemoteTo(Full256<uint8_t> /* tag */,
                                 const Vec512<int16_t> v) {
  const Vec512<uint8_t> u8{_mm512_packus_epi16(v.raw, v.raw)};

  // Compress even u64 lanes into 256 bit.
  alignas(64) static constexpr uint64_t kLanes[8] = {0, 2, 4, 6, 0, 2, 4, 6};
  const auto idx64 = Load(Full512<uint64_t>(), kLanes);
  const Vec512<uint8_t> even{_mm512_permutexvar_epi64(idx64.raw, u8.raw)};
  return LowerHalf(even);
}

HWY_API Vec128<int8_t, 16> DemoteTo(Full128<int8_t> /* tag */,
                                    const Vec512<int32_t> v) {
  const Vec512<int16_t> i16{_mm512_packs_epi32(v.raw, v.raw)};
  const Vec512<int8_t> i8{_mm512_packs_epi16(i16.raw, i16.raw)};

  alignas(64) static constexpr uint32_t kLanes[16] = {0, 4, 8, 12, 0, 4, 8, 12,
                                                      0, 4, 8, 12, 0, 4, 8, 12};
  const auto idx32 = Load(Full512<uint32_t>(), kLanes);
  const Vec512<int8_t> fixed{_mm512_permutexvar_epi32(idx32.raw, i8.raw)};
  return LowerHalf(LowerHalf(fixed));
}

HWY_API Vec256<int8_t> DemoteTo(Full256<int8_t> /* tag */,
                                const Vec512<int16_t> v) {
  const Vec512<int8_t> u8{_mm512_packs_epi16(v.raw, v.raw)};

  // Compress even u64 lanes into 256 bit.
  alignas(64) static constexpr uint64_t kLanes[8] = {0, 2, 4, 6, 0, 2, 4, 6};
  const auto idx64 = Load(Full512<uint64_t>(), kLanes);
  const Vec512<int8_t> even{_mm512_permutexvar_epi64(idx64.raw, u8.raw)};
  return LowerHalf(even);
}

HWY_API Vec256<float> DemoteTo(Full256<float> /* tag */,
                               const Vec512<double> v) {
  return Vec256<float>{_mm512_cvtpd_ps(v.raw)};
}

// For already range-limited input [0, 255].
HWY_API Vec128<uint8_t, 16> U8FromU32(const Vec512<uint32_t> v) {
  const Full512<uint32_t> d32;
  // In each 128 bit block, gather the lower byte of 4 uint32_t lanes into the
  // lowest 4 bytes.
  alignas(64) static constexpr uint32_t k8From32[4] = {0x0C080400u, ~0u, ~0u,
                                                       ~0u};
  const auto quads = TableLookupBytes(v, LoadDup128(d32, k8From32));
  // Gather the lowest 4 bytes of 4 128-bit blocks.
  alignas(64) static constexpr uint32_t kIndex32[16] = {
      0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12};
  const Vec512<uint8_t> bytes{
      _mm512_permutexvar_epi32(Load(d32, kIndex32).raw, quads.raw)};
  return LowerHalf(LowerHalf(bytes));
}

// ------------------------------ Convert integer <=> floating point

HWY_API Vec512<float> ConvertTo(Full512<float> /* tag */,
                                const Vec512<int32_t> v) {
  return Vec512<float>{_mm512_cvtepi32_ps(v.raw)};
}

HWY_API Vec512<double> ConvertTo(Full512<double> /* tag */,
                                 const Vec512<int64_t> v) {
  return Vec512<double>{_mm512_cvtepi64_pd(v.raw)};
}

// Truncates (rounds toward zero).
HWY_API Vec512<int32_t> ConvertTo(Full512<int32_t> /* tag */,
                                  const Vec512<float> v) {
  return Vec512<int32_t>{_mm512_cvttps_epi32(v.raw)};
}
HWY_API Vec512<int64_t> ConvertTo(Full512<int64_t> /* tag */,
                                  const Vec512<double> v) {
  return Vec512<int64_t>{_mm512_cvttpd_epi64(v.raw)};
}

HWY_API Vec512<int32_t> NearestInt(const Vec512<float> v) {
  return Vec512<int32_t>{_mm512_cvtps_epi32(v.raw)};
}

// ================================================== MISC

// ------------------------------ Mask

// For Clang and GCC, KORTEST wasn't added until recently.
#if HWY_COMPILER_MSVC != 0 || HWY_COMPILER_GCC >= 700 || \
    (HWY_COMPILER_CLANG >= 800 && !defined(__APPLE__))
#define HWY_COMPILER_HAS_KORTEST 1
#else
#define HWY_COMPILER_HAS_KORTEST 0
#endif

// Beware: the suffix indicates the number of mask bits, not lane size!

namespace detail {

template <typename T>
HWY_API bool AllFalse(hwy::SizeTag<1> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestz_mask64_u8(v.raw, v.raw);
#else
  return v.raw == 0;
#endif
}
template <typename T>
HWY_API bool AllFalse(hwy::SizeTag<2> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestz_mask32_u8(v.raw, v.raw);
#else
  return v.raw == 0;
#endif
}
template <typename T>
HWY_API bool AllFalse(hwy::SizeTag<4> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestz_mask16_u8(v.raw, v.raw);
#else
  return v.raw == 0;
#endif
}
template <typename T>
HWY_API bool AllFalse(hwy::SizeTag<8> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestz_mask8_u8(v.raw, v.raw);
#else
  return v.raw == 0;
#endif
}

}  // namespace detail

template <typename T>
HWY_API bool AllFalse(const Mask512<T> v) {
  return detail::AllFalse(hwy::SizeTag<sizeof(T)>(), v);
}

namespace detail {

template <typename T>
HWY_API bool AllTrue(hwy::SizeTag<1> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestc_mask64_u8(v.raw, v.raw);
#else
  return v.raw == 0xFFFFFFFFFFFFFFFFull;
#endif
}
template <typename T>
HWY_API bool AllTrue(hwy::SizeTag<2> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestc_mask32_u8(v.raw, v.raw);
#else
  return v.raw == 0xFFFFFFFFull;
#endif
}
template <typename T>
HWY_API bool AllTrue(hwy::SizeTag<4> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestc_mask16_u8(v.raw, v.raw);
#else
  return v.raw == 0xFFFFull;
#endif
}
template <typename T>
HWY_API bool AllTrue(hwy::SizeTag<8> /*tag*/, const Mask512<T> v) {
#if HWY_COMPILER_HAS_KORTEST
  return _kortestc_mask8_u8(v.raw, v.raw);
#else
  return v.raw == 0xFFull;
#endif
}

}  // namespace detail

template <typename T>
HWY_API bool AllTrue(const Mask512<T> v) {
  return detail::AllTrue(hwy::SizeTag<sizeof(T)>(), v);
}

template <typename T>
HWY_API uint64_t BitsFromMask(const Mask512<T> mask) {
  return mask.raw;
}

template <typename T>
HWY_API size_t CountTrue(const Mask512<T> mask) {
  return hwy::PopCount(mask.raw);
}

// ------------------------------ Horizontal sum (reduction)

// Returns 64-bit sums of 8-byte groups.
HWY_API Vec512<uint64_t> SumsOfU8x8(const Vec512<uint8_t> v) {
  return Vec512<uint64_t>{_mm512_sad_epu8(v.raw, _mm512_setzero_si512())};
}

namespace detail {

// Returns sum{lane[i]} in each lane. "v3210" is a replicated 128-bit block.
// Same logic as SSE4, but with Vec512 arguments.
template <typename T>
HWY_API Vec512<T> SumOfLanes(hwy::SizeTag<4> /* tag */, const Vec512<T> v3210) {
  const auto v1032 = Shuffle1032(v3210);
  const auto v31_20_31_20 = v3210 + v1032;
  const auto v20_31_20_31 = Shuffle0321(v31_20_31_20);
  return v20_31_20_31 + v31_20_31_20;
}

template <typename T>
HWY_API Vec512<T> SumOfLanes(hwy::SizeTag<8> /* tag */, const Vec512<T> v10) {
  const auto v01 = Shuffle01(v10);
  return v10 + v01;
}

}  // namespace detail

// Swaps adjacent 128-bit blocks.
HWY_API __m512i Blocks2301(__m512i v) {
  return _mm512_shuffle_i32x4(v, v, _MM_SHUFFLE(2, 3, 0, 1));
}
HWY_API __m512 Blocks2301(__m512 v) {
  return _mm512_shuffle_f32x4(v, v, _MM_SHUFFLE(2, 3, 0, 1));
}
HWY_API __m512d Blocks2301(__m512d v) {
  return _mm512_shuffle_f64x2(v, v, _MM_SHUFFLE(2, 3, 0, 1));
}

// Swaps upper/lower pairs of 128-bit blocks.
HWY_API __m512i Blocks1032(__m512i v) {
  return _mm512_shuffle_i32x4(v, v, _MM_SHUFFLE(1, 0, 3, 2));
}
HWY_API __m512 Blocks1032(__m512 v) {
  return _mm512_shuffle_f32x4(v, v, _MM_SHUFFLE(1, 0, 3, 2));
}
HWY_API __m512d Blocks1032(__m512d v) {
  return _mm512_shuffle_f64x2(v, v, _MM_SHUFFLE(1, 0, 3, 2));
}

// Supported for {uif}32x8, {uif}64x4. Returns the sum in each lane.
template <typename T>
HWY_API Vec512<T> SumOfLanes(const Vec512<T> v3210) {
  // Sum of all 128-bit blocks in each 128-bit block.
  const Vec512<T> v2301{Blocks2301(v3210.raw)};
  const Vec512<T> v32_32_10_10 = v2301 + v3210;
  const Vec512<T> v10_10_32_32{Blocks1032(v32_32_10_10.raw)};
  return detail::SumOfLanes(hwy::SizeTag<sizeof(T)>(),
                            v32_32_10_10 + v10_10_32_32);
}
