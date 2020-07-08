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

// 128-bit SSE4 vectors and operations. External include guard.

// This header is included by begin_target-inl.h, possibly inside a namespace,
// so do not include system headers (already done by highway.h). HWY_ALIGN is
// already defined unless an IDE is only parsing this file, in which case we
// include headers to avoid warnings.
#ifndef HWY_ALIGN
#include <stddef.h>
#include <stdint.h>

#include "hwy/highway.h"

#define HWY_NESTED_BEGIN  // prevent re-including this header
#undef HWY_TARGET
#define HWY_TARGET HWY_SSE4
#include "hwy/begin_target-inl.h"
#endif  // HWY_ALIGN

template <typename T>
struct Raw128 {
  using type = __m128i;
};
template <>
struct Raw128<float> {
  using type = __m128;
};
template <>
struct Raw128<double> {
  using type = __m128d;
};

template <typename T>
using Full128 = hwy::Simd<T, 16 / sizeof(T)>;

template <typename T, size_t N>
using Simd = hwy::Simd<T, N>;

template <typename T, size_t N = 16 / sizeof(T)>
class Vec128 {
  using Raw = typename Raw128<T>::type;

 public:
  // Compound assignment. Only usable if there is a corresponding non-member
  // binary operator overload. For example, only f32 and f64 support division.
  HWY_API Vec128& operator*=(const Vec128 other) {
    return *this = (*this * other);
  }
  HWY_API Vec128& operator/=(const Vec128 other) {
    return *this = (*this / other);
  }
  HWY_API Vec128& operator+=(const Vec128 other) {
    return *this = (*this + other);
  }
  HWY_API Vec128& operator-=(const Vec128 other) {
    return *this = (*this - other);
  }
  HWY_API Vec128& operator&=(const Vec128 other) {
    return *this = (*this & other);
  }
  HWY_API Vec128& operator|=(const Vec128 other) {
    return *this = (*this | other);
  }
  HWY_API Vec128& operator^=(const Vec128 other) {
    return *this = (*this ^ other);
  }

  Raw raw;
};

// Integer: FF..FF or 0. Float: MSB, all other bits undefined - see README.
template <typename T, size_t N = 16 / sizeof(T)>
class Mask128 {
  using Raw = typename Raw128<T>::type;

 public:
  Raw raw;
};

// ------------------------------ Cast

HWY_API __m128i BitCastToInteger(__m128i v) { return v; }
HWY_API __m128i BitCastToInteger(__m128 v) { return _mm_castps_si128(v); }
HWY_API __m128i BitCastToInteger(__m128d v) { return _mm_castpd_si128(v); }

template <typename T, size_t N>
HWY_API Vec128<uint8_t, N * sizeof(T)> cast_to_u8(Vec128<T, N> v) {
  return Vec128<uint8_t, N * sizeof(T)>{BitCastToInteger(v.raw)};
}

// Cannot rely on function overloading because return types differ.
template <typename T>
struct BitCastFromInteger128 {
  HWY_API __m128i operator()(__m128i v) { return v; }
};
template <>
struct BitCastFromInteger128<float> {
  HWY_API __m128 operator()(__m128i v) { return _mm_castsi128_ps(v); }
};
template <>
struct BitCastFromInteger128<double> {
  HWY_API __m128d operator()(__m128i v) { return _mm_castsi128_pd(v); }
};

template <typename T, size_t N>
HWY_API Vec128<T, N> cast_u8_to(Simd<T, N> /* tag */,
                                Vec128<uint8_t, N * sizeof(T)> v) {
  return Vec128<T, N>{BitCastFromInteger128<T>()(v.raw)};
}

template <typename T, size_t N, typename FromT>
HWY_API Vec128<T, N> BitCast(Simd<T, N> d,
                             Vec128<FromT, N * sizeof(T) / sizeof(FromT)> v) {
  return cast_u8_to(d, cast_to_u8(v));
}

// ------------------------------ Set

// Returns an all-zero vector/part.
template <typename T, size_t N, HWY_IF_LE128(T, N)>
HWY_API Vec128<T, N> Zero(Simd<T, N> /* tag */) {
  return Vec128<T, N>{_mm_setzero_si128()};
}
template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Zero(Simd<float, N> /* tag */) {
  return Vec128<float, N>{_mm_setzero_ps()};
}
template <size_t N, HWY_IF_LE128(double, N)>
HWY_API Vec128<double, N> Zero(Simd<double, N> /* tag */) {
  return Vec128<double, N>{_mm_setzero_pd()};
}

// Returns a vector/part with all lanes set to "t".
template <size_t N, HWY_IF_LE128(uint8_t, N)>
HWY_API Vec128<uint8_t, N> Set(Simd<uint8_t, N> /* tag */, const uint8_t t) {
  return Vec128<uint8_t, N>{_mm_set1_epi8(t)};
}
template <size_t N, HWY_IF_LE128(uint16_t, N)>
HWY_API Vec128<uint16_t, N> Set(Simd<uint16_t, N> /* tag */, const uint16_t t) {
  return Vec128<uint16_t, N>{_mm_set1_epi16(t)};
}
template <size_t N, HWY_IF_LE128(uint32_t, N)>
HWY_API Vec128<uint32_t, N> Set(Simd<uint32_t, N> /* tag */, const uint32_t t) {
  return Vec128<uint32_t, N>{_mm_set1_epi32(t)};
}
template <size_t N, HWY_IF_LE128(uint64_t, N)>
HWY_API Vec128<uint64_t, N> Set(Simd<uint64_t, N> /* tag */, const uint64_t t) {
  return Vec128<uint64_t, N>{_mm_set1_epi64x(t)};
}
template <size_t N, HWY_IF_LE128(int8_t, N)>
HWY_API Vec128<int8_t, N> Set(Simd<int8_t, N> /* tag */, const int8_t t) {
  return Vec128<int8_t, N>{_mm_set1_epi8(t)};
}
template <size_t N, HWY_IF_LE128(int16_t, N)>
HWY_API Vec128<int16_t, N> Set(Simd<int16_t, N> /* tag */, const int16_t t) {
  return Vec128<int16_t, N>{_mm_set1_epi16(t)};
}
template <size_t N, HWY_IF_LE128(int32_t, N)>
HWY_API Vec128<int32_t, N> Set(Simd<int32_t, N> /* tag */, const int32_t t) {
  return Vec128<int32_t, N>{_mm_set1_epi32(t)};
}
template <size_t N, HWY_IF_LE128(int64_t, N)>
HWY_API Vec128<int64_t, N> Set(Simd<int64_t, N> /* tag */, const int64_t t) {
  return Vec128<int64_t, N>{_mm_set1_epi64x(t)};
}
template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Set(Simd<float, N> /* tag */, const float t) {
  return Vec128<float, N>{_mm_set1_ps(t)};
}
template <size_t N, HWY_IF_LE128(double, N)>
HWY_API Vec128<double, N> Set(Simd<double, N> /* tag */, const double t) {
  return Vec128<double, N>{_mm_set1_pd(t)};
}

HWY_DIAGNOSTICS(push)
HWY_DIAGNOSTICS_OFF(disable : 4700, ignored "-Wuninitialized")

// Returns a vector with uninitialized elements.
template <typename T, size_t N, HWY_IF_LE128(T, N)>
HWY_API Vec128<T, N> Undefined(Simd<T, N> /* tag */) {
#ifdef __clang__
  return Vec128<T, N>{_mm_undefined_si128()};
#else
  __m128i raw;
  return Vec128<T, N>{raw};
#endif
}
template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Undefined(Simd<float, N> /* tag */) {
#ifdef __clang__
  return Vec128<float, N>{_mm_undefined_ps()};
#else
  __m128 raw;
  return Vec128<float, N>{raw};
#endif
}
template <size_t N, HWY_IF_LE128(double, N)>
HWY_API Vec128<double, N> Undefined(Simd<double, N> /* tag */) {
#ifdef __clang__
  return Vec128<double, N>{_mm_undefined_pd()};
#else
  __m128d raw;
  return Vec128<double, N>{raw};
#endif
}

HWY_DIAGNOSTICS(pop)

// ================================================== ARITHMETIC

// ------------------------------ Addition

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> operator+(const Vec128<uint8_t, N> a,
                                     const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_add_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> operator+(const Vec128<uint16_t, N> a,
                                      const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_add_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator+(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{_mm_add_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> operator+(const Vec128<uint64_t, N> a,
                                      const Vec128<uint64_t, N> b) {
  return Vec128<uint64_t, N>{_mm_add_epi64(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> operator+(const Vec128<int8_t, N> a,
                                    const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_add_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> operator+(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_add_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator+(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{_mm_add_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, N> operator+(const Vec128<int64_t, N> a,
                                     const Vec128<int64_t, N> b) {
  return Vec128<int64_t, N>{_mm_add_epi64(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> operator+(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_add_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> operator+(const Vec128<double, N> a,
                                    const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_add_pd(a.raw, b.raw)};
}

// ------------------------------ Subtraction

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> operator-(const Vec128<uint8_t, N> a,
                                     const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_sub_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> operator-(Vec128<uint16_t, N> a,
                                      Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_sub_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator-(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{_mm_sub_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> operator-(const Vec128<uint64_t, N> a,
                                      const Vec128<uint64_t, N> b) {
  return Vec128<uint64_t, N>{_mm_sub_epi64(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> operator-(const Vec128<int8_t, N> a,
                                    const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_sub_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> operator-(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_sub_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator-(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{_mm_sub_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, N> operator-(const Vec128<int64_t, N> a,
                                     const Vec128<int64_t, N> b) {
  return Vec128<int64_t, N>{_mm_sub_epi64(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> operator-(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_sub_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> operator-(const Vec128<double, N> a,
                                    const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_sub_pd(a.raw, b.raw)};
}

// ------------------------------ Saturating addition

// Returns a + b clamped to the destination range.

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> SaturatedAdd(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_adds_epu8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> SaturatedAdd(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_adds_epu16(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> SaturatedAdd(const Vec128<int8_t, N> a,
                                       const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_adds_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> SaturatedAdd(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_adds_epi16(a.raw, b.raw)};
}

// ------------------------------ Saturating subtraction

// Returns a - b clamped to the destination range.

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> SaturatedSub(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_subs_epu8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> SaturatedSub(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_subs_epu16(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> SaturatedSub(const Vec128<int8_t, N> a,
                                       const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_subs_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> SaturatedSub(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_subs_epi16(a.raw, b.raw)};
}

// ------------------------------ Average

// Returns (a + b + 1) / 2

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> AverageRound(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_avg_epu8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> AverageRound(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_avg_epu16(a.raw, b.raw)};
}

// ------------------------------ Absolute value

// Returns absolute value, except that LimitsMin() maps to LimitsMax() + 1.
template <size_t N>
HWY_API Vec128<int8_t, N> Abs(const Vec128<int8_t, N> v) {
  return Vec128<int8_t, N>{_mm_abs_epi8(v.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Abs(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{_mm_abs_epi16(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Abs(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{_mm_abs_epi32(v.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> Abs(const Vec128<float, N> v) {
  const Vec128<int32_t, N> mask{_mm_set1_epi32(0x7FFFFFFF)};
  return v & BitCast(Simd<float, N>(), mask);
}
template <size_t N>
HWY_API Vec128<double, N> Abs(const Vec128<double, N> v) {
  const Vec128<int64_t, N> mask{_mm_set1_epi64x(0x7FFFFFFFFFFFFFFFLL)};
  return v & BitCast(Simd<double, N>(), mask);
}

// ------------------------------ Shift lanes by constant #bits

// Unsigned
template <int kBits, size_t N>
HWY_API Vec128<uint16_t, N> ShiftLeft(const Vec128<uint16_t, N> v) {
  return Vec128<uint16_t, N>{_mm_slli_epi16(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint16_t, N> ShiftRight(const Vec128<uint16_t, N> v) {
  return Vec128<uint16_t, N>{_mm_srli_epi16(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint32_t, N> ShiftLeft(const Vec128<uint32_t, N> v) {
  return Vec128<uint32_t, N>{_mm_slli_epi32(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint32_t, N> ShiftRight(const Vec128<uint32_t, N> v) {
  return Vec128<uint32_t, N>{_mm_srli_epi32(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint64_t, N> ShiftLeft(const Vec128<uint64_t, N> v) {
  return Vec128<uint64_t, N>{_mm_slli_epi64(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint64_t, N> ShiftRight(const Vec128<uint64_t, N> v) {
  return Vec128<uint64_t, N>{_mm_srli_epi64(v.raw, kBits)};
}

// Signed (no i64 ShiftRight)
template <int kBits, size_t N>
HWY_API Vec128<int16_t, N> ShiftLeft(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{_mm_slli_epi16(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int16_t, N> ShiftRight(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{_mm_srai_epi16(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int32_t, N> ShiftLeft(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{_mm_slli_epi32(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int32_t, N> ShiftRight(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{_mm_srai_epi32(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int64_t, N> ShiftLeft(const Vec128<int64_t, N> v) {
  return Vec128<int64_t, N>{_mm_slli_epi64(v.raw, kBits)};
}

// ------------------------------ Shift lanes by same variable #bits

#if HWY_TARGET == HWY_SSE4

// Unsigned (no u8)
template <size_t N>
HWY_API Vec128<uint16_t, N> ShiftLeftSame(const Vec128<uint16_t, N> v,
                                          const int bits) {
  return Vec128<uint16_t, N>{_mm_sll_epi16(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> ShiftRightSame(const Vec128<uint16_t, N> v,
                                           const int bits) {
  return Vec128<uint16_t, N>{_mm_srl_epi16(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> ShiftLeftSame(const Vec128<uint32_t, N> v,
                                          const int bits) {
  return Vec128<uint32_t, N>{_mm_sll_epi32(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> ShiftRightSame(const Vec128<uint32_t, N> v,
                                           const int bits) {
  return Vec128<uint32_t, N>{_mm_srl_epi32(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> ShiftLeftSame(const Vec128<uint64_t, N> v,
                                          const int bits) {
  return Vec128<uint64_t, N>{_mm_sll_epi64(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> ShiftRightSame(const Vec128<uint64_t, N> v,
                                           const int bits) {
  return Vec128<uint64_t, N>{_mm_srl_epi64(v.raw, _mm_cvtsi32_si128(bits))};
}

// Signed (no i8,i64)
template <size_t N>
HWY_API Vec128<int16_t, N> ShiftLeftSame(const Vec128<int16_t, N> v,
                                         const int bits) {
  return Vec128<int16_t, N>{_mm_sll_epi16(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<int16_t, N> ShiftRightSame(const Vec128<int16_t, N> v,
                                          const int bits) {
  return Vec128<int16_t, N>{_mm_sra_epi16(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<int32_t, N> ShiftLeftSame(const Vec128<int32_t, N> v,
                                         const int bits) {
  return Vec128<int32_t, N>{_mm_sll_epi32(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<int32_t, N> ShiftRightSame(const Vec128<int32_t, N> v,
                                          const int bits) {
  return Vec128<int32_t, N>{_mm_sra_epi32(v.raw, _mm_cvtsi32_si128(bits))};
}
template <size_t N>
HWY_API Vec128<int64_t, N> ShiftLeftSame(const Vec128<int64_t, N> v,
                                         const int bits) {
  return Vec128<int64_t, N>{_mm_sll_epi64(v.raw, _mm_cvtsi32_si128(bits))};
}

#endif  // HWY_TARGET == HWY_SSE4

// ------------------------------ Shift lanes by independent variable #bits

#if HWY_TARGET != HWY_SSE4 || HWY_IDE

template <size_t N>
HWY_API Vec128<uint32_t, N> operator>>(const Vec128<uint32_t, N> v,
                                       const Vec128<uint32_t, N> bits) {
  return Vec128<uint32_t, N>{_mm_srlv_epi32(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator<<(const Vec128<uint32_t, N> v,
                                       const Vec128<uint32_t, N> bits) {
  return Vec128<uint32_t, N>{_mm_sllv_epi32(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> operator<<(const Vec128<uint64_t, N> v,
                                       const Vec128<uint64_t, N> bits) {
  return Vec128<uint64_t, N>{_mm_sllv_epi64(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> operator>>(const Vec128<uint64_t, N> v,
                                       const Vec128<uint64_t, N> bits) {
  return Vec128<uint64_t, N>{_mm_srlv_epi64(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator<<(const Vec128<int32_t, N> v,
                                      const Vec128<int32_t, N> bits) {
  return Vec128<int32_t, N>{_mm_sllv_epi32(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator>>(const Vec128<int32_t, N> v,
                                      const Vec128<int32_t, N> bits) {
  return Vec128<int32_t, N>{_mm_srav_epi32(v.raw, bits.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, N> operator<<(const Vec128<int64_t, N> v,
                                      const Vec128<int64_t, N> bits) {
  return Vec128<int64_t, N>{_mm_sllv_epi64(v.raw, bits.raw)};
}

#endif  // HWY_TARGET != HWY_SSE4

// ------------------------------ Minimum

// Unsigned (no u64)
template <size_t N>
HWY_API Vec128<uint8_t, N> Min(const Vec128<uint8_t, N> a,
                               const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_min_epu8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> Min(const Vec128<uint16_t, N> a,
                                const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_min_epu16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> Min(const Vec128<uint32_t, N> a,
                                const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{_mm_min_epu32(a.raw, b.raw)};
}

// Signed (no i64)
template <size_t N>
HWY_API Vec128<int8_t, N> Min(const Vec128<int8_t, N> a,
                              const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_min_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Min(const Vec128<int16_t, N> a,
                               const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_min_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Min(const Vec128<int32_t, N> a,
                               const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{_mm_min_epi32(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> Min(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_min_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> Min(const Vec128<double, N> a,
                              const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_min_pd(a.raw, b.raw)};
}

// ------------------------------ Maximum

// Unsigned (no u64)
template <size_t N>
HWY_API Vec128<uint8_t, N> Max(const Vec128<uint8_t, N> a,
                               const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{_mm_max_epu8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> Max(const Vec128<uint16_t, N> a,
                                const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_max_epu16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> Max(const Vec128<uint32_t, N> a,
                                const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{_mm_max_epu32(a.raw, b.raw)};
}

// Signed (no i64)
template <size_t N>
HWY_API Vec128<int8_t, N> Max(const Vec128<int8_t, N> a,
                              const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{_mm_max_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Max(const Vec128<int16_t, N> a,
                               const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_max_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Max(const Vec128<int32_t, N> a,
                               const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{_mm_max_epi32(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> Max(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_max_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> Max(const Vec128<double, N> a,
                              const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_max_pd(a.raw, b.raw)};
}

// ------------------------------ Integer multiplication

// Unsigned
template <size_t N>
HWY_API Vec128<uint16_t, N> operator*(const Vec128<uint16_t, N> a,
                                      const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_mullo_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator*(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{_mm_mullo_epi32(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int16_t, N> operator*(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_mullo_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator*(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{_mm_mullo_epi32(a.raw, b.raw)};
}

// Returns the upper 16 bits of a * b in each lane.
template <size_t N>
HWY_API Vec128<uint16_t, N> MulHigh(const Vec128<uint16_t, N> a,
                                    const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{_mm_mulhi_epu16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> MulHigh(const Vec128<int16_t, N> a,
                                   const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{_mm_mulhi_epi16(a.raw, b.raw)};
}

// Multiplies even lanes (0, 2 ..) and places the double-wide result into
// even and the upper half into its odd neighbor lane.
template <size_t N>
HWY_API Vec128<int64_t, (N + 1) / 2> MulEven(const Vec128<int32_t, N> a,
                                             const Vec128<int32_t, N> b) {
  return Vec128<int64_t, (N + 1) / 2>{_mm_mul_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, (N + 1) / 2> MulEven(const Vec128<uint32_t, N> a,
                                              const Vec128<uint32_t, N> b) {
  return Vec128<uint64_t, (N + 1) / 2>{_mm_mul_epu32(a.raw, b.raw)};
}

// ------------------------------ Floating-point negate

template <size_t N>
HWY_API Vec128<float, N> Neg(const Vec128<float, N> v) {
  const Simd<float, N> df;
  const Simd<uint32_t, N> du;
  const auto sign = BitCast(df, Set(du, 0x80000000u));
  return v ^ sign;
}

template <size_t N>
HWY_API Vec128<double, N> Neg(const Vec128<double, N> v) {
  const Simd<double, N> df;
  const Simd<uint64_t, N> du;
  const auto sign = BitCast(df, Set(du, 0x8000000000000000ull));
  return v ^ sign;
}

// ------------------------------ Floating-point mul / div

template <size_t N>
HWY_API Vec128<float, N> operator*(Vec128<float, N> a, Vec128<float, N> b) {
  return Vec128<float, N>{_mm_mul_ps(a.raw, b.raw)};
}
HWY_API Vec128<float, 1> operator*(const Vec128<float, 1> a,
                                   const Vec128<float, 1> b) {
  return Vec128<float, 1>{_mm_mul_ss(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> operator*(const Vec128<double, N> a,
                                    const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_mul_pd(a.raw, b.raw)};
}
HWY_API Vec128<double, 1> operator*(const Vec128<double, 1> a,
                                    const Vec128<double, 1> b) {
  return Vec128<double, 1>{_mm_mul_sd(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> operator/(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_div_ps(a.raw, b.raw)};
}
HWY_API Vec128<float, 1> operator/(const Vec128<float, 1> a,
                                   const Vec128<float, 1> b) {
  return Vec128<float, 1>{_mm_div_ss(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> operator/(const Vec128<double, N> a,
                                    const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_div_pd(a.raw, b.raw)};
}
HWY_API Vec128<double, 1> operator/(const Vec128<double, 1> a,
                                    const Vec128<double, 1> b) {
  return Vec128<double, 1>{_mm_div_sd(a.raw, b.raw)};
}

// Approximate reciprocal
template <size_t N>
HWY_API Vec128<float, N> ApproximateReciprocal(const Vec128<float, N> v) {
  return Vec128<float, N>{_mm_rcp_ps(v.raw)};
}
HWY_API Vec128<float, 1> ApproximateReciprocal(const Vec128<float, 1> v) {
  return Vec128<float, 1>{_mm_rcp_ss(v.raw)};
}

// Absolute value of difference.
template <size_t N>
HWY_API Vec128<float, N> AbsDiff(const Vec128<float, N> a,
                                 const Vec128<float, N> b) {
  return Abs(a - b);
}

// ------------------------------ Floating-point multiply-add variants

// Returns mul * x + add
template <size_t N>
HWY_API Vec128<float, N> MulAdd(const Vec128<float, N> mul,
                                const Vec128<float, N> x,
                                const Vec128<float, N> add) {
#if HWY_TARGET == HWY_SSE4
  return mul * x + add;
#else
  return Vec128<float, N>{_mm_fmadd_ps(mul.raw, x.raw, add.raw)};
#endif
}
template <size_t N>
HWY_API Vec128<double, N> MulAdd(const Vec128<double, N> mul,
                                 const Vec128<double, N> x,
                                 const Vec128<double, N> add) {
#if HWY_TARGET == HWY_SSE4
  return mul * x + add;
#else
  return Vec128<double, N>{_mm_fmadd_pd(mul.raw, x.raw, add.raw)};
#endif
}

// Returns add - mul * x
template <size_t N>
HWY_API Vec128<float, N> NegMulAdd(const Vec128<float, N> mul,
                                   const Vec128<float, N> x,
                                   const Vec128<float, N> add) {
#if HWY_TARGET == HWY_SSE4
  return add - mul * x;
#else
  return Vec128<float, N>{_mm_fnmadd_ps(mul.raw, x.raw, add.raw)};
#endif
}
template <size_t N>
HWY_API Vec128<double, N> NegMulAdd(const Vec128<double, N> mul,
                                    const Vec128<double, N> x,
                                    const Vec128<double, N> add) {
#if HWY_TARGET == HWY_SSE4
  return add - mul * x;
#else
  return Vec128<double, N>{_mm_fnmadd_pd(mul.raw, x.raw, add.raw)};
#endif
}

// Returns mul * x - sub
template <size_t N>
HWY_API Vec128<float, N> MulSub(const Vec128<float, N> mul,
                                const Vec128<float, N> x,
                                const Vec128<float, N> sub) {
#if HWY_TARGET == HWY_SSE4
  return mul * x - sub;
#else
  return Vec128<float, N>{_mm_fmsub_ps(mul.raw, x.raw, sub.raw)};
#endif
}
template <size_t N>
HWY_API Vec128<double, N> MulSub(const Vec128<double, N> mul,
                                 const Vec128<double, N> x,
                                 const Vec128<double, N> sub) {
#if HWY_TARGET == HWY_SSE4
  return mul * x - sub;
#else
  return Vec128<double, N>{_mm_fmsub_pd(mul.raw, x.raw, sub.raw)};
#endif
}

// Returns -mul * x - sub
template <size_t N>
HWY_API Vec128<float, N> NegMulSub(const Vec128<float, N> mul,
                                   const Vec128<float, N> x,
                                   const Vec128<float, N> sub) {
#if HWY_TARGET == HWY_SSE4
  return Neg(mul) * x - sub;
#else
  return Vec128<float, N>{_mm_fnmsub_ps(mul.raw, x.raw, sub.raw)};
#endif
}
template <size_t N>
HWY_API Vec128<double, N> NegMulSub(const Vec128<double, N> mul,
                                    const Vec128<double, N> x,
                                    const Vec128<double, N> sub) {
#if HWY_TARGET == HWY_SSE4
  return Neg(mul) * x - sub;
#else
  return Vec128<double, N>{_mm_fnmsub_pd(mul.raw, x.raw, sub.raw)};
#endif
}

// ------------------------------ Floating-point square root

// Full precision square root
template <size_t N>
HWY_API Vec128<float, N> Sqrt(const Vec128<float, N> v) {
  return Vec128<float, N>{_mm_sqrt_ps(v.raw)};
}
HWY_API Vec128<float, 1> Sqrt(const Vec128<float, 1> v) {
  return Vec128<float, 1>{_mm_sqrt_ss(v.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> Sqrt(const Vec128<double, N> v) {
  return Vec128<double, N>{_mm_sqrt_pd(v.raw)};
}
HWY_API Vec128<double, 1> Sqrt(const Vec128<double, 1> v) {
  return Vec128<double, 1>{_mm_sqrt_sd(_mm_setzero_pd(), v.raw)};
}

// Approximate reciprocal square root
template <size_t N>
HWY_API Vec128<float, N> ApproximateReciprocalSqrt(const Vec128<float, N> v) {
  return Vec128<float, N>{_mm_rsqrt_ps(v.raw)};
}
HWY_API Vec128<float, 1> ApproximateReciprocalSqrt(const Vec128<float, 1> v) {
  return Vec128<float, 1>{_mm_rsqrt_ss(v.raw)};
}

// ------------------------------ Floating-point rounding

// Toward nearest integer, ties to even
template <size_t N>
HWY_API Vec128<float, N> Round(const Vec128<float, N> v) {
  return Vec128<float, N>{
      _mm_round_ps(v.raw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)};
}
template <size_t N>
HWY_API Vec128<double, N> Round(const Vec128<double, N> v) {
  return Vec128<double, N>{
      _mm_round_pd(v.raw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)};
}

// Toward zero, aka truncate
template <size_t N>
HWY_API Vec128<float, N> Trunc(const Vec128<float, N> v) {
  return Vec128<float, N>{
      _mm_round_ps(v.raw, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)};
}
template <size_t N>
HWY_API Vec128<double, N> Trunc(const Vec128<double, N> v) {
  return Vec128<double, N>{
      _mm_round_pd(v.raw, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)};
}

// Toward +infinity, aka ceiling
template <size_t N>
HWY_API Vec128<float, N> Ceil(const Vec128<float, N> v) {
  return Vec128<float, N>{
      _mm_round_ps(v.raw, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)};
}
template <size_t N>
HWY_API Vec128<double, N> Ceil(const Vec128<double, N> v) {
  return Vec128<double, N>{
      _mm_round_pd(v.raw, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)};
}

// Toward -infinity, aka floor
template <size_t N>
HWY_API Vec128<float, N> Floor(const Vec128<float, N> v) {
  return Vec128<float, N>{
      _mm_round_ps(v.raw, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)};
}
template <size_t N>
HWY_API Vec128<double, N> Floor(const Vec128<double, N> v) {
  return Vec128<double, N>{
      _mm_round_pd(v.raw, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)};
}

// ================================================== COMPARE

// Comparisons fill a lane with 1-bits if the condition is true, else 0.

// ------------------------------ Equality

// Unsigned
template <size_t N>
HWY_API Mask128<uint8_t, N> operator==(const Vec128<uint8_t, N> a,
                                       const Vec128<uint8_t, N> b) {
  return Mask128<uint8_t, N>{_mm_cmpeq_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<uint16_t, N> operator==(const Vec128<uint16_t, N> a,
                                        const Vec128<uint16_t, N> b) {
  return Mask128<uint16_t, N>{_mm_cmpeq_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<uint32_t, N> operator==(const Vec128<uint32_t, N> a,
                                        const Vec128<uint32_t, N> b) {
  return Mask128<uint32_t, N>{_mm_cmpeq_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<uint64_t, N> operator==(const Vec128<uint64_t, N> a,
                                        const Vec128<uint64_t, N> b) {
  return Mask128<uint64_t, N>{_mm_cmpeq_epi64(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Mask128<int8_t, N> operator==(const Vec128<int8_t, N> a,
                                      const Vec128<int8_t, N> b) {
  return Mask128<int8_t, N>{_mm_cmpeq_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator==(Vec128<int16_t, N> a,
                                       Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{_mm_cmpeq_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator==(const Vec128<int32_t, N> a,
                                       const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{_mm_cmpeq_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int64_t, N> operator==(const Vec128<int64_t, N> a,
                                       const Vec128<int64_t, N> b) {
  return Mask128<int64_t, N>{_mm_cmpeq_epi64(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Mask128<float, N> operator==(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{_mm_cmpeq_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<double, N> operator==(const Vec128<double, N> a,
                                      const Vec128<double, N> b) {
  return Mask128<double, N>{_mm_cmpeq_pd(a.raw, b.raw)};
}

template <typename T, size_t N>
HWY_API Mask128<T, N> TestBit(Vec128<T, N> v, Vec128<T, N> bit) {
  static_assert(!hwy::IsFloat<T>(), "Only integer vectors supported");
  return (v & bit) == bit;
}

// ------------------------------ Strict inequality

// Signed/float <
template <size_t N>
HWY_API Mask128<int8_t, N> operator<(const Vec128<int8_t, N> a,
                                     const Vec128<int8_t, N> b) {
  return Mask128<int8_t, N>{_mm_cmpgt_epi8(b.raw, a.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator<(const Vec128<int16_t, N> a,
                                      const Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{_mm_cmpgt_epi16(b.raw, a.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator<(const Vec128<int32_t, N> a,
                                      const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{_mm_cmpgt_epi32(b.raw, a.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator<(const Vec128<float, N> a,
                                    const Vec128<float, N> b) {
  return Mask128<float, N>{_mm_cmplt_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<double, N> operator<(const Vec128<double, N> a,
                                     const Vec128<double, N> b) {
  return Mask128<double, N>{_mm_cmplt_pd(a.raw, b.raw)};
}

// Signed/float >
template <size_t N>
HWY_API Mask128<int8_t, N> operator>(const Vec128<int8_t, N> a,
                                     const Vec128<int8_t, N> b) {
  return Mask128<int8_t, N>{_mm_cmpgt_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator>(const Vec128<int16_t, N> a,
                                      const Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{_mm_cmpgt_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator>(const Vec128<int32_t, N> a,
                                      const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{_mm_cmpgt_epi32(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator>(const Vec128<float, N> a,
                                    const Vec128<float, N> b) {
  return Mask128<float, N>{_mm_cmpgt_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<double, N> operator>(const Vec128<double, N> a,
                                     const Vec128<double, N> b) {
  return Mask128<double, N>{_mm_cmpgt_pd(a.raw, b.raw)};
}

#if HWY_TARGET != HWY_SSE4 || HWY_IDE

template <size_t N>
HWY_API Mask128<int64_t, N> operator<(const Vec128<int64_t, N> a,
                                      const Vec128<int64_t, N> b) {
  return Mask128<int64_t, N>{_mm_cmpgt_epi64(b.raw, a.raw)};
}

template <size_t N>
HWY_API Mask128<int64_t, N> operator>(const Vec128<int64_t, N> a,
                                      const Vec128<int64_t, N> b) {
  return Mask128<int64_t, N>{_mm_cmpgt_epi64(a.raw, b.raw)};
}

#endif  // HWY_TARGET != HWY_SSE4

// ------------------------------ Weak inequality

// Float <= >=
template <size_t N>
HWY_API Mask128<float, N> operator<=(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{_mm_cmple_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<double, N> operator<=(const Vec128<double, N> a,
                                      const Vec128<double, N> b) {
  return Mask128<double, N>{_mm_cmple_pd(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator>=(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{_mm_cmpge_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<double, N> operator>=(const Vec128<double, N> a,
                                      const Vec128<double, N> b) {
  return Mask128<double, N>{_mm_cmpge_pd(a.raw, b.raw)};
}

// ================================================== LOGICAL

// ------------------------------ Bitwise AND

template <typename T, size_t N>
HWY_API Vec128<T, N> And(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{_mm_and_si128(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<float, N> And(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_and_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> And(const Vec128<double, N> a,
                              const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_and_pd(a.raw, b.raw)};
}

// ------------------------------ Bitwise AND-NOT

// Returns ~not_mask & mask.
template <typename T, size_t N>
HWY_API Vec128<T, N> AndNot(Vec128<T, N> not_mask, Vec128<T, N> mask) {
  return Vec128<T, N>{_mm_andnot_si128(not_mask.raw, mask.raw)};
}
template <size_t N>
HWY_API Vec128<float, N> AndNot(const Vec128<float, N> not_mask,
                                const Vec128<float, N> mask) {
  return Vec128<float, N>{_mm_andnot_ps(not_mask.raw, mask.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> AndNot(const Vec128<double, N> not_mask,
                                 const Vec128<double, N> mask) {
  return Vec128<double, N>{_mm_andnot_pd(not_mask.raw, mask.raw)};
}

// ------------------------------ Bitwise OR

template <typename T, size_t N>
HWY_API Vec128<T, N> Or(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{_mm_or_si128(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> Or(const Vec128<float, N> a,
                            const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_or_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> Or(const Vec128<double, N> a,
                             const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_or_pd(a.raw, b.raw)};
}

// ------------------------------ Bitwise XOR

template <typename T, size_t N>
HWY_API Vec128<T, N> Xor(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{_mm_xor_si128(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> Xor(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{_mm_xor_ps(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> Xor(const Vec128<double, N> a,
                              const Vec128<double, N> b) {
  return Vec128<double, N>{_mm_xor_pd(a.raw, b.raw)};
}

// ------------------------------ Operator overloads (internal-only if float)

template <typename T, size_t N>
HWY_API Vec128<T, N> operator&(const Vec128<T, N> a, const Vec128<T, N> b) {
  return And(a, b);
}

template <typename T, size_t N>
HWY_API Vec128<T, N> operator|(const Vec128<T, N> a, const Vec128<T, N> b) {
  return Or(a, b);
}

template <typename T, size_t N>
HWY_API Vec128<T, N> operator^(const Vec128<T, N> a, const Vec128<T, N> b) {
  return Xor(a, b);
}

// ------------------------------ Mask

// Mask and Vec are the same (true = FF..FF).
template <typename T, size_t N>
HWY_API Mask128<T, N> MaskFromVec(const Vec128<T, N> v) {
  return Mask128<T, N>{v.raw};
}

template <typename T, size_t N>
HWY_API Vec128<T, N> VecFromMask(const Mask128<T, N> v) {
  return Vec128<T, N>{v.raw};
}

// mask ? yes : no
template <typename T, size_t N>
HWY_API Vec128<T, N> IfThenElse(Mask128<T, N> mask, Vec128<T, N> yes,
                                Vec128<T, N> no) {
  return Vec128<T, N>{_mm_blendv_epi8(no.raw, yes.raw, mask.raw)};
}
template <size_t N>
HWY_API Vec128<float, N> IfThenElse(const Mask128<float, N> mask,
                                    const Vec128<float, N> yes,
                                    const Vec128<float, N> no) {
  return Vec128<float, N>{_mm_blendv_ps(no.raw, yes.raw, mask.raw)};
}
template <size_t N>
HWY_API Vec128<double, N> IfThenElse(const Mask128<double, N> mask,
                                     const Vec128<double, N> yes,
                                     const Vec128<double, N> no) {
  return Vec128<double, N>{_mm_blendv_pd(no.raw, yes.raw, mask.raw)};
}

// mask ? yes : 0
template <typename T, size_t N>
HWY_API Vec128<T, N> IfThenElseZero(Mask128<T, N> mask, Vec128<T, N> yes) {
  return yes & VecFromMask(mask);
}

// mask ? 0 : no
template <typename T, size_t N>
HWY_API Vec128<T, N> IfThenZeroElse(Mask128<T, N> mask, Vec128<T, N> no) {
  return AndNot(VecFromMask(mask), no);
}

template <typename T, size_t N, HWY_IF_FLOAT(T)>
HWY_API Vec128<T, N> ZeroIfNegative(Vec128<T, N> v) {
  const Simd<T, N> d;
  return IfThenElse(MaskFromVec(v), Zero(d), v);
}

// ================================================== MEMORY

// Clang static analysis claims the memory immediately after a partial vector
// store is uninitialized, and also flags the input to partial loads (at least
// for loadl_pd) as "garbage". This is a false alarm because msan does not
// raise errors. We work around this by using CopyBytes instead of intrinsics,
// but only for the analyzer to avoid potentially bad code generation.
// Unfortunately __clang_analyzer__ was not defined for clang-tidy prior to v7.
#ifndef HWY_SAFE_PARTIAL_LOAD_STORE
#if defined(__clang_analyzer__) || \
    (HWY_COMPILER_CLANG != 0 && HWY_COMPILER_CLANG < 700)
#define HWY_SAFE_PARTIAL_LOAD_STORE 1
#else
#define HWY_SAFE_PARTIAL_LOAD_STORE 0
#endif
#endif  // HWY_SAFE_PARTIAL_LOAD_STORE

// ------------------------------ Load

template <typename T>
HWY_API Vec128<T> Load(Full128<T> /* tag */, const T* HWY_RESTRICT aligned) {
  return Vec128<T>{_mm_load_si128(reinterpret_cast<const __m128i*>(aligned))};
}
HWY_API Vec128<float> Load(Full128<float> /* tag */,
                           const float* HWY_RESTRICT aligned) {
  return Vec128<float>{_mm_load_ps(aligned)};
}
HWY_API Vec128<double> Load(Full128<double> /* tag */,
                            const double* HWY_RESTRICT aligned) {
  return Vec128<double>{_mm_load_pd(aligned)};
}

template <typename T>
HWY_API Vec128<T> LoadU(Full128<T> /* tag */, const T* HWY_RESTRICT p) {
  return Vec128<T>{_mm_loadu_si128(reinterpret_cast<const __m128i*>(p))};
}
HWY_API Vec128<float> LoadU(Full128<float> /* tag */,
                            const float* HWY_RESTRICT p) {
  return Vec128<float>{_mm_loadu_ps(p)};
}
HWY_API Vec128<double> LoadU(Full128<double> /* tag */,
                             const double* HWY_RESTRICT p) {
  return Vec128<double>{_mm_loadu_pd(p)};
}

template <typename T>
HWY_API Vec128<T, 8 / sizeof(T)> Load(Simd<T, 8 / sizeof(T)> /* tag */,
                                      const T* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  __m128i v = _mm_setzero_si128();
  hwy::CopyBytes<8>(p, &v);
  return Vec128<T, 8 / sizeof(T)>{v};
#else
  return Vec128<T, 8 / sizeof(T)>{
      _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p))};
#endif
}

HWY_API Vec128<float, 2> Load(Simd<float, 2> /* tag */,
                              const float* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  __m128 v = _mm_setzero_ps();
  hwy::CopyBytes<8>(p, &v);
  return Vec128<float, 2>{v};
#else
  const __m128 hi = _mm_setzero_ps();
  return Vec128<float, 2>{_mm_loadl_pi(hi, reinterpret_cast<const __m64*>(p))};
#endif
}

HWY_API Vec128<double, 1> Load(Simd<double, 1> /* tag */,
                               const double* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  __m128d v = _mm_setzero_pd();
  hwy::CopyBytes<8>(p, &v);
  return Vec128<double, 1>{v};
#else
  return Vec128<double, 1>{_mm_load_sd(p)};
#endif
}

HWY_API Vec128<float, 1> Load(Simd<float, 1> /* tag */,
                              const float* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  __m128 v = _mm_setzero_ps();
  hwy::CopyBytes<4>(p, &v);
  return Vec128<float, 1>{v};
#else
  return Vec128<float, 1>{_mm_load_ss(p)};
#endif
}

// Any <= 32 bit except <float, 1>
template <typename T, size_t N, HWY_IF_LE32(T, N)>
HWY_API Vec128<T, N> Load(Simd<T, N> /* tag */, const T* HWY_RESTRICT p) {
  constexpr size_t kSize = sizeof(T) * N;
#if HWY_SAFE_PARTIAL_LOAD_STORE
  __m128 v = _mm_setzero_ps();
  hwy::CopyBytes<kSize>(p, &v);
  return Vec128<T, N>{v};
#else
  // TODO(janwas): load_ss?
  int32_t bits;
  hwy::CopyBytes<kSize>(p, &bits);
  return Vec128<T, N>{_mm_cvtsi32_si128(bits)};
#endif
}

// For < 128 bit, LoadU == Load.
template <typename T, size_t N, HWY_IF_LE64(T, N)>
HWY_API Vec128<T, N> LoadU(Simd<T, N> d, const T* HWY_RESTRICT p) {
  return Load(d, p);
}

// 128-bit SIMD => nothing to duplicate, same as an unaligned load.
template <typename T, size_t N, HWY_IF_LE128(T, N)>
HWY_API Vec128<T, N> LoadDup128(Simd<T, N> d, const T* HWY_RESTRICT p) {
  return LoadU(d, p);
}

// ------------------------------ Store

template <typename T>
HWY_API void Store(Vec128<T> v, Full128<T> /* tag */, T* HWY_RESTRICT aligned) {
  _mm_store_si128(reinterpret_cast<__m128i*>(aligned), v.raw);
}
HWY_API void Store(const Vec128<float> v, Full128<float> /* tag */,
                   float* HWY_RESTRICT aligned) {
  _mm_store_ps(aligned, v.raw);
}
HWY_API void Store(const Vec128<double> v, Full128<double> /* tag */,
                   double* HWY_RESTRICT aligned) {
  _mm_store_pd(aligned, v.raw);
}

template <typename T>
HWY_API void StoreU(Vec128<T> v, Full128<T> /* tag */, T* HWY_RESTRICT p) {
  _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v.raw);
}
HWY_API void StoreU(const Vec128<float> v, Full128<float> /* tag */,
                    float* HWY_RESTRICT p) {
  _mm_storeu_ps(p, v.raw);
}
HWY_API void StoreU(const Vec128<double> v, Full128<double> /* tag */,
                    double* HWY_RESTRICT p) {
  _mm_storeu_pd(p, v.raw);
}

template <typename T>
HWY_API void Store(Vec128<T, 8 / sizeof(T)> v, Simd<T, 8 / sizeof(T)> /* tag */,
                   T* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  hwy::CopyBytes<8>(&v, p);
#else
  _mm_storel_epi64(reinterpret_cast<__m128i*>(p), v.raw);
#endif
}
HWY_API void Store(const Vec128<float, 2> v, Simd<float, 2> /* tag */,
                   float* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  hwy::CopyBytes<8>(&v, p);
#else
  _mm_storel_pi(reinterpret_cast<__m64*>(p), v.raw);
#endif
}
HWY_API void Store(const Vec128<double, 1> v, Simd<double, 1> /* tag */,
                   double* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  hwy::CopyBytes<8>(&v, p);
#else
  _mm_storel_pd(p, v.raw);
#endif
}

// Any <= 32 bit except <float, 1>
template <typename T, size_t N, HWY_IF_LE32(T, N)>
HWY_API void Store(Vec128<T, N> v, Simd<T, N> /* tag */, T* HWY_RESTRICT p) {
  hwy::CopyBytes<sizeof(T) * N>(&v, p);
}
HWY_API void Store(const Vec128<float, 1> v, Simd<float, 1> /* tag */,
                   float* HWY_RESTRICT p) {
#if HWY_SAFE_PARTIAL_LOAD_STORE
  hwy::CopyBytes<4>(&v, p);
#else
  _mm_store_ss(p, v.raw);
#endif
}

// For < 128 bit, StoreU == Store.
template <typename T, size_t N, HWY_IF_LE64(T, N)>
HWY_API void StoreU(const Vec128<T, N> v, Simd<T, N> d, T* HWY_RESTRICT p) {
  Store(v, d, p);
}

// ------------------------------ Non-temporal stores

// On clang6, we see incorrect code generated for _mm_stream_pi, so
// round even partial vectors up to 16 bytes.
template <typename T, size_t N>
HWY_API void Stream(Vec128<T, N> v, Simd<T, N> /* tag */,
                    T* HWY_RESTRICT aligned) {
  _mm_stream_si128(reinterpret_cast<__m128i*>(aligned), v.raw);
}
template <size_t N>
HWY_API void Stream(const Vec128<float, N> v, Simd<float, N> /* tag */,
                    float* HWY_RESTRICT aligned) {
  _mm_stream_ps(aligned, v.raw);
}
template <size_t N>
HWY_API void Stream(const Vec128<double, N> v, Simd<double, N> /* tag */,
                    double* HWY_RESTRICT aligned) {
  _mm_stream_pd(aligned, v.raw);
}

// ------------------------------ Gather

#if HWY_TARGET != HWY_SSE4 || HWY_IDE

namespace detail {

template <typename T, size_t N>
HWY_API Vec128<T, N> GatherOffset(hwy::SizeTag<4> /* tag */,
                                  Simd<T, N> /* tag */,
                                  const T* HWY_RESTRICT base,
                                  const Vec128<int32_t, N> offset) {
  return Vec128<T, N>{_mm_i32gather_epi32(
      reinterpret_cast<const int32_t*>(base), offset.raw, 1)};
}
template <typename T, size_t N>
HWY_API Vec128<T, N> GatherIndex(hwy::SizeTag<4> /* tag */,
                                 Simd<T, N> /* tag */,
                                 const T* HWY_RESTRICT base,
                                 const Vec128<int32_t, N> index) {
  return Vec128<T, N>{_mm_i32gather_epi32(
      reinterpret_cast<const int32_t*>(base), index.raw, 4)};
}

template <typename T, size_t N>
HWY_API Vec128<T, N> GatherOffset(hwy::SizeTag<8> /* tag */,
                                  Simd<T, N> /* tag */,
                                  const T* HWY_RESTRICT base,
                                  const Vec128<int64_t, N> offset) {
  return Vec128<T, N>{_mm_i64gather_epi64(
      reinterpret_cast<const hwy::GatherIndex64*>(base), offset.raw, 1)};
}
template <typename T, size_t N>
HWY_API Vec128<T, N> GatherIndex(hwy::SizeTag<8> /* tag */,
                                 Simd<T, N> /* tag */,
                                 const T* HWY_RESTRICT base,
                                 const Vec128<int64_t, N> index) {
  return Vec128<T, N>{_mm_i64gather_epi64(
      reinterpret_cast<const hwy::GatherIndex64*>(base), index.raw, 8)};
}

}  // namespace detail

template <typename T, size_t N, typename Offset>
HWY_API Vec128<T, N> GatherOffset(Simd<T, N> d, const T* HWY_RESTRICT base,
                                  const Vec128<Offset, N> offset) {
  return detail::GatherOffset(hwy::SizeTag<sizeof(T)>(), d, base, offset);
}
template <typename T, size_t N, typename Index>
HWY_API Vec128<T, N> GatherIndex(Simd<T, N> d, const T* HWY_RESTRICT base,
                                 const Vec128<Index, N> index) {
  return detail::GatherIndex(hwy::SizeTag<sizeof(T)>(), d, base, index);
}

template <size_t N>
HWY_API Vec128<float, N> GatherOffset(Simd<float, N> /* tag */,
                                      const float* HWY_RESTRICT base,
                                      const Vec128<int32_t, N> offset) {
  return Vec128<float, N>{_mm_i32gather_ps(base, offset.raw, 1)};
}
template <size_t N>
HWY_API Vec128<float, N> GatherIndex(Simd<float, N> /* tag */,
                                     const float* HWY_RESTRICT base,
                                     const Vec128<int32_t, N> index) {
  return Vec128<float, N>{_mm_i32gather_ps(base, index.raw, 4)};
}

template <size_t N>
HWY_API Vec128<double, N> GatherOffset(Simd<double, N> /* tag */,
                                       const double* HWY_RESTRICT base,
                                       const Vec128<int64_t, N> offset) {
  return Vec128<double, N>{_mm_i64gather_pd(base, offset.raw, 1)};
}
template <size_t N>
HWY_API Vec128<double, N> GatherIndex(Simd<double, N> /* tag */,
                                      const double* HWY_RESTRICT base,
                                      const Vec128<int64_t, N> index) {
  return Vec128<double, N>{_mm_i64gather_pd(base, index.raw, 8)};
}

#endif  // HWY_TARGET != HWY_SSE4

// ================================================== SWIZZLE

// ------------------------------ Extract lane

// Gets the single value stored in a vector/part.
template <size_t N>
HWY_API uint16_t GetLane(const Vec128<uint16_t, N> v) {
  return _mm_cvtsi128_si32(v.raw) & 0xFFFF;
}
template <size_t N>
HWY_API int16_t GetLane(const Vec128<int16_t, N> v) {
  return _mm_cvtsi128_si32(v.raw) & 0xFFFF;
}
template <size_t N>
HWY_API uint32_t GetLane(const Vec128<uint32_t, N> v) {
  return _mm_cvtsi128_si32(v.raw);
}
template <size_t N>
HWY_API int32_t GetLane(const Vec128<int32_t, N> v) {
  return _mm_cvtsi128_si32(v.raw);
}
template <size_t N>
HWY_API float GetLane(const Vec128<float, N> v) {
  return _mm_cvtss_f32(v.raw);
}
template <size_t N>
HWY_API uint64_t GetLane(const Vec128<uint64_t, N> v) {
  return _mm_cvtsi128_si64(v.raw);
}
template <size_t N>
HWY_API int64_t GetLane(const Vec128<int64_t, N> v) {
  return _mm_cvtsi128_si64(v.raw);
}
template <size_t N>
HWY_API double GetLane(const Vec128<double, N> v) {
  return _mm_cvtsd_f64(v.raw);
}

// ------------------------------ Extract half

// Returns upper/lower half of a vector.
template <typename T, size_t N>
HWY_API Vec128<T, N / 2> LowerHalf(Vec128<T, N> v) {
  return Vec128<T, N / 2>{v.raw};
}

// These copy hi into lo (smaller instruction encoding than shifts).
template <typename T>
HWY_API Vec128<T, 8 / sizeof(T)> UpperHalf(Vec128<T> v) {
  return Vec128<T, 8 / sizeof(T)>{_mm_unpackhi_epi64(v.raw, v.raw)};
}
template <>
HWY_API Vec128<float, 2> UpperHalf(Vec128<float> v) {
  return Vec128<float, 2>{_mm_movehl_ps(v.raw, v.raw)};
}
template <>
HWY_API Vec128<double, 1> UpperHalf(Vec128<double> v) {
  return Vec128<double, 1>{_mm_unpackhi_pd(v.raw, v.raw)};
}

// ------------------------------ Shift vector by constant #bytes

// 0x01..0F, kBytes = 1 => 0x02..0F00
template <int kBytes, typename T>
HWY_API Vec128<T> ShiftLeftBytes(const Vec128<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  return Vec128<T>{_mm_slli_si128(v.raw, kBytes)};
}

template <int kLanes, typename T>
HWY_API Vec128<T> ShiftLeftLanes(const Vec128<T> v) {
  return ShiftLeftBytes<kLanes * sizeof(T)>(v);
}

// 0x01..0F, kBytes = 1 => 0x0001..0E
template <int kBytes, typename T>
HWY_API Vec128<T> ShiftRightBytes(const Vec128<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  return Vec128<T>{_mm_srli_si128(v.raw, kBytes)};
}

template <int kLanes, typename T>
HWY_API Vec128<T> ShiftRightLanes(const Vec128<T> v) {
  return ShiftRightBytes<kLanes * sizeof(T)>(v);
}

// ------------------------------ Extract from 2x 128-bit at constant offset

// Extracts 128 bits from <hi, lo> by skipping the least-significant kBytes.
template <int kBytes, typename T>
HWY_API Vec128<T> CombineShiftRightBytes(const Vec128<T> hi,
                                         const Vec128<T> lo) {
  const Full128<uint8_t> d8;
  const Vec128<uint8_t> extracted_bytes{
      _mm_alignr_epi8(BitCast(d8, hi).raw, BitCast(d8, lo).raw, kBytes)};
  return BitCast(Full128<T>(), extracted_bytes);
}

// ------------------------------ Broadcast/splat any lane

// Unsigned
template <int kLane, size_t N>
HWY_API Vec128<uint16_t, N> Broadcast(const Vec128<uint16_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  if (kLane < 4) {
    const __m128i lo = _mm_shufflelo_epi16(v.raw, (0x55 * kLane) & 0xFF);
    return Vec128<uint16_t, N>{_mm_unpacklo_epi64(lo, lo)};
  } else {
    const __m128i hi = _mm_shufflehi_epi16(v.raw, (0x55 * (kLane - 4)) & 0xFF);
    return Vec128<uint16_t, N>{_mm_unpackhi_epi64(hi, hi)};
  }
}
template <int kLane, size_t N>
HWY_API Vec128<uint32_t, N> Broadcast(const Vec128<uint32_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<uint32_t, N>{_mm_shuffle_epi32(v.raw, 0x55 * kLane)};
}
template <int kLane, size_t N>
HWY_API Vec128<uint64_t, N> Broadcast(const Vec128<uint64_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<uint64_t, N>{_mm_shuffle_epi32(v.raw, kLane ? 0xEE : 0x44)};
}

// Signed
template <int kLane, size_t N>
HWY_API Vec128<int16_t, N> Broadcast(const Vec128<int16_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  if (kLane < 4) {
    const __m128i lo = _mm_shufflelo_epi16(v.raw, (0x55 * kLane) & 0xFF);
    return Vec128<int16_t, N>{_mm_unpacklo_epi64(lo, lo)};
  } else {
    const __m128i hi = _mm_shufflehi_epi16(v.raw, (0x55 * (kLane - 4)) & 0xFF);
    return Vec128<int16_t, N>{_mm_unpackhi_epi64(hi, hi)};
  }
}
template <int kLane, size_t N>
HWY_API Vec128<int32_t, N> Broadcast(const Vec128<int32_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<int32_t, N>{_mm_shuffle_epi32(v.raw, 0x55 * kLane)};
}
template <int kLane, size_t N>
HWY_API Vec128<int64_t, N> Broadcast(const Vec128<int64_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<int64_t, N>{_mm_shuffle_epi32(v.raw, kLane ? 0xEE : 0x44)};
}

// Float
template <int kLane, size_t N>
HWY_API Vec128<float, N> Broadcast(const Vec128<float, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<float, N>{_mm_shuffle_ps(v.raw, v.raw, 0x55 * kLane)};
}
template <int kLane, size_t N>
HWY_API Vec128<double, N> Broadcast(const Vec128<double, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<double, N>{_mm_shuffle_pd(v.raw, v.raw, 3 * kLane)};
}

// ------------------------------ Shuffle bytes with variable indices

// Returns vector of bytes[from[i]]. "from" is also interpreted as bytes:
// either valid indices in [0, 16) or >= 0x80 to zero the i-th output byte.
template <typename T, typename TI>
HWY_API Vec128<T> TableLookupBytes(const Vec128<T> bytes,
                                   const Vec128<TI> from) {
  return Vec128<T>{_mm_shuffle_epi8(bytes.raw, from.raw)};
}

// ------------------------------ Hard-coded shuffles

// Notation: let Vec128<int32_t> have lanes 3,2,1,0 (0 is least-significant).
// Shuffle0321 rotates one lane to the right (the previous least-significant
// lane is now most-significant). These could also be implemented via
// CombineShiftRightBytes but the shuffle_abcd notation is more convenient.

// Swap 32-bit halves in 64-bit halves.
HWY_API Vec128<uint32_t> Shuffle2301(const Vec128<uint32_t> v) {
  return Vec128<uint32_t>{_mm_shuffle_epi32(v.raw, 0xB1)};
}
HWY_API Vec128<int32_t> Shuffle2301(const Vec128<int32_t> v) {
  return Vec128<int32_t>{_mm_shuffle_epi32(v.raw, 0xB1)};
}
HWY_API Vec128<float> Shuffle2301(const Vec128<float> v) {
  return Vec128<float>{_mm_shuffle_ps(v.raw, v.raw, 0xB1)};
}

// Swap 64-bit halves
HWY_API Vec128<uint32_t> Shuffle1032(const Vec128<uint32_t> v) {
  return Vec128<uint32_t>{_mm_shuffle_epi32(v.raw, 0x4E)};
}
HWY_API Vec128<int32_t> Shuffle1032(const Vec128<int32_t> v) {
  return Vec128<int32_t>{_mm_shuffle_epi32(v.raw, 0x4E)};
}
HWY_API Vec128<float> Shuffle1032(const Vec128<float> v) {
  return Vec128<float>{_mm_shuffle_ps(v.raw, v.raw, 0x4E)};
}
HWY_API Vec128<uint64_t> Shuffle01(const Vec128<uint64_t> v) {
  return Vec128<uint64_t>{_mm_shuffle_epi32(v.raw, 0x4E)};
}
HWY_API Vec128<int64_t> Shuffle01(const Vec128<int64_t> v) {
  return Vec128<int64_t>{_mm_shuffle_epi32(v.raw, 0x4E)};
}
HWY_API Vec128<double> Shuffle01(const Vec128<double> v) {
  return Vec128<double>{_mm_shuffle_pd(v.raw, v.raw, 1)};
}

// Rotate right 32 bits
HWY_API Vec128<uint32_t> Shuffle0321(const Vec128<uint32_t> v) {
  return Vec128<uint32_t>{_mm_shuffle_epi32(v.raw, 0x39)};
}
HWY_API Vec128<int32_t> Shuffle0321(const Vec128<int32_t> v) {
  return Vec128<int32_t>{_mm_shuffle_epi32(v.raw, 0x39)};
}
HWY_API Vec128<float> Shuffle0321(const Vec128<float> v) {
  return Vec128<float>{_mm_shuffle_ps(v.raw, v.raw, 0x39)};
}
// Rotate left 32 bits
HWY_API Vec128<uint32_t> Shuffle2103(const Vec128<uint32_t> v) {
  return Vec128<uint32_t>{_mm_shuffle_epi32(v.raw, 0x93)};
}
HWY_API Vec128<int32_t> Shuffle2103(const Vec128<int32_t> v) {
  return Vec128<int32_t>{_mm_shuffle_epi32(v.raw, 0x93)};
}
HWY_API Vec128<float> Shuffle2103(const Vec128<float> v) {
  return Vec128<float>{_mm_shuffle_ps(v.raw, v.raw, 0x93)};
}

// Reverse
HWY_API Vec128<uint32_t> Shuffle0123(const Vec128<uint32_t> v) {
  return Vec128<uint32_t>{_mm_shuffle_epi32(v.raw, 0x1B)};
}
HWY_API Vec128<int32_t> Shuffle0123(const Vec128<int32_t> v) {
  return Vec128<int32_t>{_mm_shuffle_epi32(v.raw, 0x1B)};
}
HWY_API Vec128<float> Shuffle0123(const Vec128<float> v) {
  return Vec128<float>{_mm_shuffle_ps(v.raw, v.raw, 0x1B)};
}

// ------------------------------ Permute (runtime variable)

// Returned by SetTableIndices for use by TableLookupLanes.
template <typename T>
struct permute_sse4 {
  __m128i raw;
};

template <typename T>
HWY_API permute_sse4<T> SetTableIndices(Full128<T>, const int32_t* idx) {
#if !defined(NDEBUG) || defined(ADDRESS_SANITIZER)
  const size_t N = 16 / sizeof(T);
  for (size_t i = 0; i < N; ++i) {
    if (idx[i] >= static_cast<int32_t>(N)) {
      printf("SetTableIndices [%zu] = %d >= %zu\n", i, idx[i], N);
    }
  }
#endif

  const Full128<uint8_t> d8;
  alignas(16) uint8_t control[16];
  for (size_t idx_byte = 0; idx_byte < 16; ++idx_byte) {
    const size_t idx_lane = idx_byte / sizeof(T);
    const size_t mod = idx_byte % sizeof(T);
    control[idx_byte] = idx[idx_lane] * sizeof(T) + mod;
  }
  return permute_sse4<T>{Load(d8, control).raw};
}

HWY_API Vec128<uint32_t> TableLookupLanes(const Vec128<uint32_t> v,
                                          const permute_sse4<uint32_t> idx) {
  return TableLookupBytes(v, Vec128<uint8_t>{idx.raw});
}
HWY_API Vec128<int32_t> TableLookupLanes(const Vec128<int32_t> v,
                                         const permute_sse4<int32_t> idx) {
  return TableLookupBytes(v, Vec128<uint8_t>{idx.raw});
}
HWY_API Vec128<float> TableLookupLanes(const Vec128<float> v,
                                       const permute_sse4<float> idx) {
  const Full128<int32_t> di;
  const Full128<float> df;
  return BitCast(df,
                 TableLookupBytes(BitCast(di, v), Vec128<uint8_t>{idx.raw}));
}

// ------------------------------ Interleave lanes

// Interleaves lanes from halves of the 128-bit blocks of "a" (which provides
// the least-significant lane) and "b". To concatenate two half-width integers
// into one, use ZipLower/Upper instead (also works with scalar).

HWY_API Vec128<uint8_t> InterleaveLower(const Vec128<uint8_t> a,
                                        const Vec128<uint8_t> b) {
  return Vec128<uint8_t>{_mm_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec128<uint16_t> InterleaveLower(const Vec128<uint16_t> a,
                                         const Vec128<uint16_t> b) {
  return Vec128<uint16_t>{_mm_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec128<uint32_t> InterleaveLower(const Vec128<uint32_t> a,
                                         const Vec128<uint32_t> b) {
  return Vec128<uint32_t>{_mm_unpacklo_epi32(a.raw, b.raw)};
}
HWY_API Vec128<uint64_t> InterleaveLower(const Vec128<uint64_t> a,
                                         const Vec128<uint64_t> b) {
  return Vec128<uint64_t>{_mm_unpacklo_epi64(a.raw, b.raw)};
}

HWY_API Vec128<int8_t> InterleaveLower(const Vec128<int8_t> a,
                                       const Vec128<int8_t> b) {
  return Vec128<int8_t>{_mm_unpacklo_epi8(a.raw, b.raw)};
}
HWY_API Vec128<int16_t> InterleaveLower(const Vec128<int16_t> a,
                                        const Vec128<int16_t> b) {
  return Vec128<int16_t>{_mm_unpacklo_epi16(a.raw, b.raw)};
}
HWY_API Vec128<int32_t> InterleaveLower(const Vec128<int32_t> a,
                                        const Vec128<int32_t> b) {
  return Vec128<int32_t>{_mm_unpacklo_epi32(a.raw, b.raw)};
}
HWY_API Vec128<int64_t> InterleaveLower(const Vec128<int64_t> a,
                                        const Vec128<int64_t> b) {
  return Vec128<int64_t>{_mm_unpacklo_epi64(a.raw, b.raw)};
}

HWY_API Vec128<float> InterleaveLower(const Vec128<float> a,
                                      const Vec128<float> b) {
  return Vec128<float>{_mm_unpacklo_ps(a.raw, b.raw)};
}
HWY_API Vec128<double> InterleaveLower(const Vec128<double> a,
                                       const Vec128<double> b) {
  return Vec128<double>{_mm_unpacklo_pd(a.raw, b.raw)};
}

HWY_API Vec128<uint8_t> InterleaveUpper(const Vec128<uint8_t> a,
                                        const Vec128<uint8_t> b) {
  return Vec128<uint8_t>{_mm_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec128<uint16_t> InterleaveUpper(const Vec128<uint16_t> a,
                                         const Vec128<uint16_t> b) {
  return Vec128<uint16_t>{_mm_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec128<uint32_t> InterleaveUpper(const Vec128<uint32_t> a,
                                         const Vec128<uint32_t> b) {
  return Vec128<uint32_t>{_mm_unpackhi_epi32(a.raw, b.raw)};
}
HWY_API Vec128<uint64_t> InterleaveUpper(const Vec128<uint64_t> a,
                                         const Vec128<uint64_t> b) {
  return Vec128<uint64_t>{_mm_unpackhi_epi64(a.raw, b.raw)};
}

HWY_API Vec128<int8_t> InterleaveUpper(const Vec128<int8_t> a,
                                       const Vec128<int8_t> b) {
  return Vec128<int8_t>{_mm_unpackhi_epi8(a.raw, b.raw)};
}
HWY_API Vec128<int16_t> InterleaveUpper(const Vec128<int16_t> a,
                                        const Vec128<int16_t> b) {
  return Vec128<int16_t>{_mm_unpackhi_epi16(a.raw, b.raw)};
}
HWY_API Vec128<int32_t> InterleaveUpper(const Vec128<int32_t> a,
                                        const Vec128<int32_t> b) {
  return Vec128<int32_t>{_mm_unpackhi_epi32(a.raw, b.raw)};
}
HWY_API Vec128<int64_t> InterleaveUpper(const Vec128<int64_t> a,
                                        const Vec128<int64_t> b) {
  return Vec128<int64_t>{_mm_unpackhi_epi64(a.raw, b.raw)};
}

HWY_API Vec128<float> InterleaveUpper(const Vec128<float> a,
                                      const Vec128<float> b) {
  return Vec128<float>{_mm_unpackhi_ps(a.raw, b.raw)};
}
HWY_API Vec128<double> InterleaveUpper(const Vec128<double> a,
                                       const Vec128<double> b) {
  return Vec128<double>{_mm_unpackhi_pd(a.raw, b.raw)};
}

// ------------------------------ Zip lanes

// Same as interleave_*, except that the return lanes are double-width integers;
// this is necessary because the single-lane scalar cannot return two values.

template <size_t N>
HWY_API Vec128<uint16_t, (N + 1) / 2> ZipLower(const Vec128<uint8_t, N> a,
                                               const Vec128<uint8_t, N> b) {
  return Vec128<uint16_t, (N + 1) / 2>{_mm_unpacklo_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, (N + 1) / 2> ZipLower(const Vec128<uint16_t, N> a,
                                               const Vec128<uint16_t, N> b) {
  return Vec128<uint32_t, (N + 1) / 2>{_mm_unpacklo_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, (N + 1) / 2> ZipLower(const Vec128<uint32_t, N> a,
                                               const Vec128<uint32_t, N> b) {
  return Vec128<uint64_t, (N + 1) / 2>{_mm_unpacklo_epi32(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<int16_t, (N + 1) / 2> ZipLower(const Vec128<int8_t, N> a,
                                              const Vec128<int8_t, N> b) {
  return Vec128<int16_t, (N + 1) / 2>{_mm_unpacklo_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, (N + 1) / 2> ZipLower(const Vec128<int16_t, N> a,
                                              const Vec128<int16_t, N> b) {
  return Vec128<int32_t, (N + 1) / 2>{_mm_unpacklo_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, (N + 1) / 2> ZipLower(const Vec128<int32_t, N> a,
                                              const Vec128<int32_t, N> b) {
  return Vec128<int64_t, (N + 1) / 2>{_mm_unpacklo_epi32(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<uint16_t, N / 2> ZipUpper(const Vec128<uint8_t, N> a,
                                         const Vec128<uint8_t, N> b) {
  return Vec128<uint16_t, N / 2>{_mm_unpackhi_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N / 2> ZipUpper(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint32_t, N / 2>{_mm_unpackhi_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N / 2> ZipUpper(const Vec128<uint32_t, N> a,
                                         const Vec128<uint32_t, N> b) {
  return Vec128<uint64_t, N / 2>{_mm_unpackhi_epi32(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<int16_t, N / 2> ZipUpper(const Vec128<int8_t, N> a,
                                        const Vec128<int8_t, N> b) {
  return Vec128<int16_t, N / 2>{_mm_unpackhi_epi8(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N / 2> ZipUpper(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int32_t, N / 2>{_mm_unpackhi_epi16(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, N / 2> ZipUpper(const Vec128<int32_t, N> a,
                                        const Vec128<int32_t, N> b) {
  return Vec128<int64_t, N / 2>{_mm_unpackhi_epi32(a.raw, b.raw)};
}

// ------------------------------ Blocks

// hiH,hiL loH,loL |-> hiL,loL (= lower halves)
template <typename T>
HWY_API Vec128<T> ConcatLowerLower(const Vec128<T> hi, const Vec128<T> lo) {
  const Full128<uint64_t> d64;
  return BitCast(Full128<T>(),
                 InterleaveLower(BitCast(d64, lo), BitCast(d64, hi)));
}

// hiH,hiL loH,loL |-> hiH,loH (= upper halves)
template <typename T>
HWY_API Vec128<T> ConcatUpperUpper(const Vec128<T> hi, const Vec128<T> lo) {
  const Full128<uint64_t> d64;
  return BitCast(Full128<T>(),
                 InterleaveUpper(BitCast(d64, lo), BitCast(d64, hi)));
}

// hiH,hiL loH,loL |-> hiL,loH (= inner halves)
template <typename T>
HWY_API Vec128<T> ConcatLowerUpper(const Vec128<T> hi, const Vec128<T> lo) {
  return CombineShiftRightBytes<8>(hi, lo);
}

// hiH,hiL loH,loL |-> hiH,loL (= outer halves)
template <typename T>
HWY_API Vec128<T> ConcatUpperLower(const Vec128<T> hi, const Vec128<T> lo) {
  return Vec128<T>{_mm_blend_epi16(hi.raw, lo.raw, 0x0F)};
}
template <>
HWY_API Vec128<float> ConcatUpperLower(const Vec128<float> hi,
                                       const Vec128<float> lo) {
  return Vec128<float>{_mm_blend_ps(hi.raw, lo.raw, 3)};
}
template <>
HWY_API Vec128<double> ConcatUpperLower(const Vec128<double> hi,
                                        const Vec128<double> lo) {
  return Vec128<double>{_mm_blend_pd(hi.raw, lo.raw, 1)};
}

// ------------------------------ Odd/even lanes

namespace detail {

template <typename T>
HWY_API Vec128<T> OddEven(hwy::SizeTag<1> /* tag */, const Vec128<T> a,
                          const Vec128<T> b) {
  const Full128<T> d;
  const Full128<uint8_t> d8;
  alignas(16) constexpr uint8_t mask[16] = {0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0,
                                            0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0};
  return IfThenElse(MaskFromVec(BitCast(d, Load(d8, mask))), b, a);
}
template <typename T>
HWY_API Vec128<T> OddEven(hwy::SizeTag<2> /* tag */, const Vec128<T> a,
                          const Vec128<T> b) {
  return Vec128<T>{_mm_blend_epi16(a.raw, b.raw, 0x55)};
}
template <typename T>
HWY_API Vec128<T> OddEven(hwy::SizeTag<4> /* tag */, const Vec128<T> a,
                          const Vec128<T> b) {
  return Vec128<T>{_mm_blend_epi16(a.raw, b.raw, 0x33)};
}
template <typename T>
HWY_API Vec128<T> OddEven(hwy::SizeTag<8> /* tag */, const Vec128<T> a,
                          const Vec128<T> b) {
  return Vec128<T>{_mm_blend_epi16(a.raw, b.raw, 0x0F)};
}

}  // namespace detail

template <typename T>
HWY_API Vec128<T> OddEven(const Vec128<T> a, const Vec128<T> b) {
  return detail::OddEven(hwy::SizeTag<sizeof(T)>(), a, b);
}
template <>
HWY_API Vec128<float> OddEven<float>(const Vec128<float> a,
                                     const Vec128<float> b) {
  return Vec128<float>{_mm_blend_ps(a.raw, b.raw, 5)};
}

template <>
HWY_API Vec128<double> OddEven<double>(const Vec128<double> a,
                                       const Vec128<double> b) {
  return Vec128<double>{_mm_blend_pd(a.raw, b.raw, 1)};
}

// ================================================== CONVERT

// ------------------------------ Promotions (part w/ narrow lanes -> full)

// Unsigned: zero-extend.
template <size_t N>
HWY_API Vec128<uint16_t, N> PromoteTo(Simd<uint16_t, N> /* tag */,
                                      const Vec128<uint8_t, N> v) {
  return Vec128<uint16_t, N>{_mm_cvtepu8_epi16(v.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> PromoteTo(Simd<uint32_t, N> /* tag */,
                                      const Vec128<uint8_t, N> v) {
  return Vec128<uint32_t, N>{_mm_cvtepu8_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> PromoteTo(Simd<int16_t, N> /* tag */,
                                     const Vec128<uint8_t, N> v) {
  return Vec128<int16_t, N>{_mm_cvtepu8_epi16(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<uint8_t, N> v) {
  return Vec128<int32_t, N>{_mm_cvtepu8_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> PromoteTo(Simd<uint32_t, N> /* tag */,
                                      const Vec128<uint16_t, N> v) {
  return Vec128<uint32_t, N>{_mm_cvtepu16_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<uint16_t, N> v) {
  return Vec128<int32_t, N>{_mm_cvtepu16_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<uint64_t, N> PromoteTo(Simd<uint64_t, N> /* tag */,
                                      const Vec128<uint32_t, N> v) {
  return Vec128<uint64_t, N>{_mm_cvtepu32_epi64(v.raw)};
}

// Signed: replicate sign bit.
template <size_t N>
HWY_API Vec128<int16_t, N> PromoteTo(Simd<int16_t, N> /* tag */,
                                     const Vec128<int8_t, N> v) {
  return Vec128<int16_t, N>{_mm_cvtepi8_epi16(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<int8_t, N> v) {
  return Vec128<int32_t, N>{_mm_cvtepi8_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<int16_t, N> v) {
  return Vec128<int32_t, N>{_mm_cvtepi16_epi32(v.raw)};
}
template <size_t N>
HWY_API Vec128<int64_t, N> PromoteTo(Simd<int64_t, N> /* tag */,
                                     const Vec128<int32_t, N> v) {
  return Vec128<int64_t, N>{_mm_cvtepi32_epi64(v.raw)};
}

template <size_t N>
HWY_API Vec128<double, N> PromoteTo(Simd<double, N> /* tag */,
                                    const Vec128<float, N> v) {
  return Vec128<double, N>{_mm_cvtps_pd(v.raw)};
}

HWY_API Vec128<uint32_t> U32FromU8(const Vec128<uint8_t> v) {
  return Vec128<uint32_t>{_mm_cvtepu8_epi32(v.raw)};
}

// ------------------------------ Demotions (full -> part w/ narrow lanes)

template <size_t N>
HWY_API Vec128<uint16_t, N> DemoteTo(Simd<uint16_t, N> /* tag */,
                                     const Vec128<int32_t, N> v) {
  return Vec128<uint16_t, N>{_mm_packus_epi32(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<int16_t, N> DemoteTo(Simd<int16_t, N> /* tag */,
                                    const Vec128<int32_t, N> v) {
  return Vec128<int16_t, N>{_mm_packs_epi32(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<uint8_t, N> DemoteTo(Simd<uint8_t, N> /* tag */,
                                    const Vec128<int32_t, N> v) {
  const __m128i u16 = _mm_packus_epi32(v.raw, v.raw);
  return Vec128<uint8_t, N>{_mm_packus_epi16(u16, u16)};
}

template <size_t N>
HWY_API Vec128<uint8_t, N> DemoteTo(Simd<uint8_t, N> /* tag */,
                                    const Vec128<int16_t, N> v) {
  return Vec128<uint8_t, N>{_mm_packus_epi16(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<int8_t, N> DemoteTo(Simd<int8_t, N> /* tag */,
                                   const Vec128<int32_t, N> v) {
  const __m128i i16 = _mm_packs_epi32(v.raw, v.raw);
  return Vec128<int8_t, N>{_mm_packs_epi16(i16, i16)};
}

template <size_t N>
HWY_API Vec128<int8_t, N> DemoteTo(Simd<int8_t, N> /* tag */,
                                   const Vec128<int16_t, N> v) {
  return Vec128<int8_t, N>{_mm_packs_epi16(v.raw, v.raw)};
}

template <size_t N>
HWY_INLINE Vec128<float, N> DemoteTo(Simd<float, N> /* tag */,
                                     const Vec128<double, N> v) {
  return Vec128<float, N>{_mm_cvtpd_ps(v.raw)};
}

// For already range-limited input [0, 255].
HWY_API Vec128<uint8_t, 4> U8FromU32(const Vec128<uint32_t> v) {
  const Full128<uint32_t> d32;
  const Full128<uint8_t> d8;
  alignas(16) static constexpr uint32_t k8From32[4] = {
      0x0C080400u, 0x0C080400u, 0x0C080400u, 0x0C080400u};
  // Also replicate bytes into all 32 bit lanes for safety.
  const auto quad = TableLookupBytes(v, Load(d32, k8From32));
  return LowerHalf(LowerHalf(BitCast(d8, quad)));
}

// ------------------------------ Convert integer <=> floating point

template <size_t N>
HWY_API Vec128<float, N> ConvertTo(Simd<float, N> /* tag */,
                                   const Vec128<int32_t, N> v) {
  return Vec128<float, N>{_mm_cvtepi32_ps(v.raw)};
}

template <size_t N>
HWY_API Vec128<double, N> ConvertTo(Simd<double, N> dd,
                                    const Vec128<int64_t, N> v) {
#if HWY_TARGET == HWY_AVX3
  (void)dd;
  return Vec128<double, N>{_mm_cvtepi64_pd(v.raw)};
#else
  alignas(16) int64_t lanes_i[2];
  Store(v, Simd<int64_t, N>(), lanes_i);
  alignas(16) double lanes_d[2];
  for (size_t i = 0; i < N; ++i) {
    lanes_d[i] = static_cast<double>(lanes_i[i]);
  }
  return Load(dd, lanes_d);
#endif
}

// Truncates (rounds toward zero).
template <size_t N>
HWY_API Vec128<int32_t, N> ConvertTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<float, N> v) {
  return Vec128<int32_t, N>{_mm_cvttps_epi32(v.raw)};
}

template <size_t N>
HWY_API Vec128<int64_t, N> ConvertTo(Simd<int64_t, N> di,
                                     const Vec128<double, N> v) {
#if HWY_TARGET == HWY_AVX3
  (void)di;
  return Vec128<int64_t, N>{_mm_cvttpd_epi64(v.raw)};
#else
  alignas(16) double lanes_d[2];
  Store(v, Simd<double, N>(), lanes_d);
  alignas(16) int64_t lanes_i[2];
  for (size_t i = 0; i < N; ++i) {
    lanes_i[i] = static_cast<int64_t>(lanes_d[i]);
  }
  return Load(di, lanes_i);
#endif
}

template <size_t N>
HWY_API Vec128<int32_t, N> NearestInt(const Vec128<float, N> v) {
  return Vec128<int32_t, N>{_mm_cvtps_epi32(v.raw)};
}

// ================================================== MISC

// ------------------------------ Mask

namespace detail {

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<1> /*tag*/, const Mask128<T> mask) {
  const Full128<uint8_t> d;
  const auto sign_bits = BitCast(d, VecFromMask(mask)).raw;
  return static_cast<unsigned>(_mm_movemask_epi8(sign_bits));
}

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<2> /*tag*/, const Mask128<T> mask) {
  // Remove useless lower half of each u16 while preserving the sign bit.
  const auto sign_bits = _mm_packs_epi16(mask.raw, _mm_setzero_si128());
  return static_cast<unsigned>(_mm_movemask_epi8(sign_bits));
}

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<4> /*tag*/, const Mask128<T> mask) {
  const Full128<float> d;
  const auto sign_bits = BitCast(d, VecFromMask(mask)).raw;
  return static_cast<unsigned>(_mm_movemask_ps(sign_bits));
}

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<8> /*tag*/, const Mask128<T> mask) {
  const Full128<double> d;
  const auto sign_bits = BitCast(d, VecFromMask(mask)).raw;
  return static_cast<unsigned>(_mm_movemask_pd(sign_bits));
}

}  // namespace detail

template <typename T>
HWY_API uint64_t BitsFromMask(const Mask128<T> mask) {
  return detail::BitsFromMask(hwy::SizeTag<sizeof(T)>(), mask);
}

template <typename T>
HWY_API bool AllFalse(const Mask128<T> mask) {
  // Cheaper than PTEST, which is 2 uop / 3L.
  return BitsFromMask(mask) == 0;
}

template <typename T>
HWY_API bool AllTrue(const Mask128<T> mask) {
  constexpr uint64_t kAllBits = (1ull << (16 / sizeof(T))) - 1;
  return BitsFromMask(mask) == kAllBits;
}

template <typename T>
HWY_API size_t CountTrue(const Mask128<T> mask) {
  return hwy::PopCount(BitsFromMask(mask));
}

// ------------------------------ Horizontal sum (reduction)

// Returns 64-bit sums of 8-byte groups.
template <size_t N>
HWY_API Vec128<uint64_t, N / 8> SumsOfU8x8(const Vec128<uint8_t, N> v) {
  return Vec128<uint64_t, N / 8>{_mm_sad_epu8(v.raw, _mm_setzero_si128())};
}

namespace detail {

// For u32/i32/f32.
template <typename T, size_t N>
HWY_API Vec128<T, N> SumOfLanes(hwy::SizeTag<4> /* tag */,
                                const Vec128<T, N> v3210) {
  const Vec128<T> v1032 = Shuffle1032(v3210);
  const Vec128<T> v31_20_31_20 = v3210 + v1032;
  const Vec128<T> v20_31_20_31 = Shuffle0321(v31_20_31_20);
  return v20_31_20_31 + v31_20_31_20;
}

// For u64/i64/f64.
template <typename T, size_t N>
HWY_API Vec128<T, N> SumOfLanes(hwy::SizeTag<8> /* tag */,
                                const Vec128<T, N> v10) {
  const Vec128<T> v01 = Shuffle01(v10);
  return v10 + v01;
}

}  // namespace detail

// Supported for u/i/f 32/64. Returns the sum in each lane.
template <typename T, size_t N>
HWY_API Vec128<T, N> SumOfLanes(const Vec128<T, N> v) {
  return detail::SumOfLanes(hwy::SizeTag<sizeof(T)>(), v);
}
