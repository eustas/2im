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

// 128-bit WASM vectors and operations. External include guard.

// This header is included by begin_target-inl.h, possibly inside a namespace,
// so do not include system headers (already done by highway.h). HWY_ALIGN is
// already defined unless an IDE is only parsing this file, in which case we
// include headers to avoid warnings.
#ifndef HWY_ALIGN
#include <stddef.h>
#include <stdint.h>

#include "hwy/highway.h"
#endif  // HWY_ALIGN

template <typename T>
struct Raw128 {
  using type = __v128_u;
};
template <>
struct Raw128<float> {
  using type = __f32x4;
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

HWY_API __v128_u BitCastToInteger(__v128_u v) { return v; }
HWY_API __v128_u BitCastToInteger(__f32x4 v) {
  return static_cast<__v128_u>(v);
}
HWY_API __v128_u BitCastToInteger(__f64x2 v) {
  return static_cast<__v128_u>(v);
}

template <typename T, size_t N>
HWY_API Vec128<uint8_t, N * sizeof(T)> cast_to_u8(Vec128<T, N> v) {
  return Vec128<uint8_t, N * sizeof(T)>{BitCastToInteger(v.raw)};
}

// Cannot rely on function overloading because return types differ.
template <typename T>
struct BitCastFromInteger128 {
  HWY_API __v128_u operator()(__v128_u v) { return v; }
};
template <>
struct BitCastFromInteger128<float> {
  HWY_API __f32x4 operator()(__v128_u v) { return static_cast<__f32x4>(v); }
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
  return Vec128<T, N>{wasm_i32x4_splat(0)};
}
template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Zero(Simd<float, N> /* tag */) {
  return Vec128<float, N>{wasm_f32x4_splat(0.0f)};
}

// Returns a vector/part with all lanes set to "t".
template <size_t N, HWY_IF_LE128(uint8_t, N)>
HWY_API Vec128<uint8_t, N> Set(Simd<uint8_t, N> /* tag */, const uint8_t t) {
  return Vec128<uint8_t, N>{wasm_i8x16_splat(t)};
}
template <size_t N, HWY_IF_LE128(uint16_t, N)>
HWY_API Vec128<uint16_t, N> Set(Simd<uint16_t, N> /* tag */, const uint16_t t) {
  return Vec128<uint16_t, N>{wasm_i16x8_splat(t)};
}
template <size_t N, HWY_IF_LE128(uint32_t, N)>
HWY_API Vec128<uint32_t, N> Set(Simd<uint32_t, N> /* tag */, const uint32_t t) {
  return Vec128<uint32_t, N>{wasm_i32x4_splat(t)};
}

template <size_t N, HWY_IF_LE128(int8_t, N)>
HWY_API Vec128<int8_t, N> Set(Simd<int8_t, N> /* tag */, const int8_t t) {
  return Vec128<int8_t, N>{wasm_i8x16_splat(t)};
}
template <size_t N, HWY_IF_LE128(int16_t, N)>
HWY_API Vec128<int16_t, N> Set(Simd<int16_t, N> /* tag */, const int16_t t) {
  return Vec128<int16_t, N>{wasm_i16x8_splat(t)};
}
template <size_t N, HWY_IF_LE128(int32_t, N)>
HWY_API Vec128<int32_t, N> Set(Simd<int32_t, N> /* tag */, const int32_t t) {
  return Vec128<int32_t, N>{wasm_i32x4_splat(t)};
}

template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Set(Simd<float, N> /* tag */, const float t) {
  return Vec128<float, N>{wasm_f32x4_splat(t)};
}

HWY_DIAGNOSTICS(push)
HWY_DIAGNOSTICS_OFF(disable : 4700, ignored "-Wuninitialized")

// Returns a vector with uninitialized elements.
template <typename T, size_t N, HWY_IF_LE128(T, N)>
HWY_API Vec128<T, N> Undefined(Simd<T, N> /* tag */) {
  __v128_u raw;
  return Vec128<T, N>{raw};
}
template <size_t N, HWY_IF_LE128(float, N)>
HWY_API Vec128<float, N> Undefined(Simd<float, N> /* tag */) {
  __f32x4 raw;
  return Vec128<float, N>{raw};
}

HWY_DIAGNOSTICS(pop)

// ================================================== ARITHMETIC

// ------------------------------ Addition

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> operator+(const Vec128<uint8_t, N> a,
                                     const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_i8x16_add(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> operator+(const Vec128<uint16_t, N> a,
                                      const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_i16x8_add(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator+(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{wasm_i32x4_add(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> operator+(const Vec128<int8_t, N> a,
                                    const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_add(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> operator+(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_add(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator+(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{wasm_i32x4_add(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> operator+(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_add(a.raw, b.raw)};
}

// ------------------------------ Subtraction

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> operator-(const Vec128<uint8_t, N> a,
                                     const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_i8x16_sub(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> operator-(Vec128<uint16_t, N> a,
                                      Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_i16x8_sub(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator-(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{wasm_i32x4_sub(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> operator-(const Vec128<int8_t, N> a,
                                    const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_sub(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> operator-(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_sub(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator-(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{wasm_i32x4_sub(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> operator-(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_sub(a.raw, b.raw)};
}

// ------------------------------ Saturating addition

// Returns a + b clamped to the destination range.

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> SaturatedAdd(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_u8x16_add_saturate(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> SaturatedAdd(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_u16x8_add_saturate(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> SaturatedAdd(const Vec128<int8_t, N> a,
                                       const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_add_saturate(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> SaturatedAdd(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_add_saturate(a.raw, b.raw)};
}

// ------------------------------ Saturating subtraction

// Returns a - b clamped to the destination range.

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> SaturatedSub(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_u8x16_sub_saturate(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> SaturatedSub(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_u16x8_sub_saturate(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> SaturatedSub(const Vec128<int8_t, N> a,
                                       const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_sub_saturate(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> SaturatedSub(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_sub_saturate(a.raw, b.raw)};
}

// ------------------------------ Average

// Returns (a + b + 1) / 2

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> AverageRound(const Vec128<uint8_t, N> a,
                                        const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_u8x16_avgr(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> AverageRound(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_u16x8_avgr(a.raw, b.raw)};
}

// ------------------------------ Absolute value

// Returns absolute value, except that LimitsMin() maps to LimitsMax() + 1.
template <size_t N>
HWY_API Vec128<int8_t, N> Abs(const Vec128<int8_t, N> v) {
  return Vec128<int8_t, N>{wasm_i8x16_abs(v.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Abs(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_abs(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Abs(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_abs(v.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> Abs(const Vec128<float, N> v) {
  return Vec128<float, N>{wasm_f32x4_abs(v.raw)};
}

// ------------------------------ Shift lanes by constant #bits

// Unsigned
template <int kBits, size_t N>
HWY_API Vec128<uint16_t, N> ShiftLeft(const Vec128<uint16_t, N> v) {
  return Vec128<uint16_t, N>{wasm_i16x8_shl(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint16_t, N> ShiftRight(const Vec128<uint16_t, N> v) {
  return Vec128<uint16_t, N>{wasm_u16x8_shr(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint32_t, N> ShiftLeft(const Vec128<uint32_t, N> v) {
  return Vec128<uint32_t, N>{wasm_i32x4_shl(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<uint32_t, N> ShiftRight(const Vec128<uint32_t, N> v) {
  return Vec128<uint32_t, N>{wasm_u32x4_shr(v.raw, kBits)};
}

// Signed
template <int kBits, size_t N>
HWY_API Vec128<int16_t, N> ShiftLeft(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_shl(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int16_t, N> ShiftRight(const Vec128<int16_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_shr(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int32_t, N> ShiftLeft(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_shl(v.raw, kBits)};
}
template <int kBits, size_t N>
HWY_API Vec128<int32_t, N> ShiftRight(const Vec128<int32_t, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_shr(v.raw, kBits)};
}

// ------------------------------ Shift lanes by same variable #bits

// Unsigned (no u8)
template <size_t N>
HWY_API Vec128<uint16_t, N> ShiftLeftSame(const Vec128<uint16_t, N> v,
                                          const int bits) {
  return Vec128<uint16_t, N>{wasm_i16x8_shl(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> ShiftRightSame(const Vec128<uint16_t, N> v,
                                           const int bits) {
  return Vec128<uint16_t, N>{wasm_u16x8_shr(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> ShiftLeftSame(const Vec128<uint32_t, N> v,
                                          const int bits) {
  return Vec128<uint32_t, N>{wasm_i32x4_shl(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> ShiftRightSame(const Vec128<uint32_t, N> v,
                                           const int bits) {
  return Vec128<uint32_t, N>{wasm_u32x4_shr(v.raw, bits)};
}

// Signed (no i8)
template <size_t N>
HWY_API Vec128<int16_t, N> ShiftLeftSame(const Vec128<int16_t, N> v,
                                         const int bits) {
  return Vec128<int16_t, N>{wasm_i16x8_shl(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> ShiftRightSame(const Vec128<int16_t, N> v,
                                          const int bits) {
  return Vec128<int16_t, N>{wasm_i16x8_shr(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> ShiftLeftSame(const Vec128<int32_t, N> v,
                                         const int bits) {
  return Vec128<int32_t, N>{wasm_i32x4_shl(v.raw, bits)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> ShiftRightSame(const Vec128<int32_t, N> v,
                                          const int bits) {
  return Vec128<int32_t, N>{wasm_i32x4_shr(v.raw, bits)};
}

// ------------------------------ Shift lanes by independent variable #bits

// Unsupported.

// ------------------------------ Minimum

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> Min(const Vec128<uint8_t, N> a,
                               const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_u8x16_min(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> Min(const Vec128<uint16_t, N> a,
                                const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_u16x8_min(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> Min(const Vec128<uint32_t, N> a,
                                const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{wasm_u32x4_min(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> Min(const Vec128<int8_t, N> a,
                              const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_min(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Min(const Vec128<int16_t, N> a,
                               const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_min(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Min(const Vec128<int32_t, N> a,
                               const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{wasm_i32x4_min(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> Min(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_min(a.raw, b.raw)};
}

// ------------------------------ Maximum

// Unsigned
template <size_t N>
HWY_API Vec128<uint8_t, N> Max(const Vec128<uint8_t, N> a,
                               const Vec128<uint8_t, N> b) {
  return Vec128<uint8_t, N>{wasm_u8x16_max(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint16_t, N> Max(const Vec128<uint16_t, N> a,
                                const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_u16x8_max(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> Max(const Vec128<uint32_t, N> a,
                                const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{wasm_u32x4_max(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int8_t, N> Max(const Vec128<int8_t, N> a,
                              const Vec128<int8_t, N> b) {
  return Vec128<int8_t, N>{wasm_i8x16_max(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> Max(const Vec128<int16_t, N> a,
                               const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_max(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> Max(const Vec128<int32_t, N> a,
                               const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{wasm_i32x4_max(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Vec128<float, N> Max(const Vec128<float, N> a,
                             const Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_max(a.raw, b.raw)};
}

// ------------------------------ Integer multiplication

// Unsigned
template <size_t N>
HWY_API Vec128<uint16_t, N> operator*(const Vec128<uint16_t, N> a,
                                      const Vec128<uint16_t, N> b) {
  return Vec128<uint16_t, N>{wasm_i16x8_mul(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> operator*(const Vec128<uint32_t, N> a,
                                      const Vec128<uint32_t, N> b) {
  return Vec128<uint32_t, N>{wasm_i32x4_mul(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Vec128<int16_t, N> operator*(const Vec128<int16_t, N> a,
                                     const Vec128<int16_t, N> b) {
  return Vec128<int16_t, N>{wasm_i16x8_mul(a.raw, b.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> operator*(const Vec128<int32_t, N> a,
                                     const Vec128<int32_t, N> b) {
  return Vec128<int32_t, N>{wasm_i32x4_mul(a.raw, b.raw)};
}

// Returns the upper 16 bits of a * b in each lane.
template <size_t N>
HWY_API Vec128<uint16_t, N> MulHigh(const Vec128<uint16_t, N> a,
                                    const Vec128<uint16_t, N> b) {
  // TODO(eustas): replace, when implemented in WASM.
  alignas(16) uint16_t a_lanes[8];
  alignas(16) uint16_t b_lanes[8];
  alignas(16) uint16_t c_lanes[8];
  wasm_v128_store(a_lanes, a.raw);
  wasm_v128_store(b_lanes, b.raw);
  for (size_t i = 0; i < 8; ++i) {
    uint32_t ab = static_cast<uint32_t>(a_lanes[i]) * b_lanes[i];
    c_lanes[i] = static_cast<uint16_t>(ab >> 16);
  }
  return Vec128<uint16_t, N>{wasm_v128_load(c_lanes)};
}
template <size_t N>
HWY_API Vec128<int16_t, N> MulHigh(const Vec128<int16_t, N> a,
                                   const Vec128<int16_t, N> b) {
  // TODO(eustas): replace, when implemented in WASM.
  alignas(16) int16_t a_lanes[8];
  alignas(16) int16_t b_lanes[8];
  alignas(16) int16_t c_lanes[8];
  wasm_v128_store(a_lanes, a.raw);
  wasm_v128_store(b_lanes, b.raw);
  for (size_t i = 0; i < 8; ++i) {
    int32_t ab = static_cast<int32_t>(a_lanes[i]) * b_lanes[i];
    c_lanes[i] = static_cast<int16_t>(ab >> 16);
  }
  return Vec128<int16_t, N>{wasm_v128_load(c_lanes)};
}

// Multiplies even lanes (0, 2 ..) and places the double-wide result into
// even and the upper half into its odd neighbor lane.
template <size_t N>
HWY_API Vec128<int64_t, (N + 1) / 2> MulEven(const Vec128<int32_t, N> a,
                                             const Vec128<int32_t, N> b) {
  // TODO(eustas): replace, when implemented in WASM.
  alignas(16) int32_t a_lanes[4];
  alignas(16) int32_t b_lanes[4];
  alignas(16) int64_t c_lanes[2];
  wasm_v128_store(a_lanes, a.raw);
  wasm_v128_store(b_lanes, b.raw);
  for (size_t i = 0; i < 2; ++i) {
    c_lanes[i] = static_cast<int64_t>(a_lanes[2 * i]) * b_lanes[2 * i];
  }
  return Vec128<int64_t, (N + 1) / 2>{wasm_v128_load(c_lanes)};
}
template <size_t N>
HWY_API Vec128<uint64_t, (N + 1) / 2> MulEven(const Vec128<uint32_t, N> a,
                                              const Vec128<uint32_t, N> b) {
  // TODO(eustas): replace, when implemented in WASM.
  alignas(16) uint32_t a_lanes[4];
  alignas(16) uint32_t b_lanes[4];
  alignas(16) uint64_t c_lanes[2];
  wasm_v128_store(a_lanes, a.raw);
  wasm_v128_store(b_lanes, b.raw);
  for (size_t i = 0; i < 2; ++i) {
    c_lanes[i] = static_cast<uint64_t>(a_lanes[2 * i]) * b_lanes[2 * i];
  }
  return Vec128<uint64_t, (N + 1) / 2>{wasm_v128_load(c_lanes)};
}

// ------------------------------ Floating-point negate

template <size_t N>
HWY_API Vec128<float, N> Neg(const Vec128<float, N> v) {
  const Simd<float, N> df;
  const Simd<uint32_t, N> du;
  const auto sign = BitCast(df, Set(du, 0x80000000u));
  return v ^ sign;
}

// ------------------------------ Floating-point mul / div

template <size_t N>
HWY_API Vec128<float, N> operator*(Vec128<float, N> a, Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_mul(a.raw, b.raw)};
}

template <size_t N>
HWY_API Vec128<float, N> operator/(const Vec128<float, N> a,
                                   const Vec128<float, N> b) {
  return Vec128<float, N>{wasm_f32x4_div(a.raw, b.raw)};
}

// Approximate reciprocal
template <size_t N>
HWY_API Vec128<float, N> ApproximateReciprocal(const Vec128<float, N> v) {
  // TODO(eustas): replace, when implemented in WASM.
  const Vec128<float, N> one = Vec128<float, N>{wasm_f32x4_splat(1.0f)};
  return one / v;
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
  // TODO(eustas): replace, when implemented in WASM.
  // TODO(eustas): is it wasm_f32x4_qfma?
  return mul * x + add;
}

// Returns add - mul * x
template <size_t N>
HWY_API Vec128<float, N> NegMulAdd(const Vec128<float, N> mul,
                                   const Vec128<float, N> x,
                                   const Vec128<float, N> add) {
  // TODO(eustas): replace, when implemented in WASM.
  return add - mul * x;
}

// Returns mul * x - sub
template <size_t N>
HWY_API Vec128<float, N> MulSub(const Vec128<float, N> mul,
                                const Vec128<float, N> x,
                                const Vec128<float, N> sub) {
  // TODO(eustas): replace, when implemented in WASM.
  // TODO(eustas): is it wasm_f32x4_qfms?
  return mul * x - sub;
}

// Returns -mul * x - sub
template <size_t N>
HWY_API Vec128<float, N> NegMulSub(const Vec128<float, N> mul,
                                   const Vec128<float, N> x,
                                   const Vec128<float, N> sub) {
  // TODO(eustas): replace, when implemented in WASM.
  return Neg(mul) * x - sub;
}

// ------------------------------ Floating-point square root

// Full precision square root
template <size_t N>
HWY_API Vec128<float, N> Sqrt(const Vec128<float, N> v) {
  return Vec128<float, N>{wasm_f32x4_sqrt(v.raw)};
}

// Approximate reciprocal square root
template <size_t N>
HWY_API Vec128<float, N> ApproximateReciprocalSqrt(const Vec128<float, N> v) {
  // TODO(eustas): find cheaper a way to calculate this.
  const Vec128<float, N> one = Vec128<float, N>{wasm_f32x4_splat(1.0f)};
  return one / Sqrt(v);
}

// ------------------------------ Floating-point rounding

// Toward nearest integer, ties to even
template <size_t N>
HWY_API Vec128<float, N> Round(const Vec128<float, N> v) {
  // TODO(eustas): workaround does not work - wasm_i32x4_trunc_saturate_f32x4
  //               limits feasible input to +-2^31.
  // TODO(eustas): how to do "ties to even"?
  // TODO(eustas): 8 ops; isn't it cheaper to store/convert/load?
  // const __f32x4 c00 = wasm_f32x4_splat(0.0f);
  // const __f32x4 corr = wasm_f32x4_convert_i32x4(wasm_f32x4_le(v.raw, c00));
  // const __f32x4 c05 = wasm_f32x4_splat(0.5f);
  // +0.5 for non-negative lane, -0.5 for other.
  // const __f32x4 delta = wasm_f32x4_add(c05, corr);
  // Shift input by 0.5 away from 0.
  // const __f32x4 fixed = wasm_f32x4_add(v.raw, delta);
  // const __i32x4 result = wasm_i32x4_trunc_saturate_f32x4(fixed);
  // return Vec128<float, N>{wasm_f32x4_convert_i32x4(result)};
  alignas(16) float input[4];
  alignas(16) float output[4];
  wasm_v128_store(input, v.raw);
  for (size_t i = 0; i < 4; ++i) {
    output[i] = std::round(input[i]);
  }
  return Vec128<float, N>{wasm_v128_load(output)};
}

// Toward zero, aka truncate
template <size_t N>
HWY_API Vec128<float, N> Trunc(const Vec128<float, N> v) {
  // TODO(eustas): impossible
  // const __i32x4 result = wasm_i32x4_trunc_saturate_f32x4(v.raw);
  // return Vec128<float, N>{wasm_f32x4_convert_i32x4(result)};
  alignas(16) float input[4];
  alignas(16) float output[4];
  wasm_v128_store(input, v.raw);
  for (size_t i = 0; i < 4; ++i) {
    output[i] = std::trunc(input[i]);
  }
  return Vec128<float, N>{wasm_v128_load(output)};
}

// Toward +infinity, aka ceiling
template <size_t N>
HWY_API Vec128<float, N> Ceil(const Vec128<float, N> v) {
  // TODO(eustas): impossible
  // const __i32x4 truncated = wasm_i32x4_trunc_saturate_f32x4(v.raw);
  // const __f32x4 draft = wasm_f32x4_convert_i32x4(truncated);
  // -1 for positive with fractional part, 0 for negative and integer.
  // const __i32x4 delta = wasm_f32x4_gt(v.raw, draft);
  // const __f32x4 corr = wasm_f32x4_convert_i32x4(delta);
  // return Vec128<float, N>{wasm_f32x4_sub(draft, corr)};
  alignas(16) float input[4];
  alignas(16) float output[4];
  wasm_v128_store(input, v.raw);
  for (size_t i = 0; i < 4; ++i) {
    output[i] = std::ceil(input[i]);
  }
  return Vec128<float, N>{wasm_v128_load(output)};
}

// Toward -infinity, aka floor
template <size_t N>
HWY_API Vec128<float, N> Floor(const Vec128<float, N> v) {
  // TODO(eustas): impossible
  // const __i32x4 truncated = wasm_i32x4_trunc_saturate_f32x4(v.raw);
  // const __f32x4 draft = wasm_f32x4_convert_i32x4(truncated);
  // -1 for negative with fractional part, 0 for positive and integer.
  // const __i32x4 delta = wasm_f32x4_lt(v.raw, draft);
  // const __f32x4 corr = wasm_f32x4_convert_i32x4(delta);
  // return Vec128<float, N>{wasm_f32x4_add(draft, corr)};
  alignas(16) float input[4];
  alignas(16) float output[4];
  wasm_v128_store(input, v.raw);
  for (size_t i = 0; i < 4; ++i) {
    output[i] = std::floor(input[i]);
  }
  return Vec128<float, N>{wasm_v128_load(output)};
}

// ================================================== COMPARE

// Comparisons fill a lane with 1-bits if the condition is true, else 0.

// ------------------------------ Equality

// Unsigned
template <size_t N>
HWY_API Mask128<uint8_t, N> operator==(const Vec128<uint8_t, N> a,
                                       const Vec128<uint8_t, N> b) {
  return Mask128<uint8_t, N>{wasm_i8x16_eq(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<uint16_t, N> operator==(const Vec128<uint16_t, N> a,
                                        const Vec128<uint16_t, N> b) {
  return Mask128<uint16_t, N>{wasm_i16x8_eq(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<uint32_t, N> operator==(const Vec128<uint32_t, N> a,
                                        const Vec128<uint32_t, N> b) {
  return Mask128<uint32_t, N>{wasm_i32x4_eq(a.raw, b.raw)};
}

// Signed
template <size_t N>
HWY_API Mask128<int8_t, N> operator==(const Vec128<int8_t, N> a,
                                      const Vec128<int8_t, N> b) {
  return Mask128<int8_t, N>{wasm_i8x16_eq(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator==(Vec128<int16_t, N> a,
                                       Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{wasm_i16x8_eq(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator==(const Vec128<int32_t, N> a,
                                       const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{wasm_i32x4_eq(a.raw, b.raw)};
}

// Float
template <size_t N>
HWY_API Mask128<float, N> operator==(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{wasm_f32x4_eq(a.raw, b.raw)};
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
  return Mask128<int8_t, N>{wasm_i8x16_lt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator<(const Vec128<int16_t, N> a,
                                      const Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{wasm_i16x8_lt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator<(const Vec128<int32_t, N> a,
                                      const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{wasm_i32x4_lt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator<(const Vec128<float, N> a,
                                    const Vec128<float, N> b) {
  return Mask128<float, N>{wasm_f32x4_lt(a.raw, b.raw)};
}

// Signed/float >
template <size_t N>
HWY_API Mask128<int8_t, N> operator>(const Vec128<int8_t, N> a,
                                     const Vec128<int8_t, N> b) {
  return Mask128<int8_t, N>{wasm_i8x16_gt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int16_t, N> operator>(const Vec128<int16_t, N> a,
                                      const Vec128<int16_t, N> b) {
  return Mask128<int16_t, N>{wasm_i16x8_gt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<int32_t, N> operator>(const Vec128<int32_t, N> a,
                                      const Vec128<int32_t, N> b) {
  return Mask128<int32_t, N>{wasm_i32x4_gt(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator>(const Vec128<float, N> a,
                                    const Vec128<float, N> b) {
  return Mask128<float, N>{wasm_f32x4_gt(a.raw, b.raw)};
}

// ------------------------------ Weak inequality

// Float <= >=
template <size_t N>
HWY_API Mask128<float, N> operator<=(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{wasm_f32x4_le(a.raw, b.raw)};
}
template <size_t N>
HWY_API Mask128<float, N> operator>=(const Vec128<float, N> a,
                                     const Vec128<float, N> b) {
  return Mask128<float, N>{wasm_f32x4_ge(a.raw, b.raw)};
}

// ================================================== LOGICAL

// ------------------------------ Bitwise AND

template <typename T, size_t N>
HWY_API Vec128<T, N> And(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{wasm_v128_and(a.raw, b.raw)};
}

// ------------------------------ Bitwise AND-NOT

// Returns ~not_mask & mask.
template <typename T, size_t N>
HWY_API Vec128<T, N> AndNot(Vec128<T, N> not_mask, Vec128<T, N> mask) {
  return Vec128<T, N>{wasm_v128_and(wasm_v128_not(not_mask.raw), mask.raw)};
}

// ------------------------------ Bitwise OR

template <typename T, size_t N>
HWY_API Vec128<T, N> Or(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{wasm_v128_or(a.raw, b.raw)};
}

// ------------------------------ Bitwise XOR

template <typename T, size_t N>
HWY_API Vec128<T, N> Xor(Vec128<T, N> a, Vec128<T, N> b) {
  return Vec128<T, N>{wasm_v128_xor(a.raw, b.raw)};
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
  return Vec128<T, N>{wasm_v128_bitselect(yes.raw, no.raw, mask.raw)};
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
  const auto zero = Zero(d);
  return IfThenElse(Mask128<T, N>{(v > zero).raw}, v, zero);
}

// ================================================== MEMORY

// ------------------------------ Load

template <typename T>
HWY_API Vec128<T> Load(Full128<T> /* tag */, const T* HWY_RESTRICT aligned) {
  return Vec128<T>{wasm_v128_load(aligned)};
}

// Partial load.
template <typename T, size_t N, HWY_IF_LE64(T, N)>
HWY_API Vec128<T, N> Load(Simd<T, N> /* tag */, const T* HWY_RESTRICT p) {
  Vec128<T, N> v;
  hwy::CopyBytes<sizeof(T) * N>(p, &v);
  return v;
}

// LoadU == Load.
template <typename T, size_t N>
HWY_API Vec128<T, N> LoadU(Simd<T, N> d, const T* HWY_RESTRICT p) {
  return Load(d, p);
}

// 128-bit SIMD => nothing to duplicate, same as an unaligned load.
template <typename T, size_t N, HWY_IF_LE128(T, N)>
HWY_API Vec128<T, N> LoadDup128(Simd<T, N> d, const T* HWY_RESTRICT p) {
  return Load(d, p);
}

// ------------------------------ Store

template <typename T>
HWY_API void Store(Vec128<T> v, Full128<T> /* tag */, T* HWY_RESTRICT aligned) {
  wasm_v128_store(aligned, v.raw);
}

// Partial store.
template <typename T, size_t N, HWY_IF_LE64(T, N)>
HWY_API void Store(Vec128<T, N> v, Simd<T, N> /* tag */, T* HWY_RESTRICT p) {
  hwy::CopyBytes<sizeof(T) * N>(&v, p);
}

HWY_API void Store(const Vec128<float, 1> v, Simd<float, 1> /* tag */,
                   float* HWY_RESTRICT p) {
  *p = wasm_f32x4_extract_lane(v.raw, 0);
}

// StoreU == Store.
template <typename T, size_t N>
HWY_API void StoreU(Vec128<T, N> v, Simd<T, N> d, T* HWY_RESTRICT p) {
  Store(v, d, p);
}

// ------------------------------ Non-temporal stores

// Same as aligned stores on non-x86.

template <typename T, size_t N>
HWY_API void Stream(Vec128<T, N> v, Simd<T, N> /* tag */,
                    T* HWY_RESTRICT aligned) {
  wasm_v128_store(aligned, v.raw);
}

// ------------------------------ Gather

// Unsupported.

// ================================================== SWIZZLE

// ------------------------------ Extract lane

// Gets the single value stored in a vector/part.
template <size_t N>
HWY_API uint16_t GetLane(const Vec128<uint16_t, N> v) {
  return wasm_i16x8_extract_lane(v.raw, 0);
}
template <size_t N>
HWY_API int16_t GetLane(const Vec128<int16_t, N> v) {
  return wasm_i16x8_extract_lane(v.raw, 0);
}
template <size_t N>
HWY_API uint32_t GetLane(const Vec128<uint32_t, N> v) {
  return wasm_i32x4_extract_lane(v.raw, 0);
}
template <size_t N>
HWY_API int32_t GetLane(const Vec128<int32_t, N> v) {
  return wasm_i32x4_extract_lane(v.raw, 0);
}
template <size_t N>
HWY_API float GetLane(const Vec128<float, N> v) {
  return wasm_f32x4_extract_lane(v.raw, 0);
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
  // TODO(eustas): use swizzle?
  return Vec128<T, 8 / sizeof(T)>{wasm_v32x4_shuffle(v.raw, v.raw, 2, 3, 2, 3)};
}
template <>
HWY_API Vec128<float, 2> UpperHalf(Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float, 2>{wasm_v32x4_shuffle(v.raw, v.raw, 2, 3, 2, 3)};
}

// ------------------------------ Shift vector by constant #bytes

// 0x01..0F, kBytes = 1 => 0x02..0F00
template <int kBytes, typename T>
HWY_API Vec128<T> ShiftLeftBytes(const Vec128<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  const __i8x16 zero = wasm_i8x16_splat(0);
  switch (kBytes) {
    case 0:
      return v;

    case 1:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 0, 1, 2, 3, 4, 5, 6,
                                          7, 8, 9, 10, 11, 12, 13, 14)};

    case 2:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 0, 1, 2, 3, 4, 5,
                                          6, 7, 8, 9, 10, 11, 12, 13)};

    case 3:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 0, 1, 2, 3,
                                          4, 5, 6, 7, 8, 9, 10, 11, 12)};

    case 4:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 0, 1, 2,
                                          3, 4, 5, 6, 7, 8, 9, 10, 11)};

    case 5:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 0, 1,
                                          2, 3, 4, 5, 6, 7, 8, 9, 10)};

    case 6:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9)};

    case 7:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 0, 1, 2, 3, 4, 5, 6, 7, 8)};

    case 8:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 0, 1, 2, 3, 4, 5, 6, 7)};

    case 9:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 0, 1, 2, 3, 4, 5, 6)};

    case 10:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 0, 1, 2, 3, 4, 5)};

    case 11:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 0, 1, 2, 3, 4)};

    case 12:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 0, 1, 2, 3)};

    case 13:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 0, 1, 2)};

    case 14:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 0,
                                          1)};

    case 15:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 16, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          0)};
  }
  return Vec128<T>{zero};
}

template <int kLanes, typename T>
HWY_API Vec128<T> ShiftLeftLanes(const Vec128<T> v) {
  return ShiftLeftBytes<kLanes * sizeof(T)>(v);
}

// 0x01..0F, kBytes = 1 => 0x0001..0E
template <int kBytes, typename T>
HWY_API Vec128<T> ShiftRightBytes(const Vec128<T> v) {
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  const __i8x16 zero = wasm_i8x16_splat(0);
  switch (kBytes) {
    case 0:
      return v;

    case 1:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 1, 2, 3, 4, 5, 6, 7, 8,
                                          9, 10, 11, 12, 13, 14, 15, 16)};

    case 2:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 2, 3, 4, 5, 6, 7, 8, 9,
                                          10, 11, 12, 13, 14, 15, 16, 16)};

    case 3:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 3, 4, 5, 6, 7, 8, 9, 10,
                                          11, 12, 13, 14, 15, 16, 16, 16)};

    case 4:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 4, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15, 16, 16, 16, 16)};

    case 5:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15, 16, 16, 16, 16, 16)};

    case 6:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 6, 7, 8, 9, 10, 11, 12,
                                          13, 14, 15, 16, 16, 16, 16, 16, 16)};

    case 7:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 7, 8, 9, 10, 11, 12, 13,
                                          14, 15, 16, 16, 16, 16, 16, 16, 16)};

    case 8:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 8, 9, 10, 11, 12, 13, 14,
                                          15, 16, 16, 16, 16, 16, 16, 16, 16)};

    case 9:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 9, 10, 11, 12, 13, 14,
                                          15, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 10:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 10, 11, 12, 13, 14, 15,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 11:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 11, 12, 13, 14, 15, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 12:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 12, 13, 14, 15, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 13:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 13, 14, 15, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 14:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 14, 15, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};

    case 15:
      return Vec128<T>{wasm_v8x16_shuffle(v.raw, zero, 15, 16, 16, 16, 16, 16,
                                          16, 16, 16, 16, 16, 16, 16, 16, 16,
                                          16)};
  }
  return Vec128<T>{zero};
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
  static_assert(0 <= kBytes && kBytes <= 16, "Invalid kBytes");
  switch (kBytes) {
    case 0:
      return lo;

    case 1:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15, 16)};

    case 2:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 2, 3, 4, 5, 6, 7, 8,
                                          9, 10, 11, 12, 13, 14, 15, 16, 17)};

    case 3:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 3, 4, 5, 6, 7, 8, 9,
                                          10, 11, 12, 13, 14, 15, 16, 17, 18)};

    case 4:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 4, 5, 6, 7, 8, 9, 10,
                                          11, 12, 13, 14, 15, 16, 17, 18, 19)};

    case 5:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15, 16, 17, 18, 19, 20)};

    case 6:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15, 16, 17, 18, 19, 20,
                                          21)};

    case 7:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 7, 8, 9, 10, 11, 12,
                                          13, 14, 15, 16, 17, 18, 19, 20, 21,
                                          22)};

    case 8:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 8, 9, 10, 11, 12, 13,
                                          14, 15, 16, 17, 18, 19, 20, 21, 22,
                                          23)};

    case 9:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 9, 10, 11, 12, 13, 14,
                                          15, 16, 17, 18, 19, 20, 21, 22, 23,
                                          24)};

    case 10:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 10, 11, 12, 13, 14,
                                          15, 16, 17, 18, 19, 20, 21, 22, 23,
                                          24, 25)};

    case 11:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 11, 12, 13, 14, 15,
                                          16, 17, 18, 19, 20, 21, 22, 23, 24,
                                          25, 26)};

    case 12:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 12, 13, 14, 15, 16,
                                          17, 18, 19, 20, 21, 22, 23, 24, 25,
                                          26, 27)};

    case 13:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 13, 14, 15, 16, 17,
                                          18, 19, 20, 21, 22, 23, 24, 25, 26,
                                          27, 28)};

    case 14:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 14, 15, 16, 17, 18,
                                          19, 20, 21, 22, 23, 24, 25, 26, 27,
                                          28, 29)};

    case 15:
      return Vec128<T>{wasm_v8x16_shuffle(lo.raw, hi.raw, 15, 16, 17, 18, 19,
                                          20, 21, 22, 23, 24, 25, 26, 27, 28,
                                          29, 30)};
  }
  return hi;
}

// ------------------------------ Broadcast/splat any lane

// Unsigned
template <int kLane, size_t N>
HWY_API Vec128<uint16_t, N> Broadcast(const Vec128<uint16_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<uint16_t, N>{wasm_v16x8_shuffle(
      v.raw, v.raw, kLane, kLane, kLane, kLane, kLane, kLane, kLane, kLane)};
}
template <int kLane, size_t N>
HWY_API Vec128<uint32_t, N> Broadcast(const Vec128<uint32_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<uint32_t, N>{
      wasm_v32x4_shuffle(v.raw, v.raw, kLane, kLane, kLane, kLane)};
}

// Signed
template <int kLane, size_t N>
HWY_API Vec128<int16_t, N> Broadcast(const Vec128<int16_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<int16_t, N>{wasm_v16x8_shuffle(
      v.raw, v.raw, kLane, kLane, kLane, kLane, kLane, kLane, kLane, kLane)};
}
template <int kLane, size_t N>
HWY_API Vec128<int32_t, N> Broadcast(const Vec128<int32_t, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<int32_t, N>{
      wasm_v32x4_shuffle(v.raw, v.raw, kLane, kLane, kLane, kLane)};
}

// Float
template <int kLane, size_t N>
HWY_API Vec128<float, N> Broadcast(const Vec128<float, N> v) {
  static_assert(0 <= kLane && kLane < N, "Invalid lane");
  return Vec128<float, N>{
      wasm_v32x4_shuffle(v.raw, v.raw, kLane, kLane, kLane, kLane)};
}

// ------------------------------ Shuffle bytes with variable indices

// Returns vector of bytes[from[i]]. "from" is also interpreted as bytes:
// either valid indices in [0, 16) or >= 0x80 to zero the i-th output byte.
template <typename T, typename TI>
HWY_API Vec128<T> TableLookupBytes(const Vec128<T> bytes,
                                   const Vec128<TI> from) {
  // TODO(eustas): use swizzle? what about 0x80+ indices?
  alignas(16) uint8_t control[16];
  alignas(16) uint8_t input[16];
  alignas(16) uint8_t output[16];
  wasm_v128_store(control, from.raw);
  wasm_v128_store(input, bytes.raw);
  // TODO(eustas): wasm_v8x16_shuffle does not work: params have to be
  // constants.
  for (size_t i = 0; i < 16; ++i) {
    const int idx = control[i];
    output[i] = (idx >= 0x80) ? 0 : input[idx];
  }
  return Vec128<T>{wasm_v128_load(output)};
}

// ------------------------------ Hard-coded shuffles

// Notation: let Vec128<int32_t> have lanes 3,2,1,0 (0 is least-significant).
// Shuffle0321 rotates one lane to the right (the previous least-significant
// lane is now most-significant). These could also be implemented via
// CombineShiftRightBytes but the shuffle_abcd notation is more convenient.

// Swap 32-bit halves in 64-bit halves.
HWY_API Vec128<uint32_t> Shuffle2301(const Vec128<uint32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<uint32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 0, 3, 2)};
}
HWY_API Vec128<int32_t> Shuffle2301(const Vec128<int32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<int32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 0, 3, 2)};
}
HWY_API Vec128<float> Shuffle2301(const Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 0, 3, 2)};
}

// Swap 64-bit halves
HWY_API Vec128<uint32_t> Shuffle1032(const Vec128<uint32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<uint32_t>{wasm_v64x2_shuffle(v.raw, v.raw, 1, 0)};
}
HWY_API Vec128<int32_t> Shuffle1032(const Vec128<int32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<int32_t>{wasm_v64x2_shuffle(v.raw, v.raw, 1, 0)};
}
HWY_API Vec128<float> Shuffle1032(const Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float>{wasm_v64x2_shuffle(v.raw, v.raw, 1, 0)};
}

// Rotate right 32 bits
HWY_API Vec128<uint32_t> Shuffle0321(const Vec128<uint32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<uint32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 2, 3, 0)};
}
HWY_API Vec128<int32_t> Shuffle0321(const Vec128<int32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<int32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 2, 3, 0)};
}
HWY_API Vec128<float> Shuffle0321(const Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float>{wasm_v32x4_shuffle(v.raw, v.raw, 1, 2, 3, 0)};
}
// Rotate left 32 bits
HWY_API Vec128<uint32_t> Shuffle2103(const Vec128<uint32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<uint32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 0, 1, 2)};
}
HWY_API Vec128<int32_t> Shuffle2103(const Vec128<int32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<int32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 0, 1, 2)};
}
HWY_API Vec128<float> Shuffle2103(const Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 0, 1, 2)};
}

// Reverse
HWY_API Vec128<uint32_t> Shuffle0123(const Vec128<uint32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<uint32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 2, 1, 0)};
}
HWY_API Vec128<int32_t> Shuffle0123(const Vec128<int32_t> v) {
  // TODO(eustas): use swizzle?
  return Vec128<int32_t>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 2, 1, 0)};
}
HWY_API Vec128<float> Shuffle0123(const Vec128<float> v) {
  // TODO(eustas): use swizzle?
  return Vec128<float>{wasm_v32x4_shuffle(v.raw, v.raw, 3, 2, 1, 0)};
}

// ------------------------------ Permute (runtime variable)

// Returned by SetTableIndices for use by TableLookupLanes.
template <typename T>
struct permute_wasm {
  __v128_u raw;
};

template <typename T>
HWY_API permute_wasm<T> SetTableIndices(Full128<T>, const int32_t* idx) {
#if !defined(NDEBUG) || defined(ADDRESS_SANITIZER)
  const size_t N = 16 / sizeof(T);
  for (size_t i = 0; i < N; ++i) {
    // TODO(eustas): also assume idx[i] >= 0
    if (idx[i] >= static_cast<int32_t>(N)) {
      printf("SetTableIndices [%zu] = %d >= %zu\n", i, idx[i], N);
    }
  }
#endif

  const Full128<uint8_t> d8;
  alignas(16) uint8_t control[16];  // = MaxLanes <= Lanes()
  for (size_t idx_byte = 0; idx_byte < 16; ++idx_byte) {
    const size_t idx_lane = idx_byte / sizeof(T);
    const size_t mod = idx_byte % sizeof(T);
    control[idx_byte] = idx[idx_lane] * sizeof(T) + mod;
  }
  return permute_wasm<T>{Load(d8, control).raw};
}

HWY_API Vec128<uint32_t> TableLookupLanes(const Vec128<uint32_t> v,
                                          const permute_wasm<uint32_t> idx) {
  return TableLookupBytes(v, Vec128<uint8_t>{idx.raw});
}

HWY_API Vec128<int32_t> TableLookupLanes(const Vec128<int32_t> v,
                                         const permute_wasm<int32_t> idx) {
  return TableLookupBytes(v, Vec128<uint8_t>{idx.raw});
}

HWY_API Vec128<float> TableLookupLanes(const Vec128<float> v,
                                       const permute_wasm<float> idx) {
  return TableLookupBytes(v, Vec128<uint8_t>{idx.raw});
}

// ------------------------------ Zip lanes

// Same as Interleave*, except that the return lanes are double-width integers;
// this is necessary because the single-lane scalar cannot return two values.

template <size_t N>
HWY_API Vec128<uint16_t, (N + 1) / 2> ZipLower(const Vec128<uint8_t, N> a,
                                               const Vec128<uint8_t, N> b) {
  return Vec128<uint16_t, (N + 1) / 2>{wasm_v8x16_shuffle(
      a.raw, b.raw, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23)};
}
template <size_t N>
HWY_API Vec128<uint32_t, (N + 1) / 2> ZipLower(const Vec128<uint16_t, N> a,
                                               const Vec128<uint16_t, N> b) {
  return Vec128<uint32_t, (N + 1) / 2>{
      wasm_v16x8_shuffle(a.raw, b.raw, 0, 8, 1, 9, 2, 10, 3, 11)};
}

template <size_t N>
HWY_API Vec128<int16_t, (N + 1) / 2> ZipLower(const Vec128<int8_t, N> a,
                                              const Vec128<int8_t, N> b) {
  return Vec128<int16_t, (N + 1) / 2>{wasm_v8x16_shuffle(
      a.raw, b.raw, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23)};
}
template <size_t N>
HWY_API Vec128<int32_t, (N + 1) / 2> ZipLower(const Vec128<int16_t, N> a,
                                              const Vec128<int16_t, N> b) {
  return Vec128<int32_t, (N + 1) / 2>{
      wasm_v16x8_shuffle(a.raw, b.raw, 0, 8, 1, 9, 2, 10, 3, 11)};
}

template <size_t N>
HWY_API Vec128<uint16_t, N / 2> ZipUpper(const Vec128<uint8_t, N> a,
                                         const Vec128<uint8_t, N> b) {
  return Vec128<uint16_t, N / 2>{wasm_v8x16_shuffle(a.raw, b.raw, 8, 24, 9, 25,
                                                    10, 26, 11, 27, 12, 28, 13,
                                                    29, 14, 30, 15, 31)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N / 2> ZipUpper(const Vec128<uint16_t, N> a,
                                         const Vec128<uint16_t, N> b) {
  return Vec128<uint32_t, N / 2>{
      wasm_v16x8_shuffle(a.raw, b.raw, 4, 12, 5, 13, 6, 14, 7, 15)};
}

template <size_t N>
HWY_API Vec128<int16_t, N / 2> ZipUpper(const Vec128<int8_t, N> a,
                                        const Vec128<int8_t, N> b) {
  return Vec128<int16_t, N / 2>{wasm_v8x16_shuffle(a.raw, b.raw, 8, 24, 9, 25,
                                                   10, 26, 11, 27, 12, 28, 13,
                                                   29, 14, 30, 15, 31)};
}
template <size_t N>
HWY_API Vec128<int32_t, N / 2> ZipUpper(const Vec128<int16_t, N> a,
                                        const Vec128<int16_t, N> b) {
  return Vec128<int32_t, N / 2>{
      wasm_v16x8_shuffle(a.raw, b.raw, 4, 12, 5, 13, 6, 14, 7, 15)};
}

// ------------------------------ Interleave lanes

// Interleaves lanes from halves of the 128-bit blocks of "a" (which provides
// the least-significant lane) and "b". To concatenate two half-width integers
// into one, use ZipLower/Upper instead (also works with scalar).

template <typename T>
HWY_API Vec128<T> InterleaveLower(const Vec128<T> a, const Vec128<T> b) {
  return Vec128<T>{ZipLower(a, b).raw};
}
template <>
HWY_API Vec128<uint32_t> InterleaveLower<uint32_t>(const Vec128<uint32_t> a,
                                                   const Vec128<uint32_t> b) {
  return Vec128<uint32_t>{wasm_v32x4_shuffle(a.raw, b.raw, 0, 4, 1, 5)};
}
template <>
HWY_API Vec128<int32_t> InterleaveLower<int32_t>(const Vec128<int32_t> a,
                                                 const Vec128<int32_t> b) {
  return Vec128<int32_t>{wasm_v32x4_shuffle(a.raw, b.raw, 0, 4, 1, 5)};
}
template <>
HWY_API Vec128<float> InterleaveLower<float>(const Vec128<float> a,
                                             const Vec128<float> b) {
  return Vec128<float>{wasm_v32x4_shuffle(a.raw, b.raw, 0, 4, 1, 5)};
}

template <typename T>
HWY_API Vec128<T> InterleaveUpper(const Vec128<T> a, const Vec128<T> b) {
  return Vec128<T>{ZipUpper(a, b).raw};
}
template <>
HWY_API Vec128<uint32_t> InterleaveUpper<uint32_t>(const Vec128<uint32_t> a,
                                                   const Vec128<uint32_t> b) {
  return Vec128<uint32_t>{wasm_v32x4_shuffle(a.raw, b.raw, 2, 6, 3, 7)};
}
template <>
HWY_API Vec128<int32_t> InterleaveUpper<int32_t>(const Vec128<int32_t> a,
                                                 const Vec128<int32_t> b) {
  return Vec128<int32_t>{wasm_v32x4_shuffle(a.raw, b.raw, 2, 6, 3, 7)};
}
template <>
HWY_API Vec128<float> InterleaveUpper<float>(const Vec128<float> a,
                                             const Vec128<float> b) {
  return Vec128<float>{wasm_v32x4_shuffle(a.raw, b.raw, 2, 6, 3, 7)};
}

// ------------------------------ Blocks

// hiH,hiL loH,loL |-> hiL,loL (= lower halves)
template <typename T>
HWY_API Vec128<T> ConcatLowerLower(const Vec128<T> hi, const Vec128<T> lo) {
  return Vec128<T>{wasm_v64x2_shuffle(lo.raw, hi.raw, 0, 2)};
}

// hiH,hiL loH,loL |-> hiH,loH (= upper halves)
template <typename T>
HWY_API Vec128<T> ConcatUpperUpper(const Vec128<T> hi, const Vec128<T> lo) {
  return Vec128<T>{wasm_v64x2_shuffle(lo.raw, hi.raw, 1, 3)};
}

// hiH,hiL loH,loL |-> hiL,loH (= inner halves)
template <typename T>
HWY_API Vec128<T> ConcatLowerUpper(const Vec128<T> hi, const Vec128<T> lo) {
  return CombineShiftRightBytes<8>(hi, lo);
}

// hiH,hiL loH,loL |-> hiH,loL (= outer halves)
template <typename T>
HWY_API Vec128<T> ConcatUpperLower(const Vec128<T> hi, const Vec128<T> lo) {
  return Vec128<T>{wasm_v64x2_shuffle(lo.raw, hi.raw, 0, 3)};
}

// ------------------------------ Odd/even lanes

namespace {

template <typename T>
HWY_API Vec128<T> odd_even_impl(hwy::SizeTag<1> /* tag */, const Vec128<T> a,
                                const Vec128<T> b) {
  const Full128<T> d;
  const Full128<uint8_t> d8;
  alignas(16) constexpr uint8_t mask[16] = {0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0,
                                            0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0};
  return IfThenElse(MaskFromVec(BitCast(d, Load(d8, mask))), b, a);
}
template <typename T>
HWY_API Vec128<T> odd_even_impl(hwy::SizeTag<2> /* tag */, const Vec128<T> a,
                                const Vec128<T> b) {
  return Vec128<T>{wasm_v16x8_shuffle(a.raw, b.raw, 8, 1, 10, 3, 12, 5, 14, 7)};
}
template <typename T>
HWY_API Vec128<T> odd_even_impl(hwy::SizeTag<4> /* tag */, const Vec128<T> a,
                                const Vec128<T> b) {
  return Vec128<T>{wasm_v32x4_shuffle(a.raw, b.raw, 4, 1, 6, 3)};
}
// TODO(eustas): implement
// template <typename T>
// HWY_API Vec128<T> odd_even_impl(hwy::SizeTag<8> /* tag */,
//                                                 const Vec128<T> a,
//                                                 const Vec128<T> b)

}  // namespace

template <typename T>
HWY_API Vec128<T> OddEven(const Vec128<T> a, const Vec128<T> b) {
  return odd_even_impl(hwy::SizeTag<sizeof(T)>(), a, b);
}
template <>
HWY_API Vec128<float> OddEven<float>(const Vec128<float> a,
                                     const Vec128<float> b) {
  return Vec128<float>{wasm_v32x4_shuffle(a.raw, b.raw, 4, 1, 6, 3)};
}

// ================================================== CONVERT

// ------------------------------ Promotions (part w/ narrow lanes -> full)

// Unsigned: zero-extend.
template <size_t N>
HWY_API Vec128<uint16_t, N> PromoteTo(Simd<uint16_t, N> /* tag */,
                                      const Vec128<uint8_t, N> v) {
  return Vec128<uint16_t, N>{wasm_i16x8_widen_low_u8x16(v.raw)};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> PromoteTo(Simd<uint32_t, N> /* tag */,
                                      const Vec128<uint8_t, N> v) {
  return Vec128<uint32_t, N>{
      wasm_i32x4_widen_low_u16x8(wasm_i16x8_widen_low_u8x16(v.raw))};
}
template <size_t N>
HWY_API Vec128<int16_t, N> PromoteTo(Simd<int16_t, N> /* tag */,
                                     const Vec128<uint8_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_widen_low_u8x16(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<uint8_t, N> v) {
  return Vec128<int32_t, N>{
      wasm_i32x4_widen_low_u16x8(wasm_i16x8_widen_low_u8x16(v.raw))};
}
template <size_t N>
HWY_API Vec128<uint32_t, N> PromoteTo(Simd<uint32_t, N> /* tag */,
                                      const Vec128<uint16_t, N> v) {
  return Vec128<uint32_t, N>{wasm_i32x4_widen_low_u16x8(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<uint16_t, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_widen_low_u16x8(v.raw)};
}

// Signed: replicate sign bit.
template <size_t N>
HWY_API Vec128<int16_t, N> PromoteTo(Simd<int16_t, N> /* tag */,
                                     const Vec128<int8_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_widen_low_i8x16(v.raw)};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<int8_t, N> v) {
  return Vec128<int32_t, N>{
      wasm_i32x4_widen_low_i16x8(wasm_i16x8_widen_low_i8x16(v.raw))};
}
template <size_t N>
HWY_API Vec128<int32_t, N> PromoteTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<int16_t, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_widen_low_i16x8(v.raw)};
}

HWY_API Vec128<uint32_t> U32FromU8(const Vec128<uint8_t> v) {
  return Vec128<uint32_t>{
      wasm_i32x4_widen_low_u16x8(wasm_i16x8_widen_low_u8x16(v.raw))};
}

// ------------------------------ Demotions (full -> part w/ narrow lanes)

template <size_t N>
HWY_API Vec128<uint16_t, N> DemoteTo(Simd<uint16_t, N> /* tag */,
                                     const Vec128<int32_t, N> v) {
  return Vec128<uint16_t, N>{wasm_u16x8_narrow_i32x4(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<int16_t, N> DemoteTo(Simd<int16_t, N> /* tag */,
                                    const Vec128<int32_t, N> v) {
  return Vec128<int16_t, N>{wasm_i16x8_narrow_i32x4(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<uint8_t, N> DemoteTo(Simd<uint8_t, N> /* tag */,
                                    const Vec128<int32_t, N> v) {
  const auto intermediate = wasm_i16x8_narrow_i32x4(v.raw, v.raw);
  return Vec128<uint8_t, N>{
      wasm_u8x16_narrow_i16x8(intermediate, intermediate)};
}

template <size_t N>
HWY_API Vec128<uint8_t, N> DemoteTo(Simd<uint8_t, N> /* tag */,
                                    const Vec128<int16_t, N> v) {
  return Vec128<uint8_t, N>{wasm_u8x16_narrow_i16x8(v.raw, v.raw)};
}

template <size_t N>
HWY_API Vec128<int8_t, N> DemoteTo(Simd<int8_t, N> /* tag */,
                                   const Vec128<int32_t, N> v) {
  const auto intermediate = wasm_i16x8_narrow_i32x4(v.raw, v.raw);
  return Vec128<int8_t, N>{wasm_i8x16_narrow_i16x8(intermediate, intermediate)};
}

template <size_t N>
HWY_API Vec128<int8_t, N> DemoteTo(Simd<int8_t, N> /* tag */,
                                   const Vec128<int16_t, N> v) {
  return Vec128<int8_t, N>{wasm_i8x16_narrow_i16x8(v.raw, v.raw)};
}

// For already range-limited input [0, 255].
HWY_API Vec128<uint8_t, 4> U8FromU32(const Vec128<uint32_t> v) {
  const auto intermediate = wasm_i16x8_narrow_i32x4(v.raw, v.raw);
  return Vec128<uint8_t, 4>{
      wasm_u8x16_narrow_i16x8(intermediate, intermediate)};
}

// ------------------------------ Convert i32 <=> f32

template <size_t N>
HWY_API Vec128<float, N> ConvertTo(Simd<float, N> /* tag */,
                                   const Vec128<int32_t, N> v) {
  return Vec128<float, N>{wasm_f32x4_convert_i32x4(v.raw)};
}
// Truncates (rounds toward zero).
template <size_t N>
HWY_API Vec128<int32_t, N> ConvertTo(Simd<int32_t, N> /* tag */,
                                     const Vec128<float, N> v) {
  return Vec128<int32_t, N>{wasm_i32x4_trunc_saturate_f32x4(v.raw)};
}

template <size_t N>
HWY_API Vec128<int32_t, N> NearestInt(const Vec128<float, N> v) {
  const __f32x4 c00 = wasm_f32x4_splat(0.0f);
  const __f32x4 corr = wasm_f32x4_convert_i32x4(wasm_f32x4_le(v.raw, c00));
  const __f32x4 c05 = wasm_f32x4_splat(0.5f);
  // +0.5 for non-negative lane, -0.5 for other.
  const __f32x4 delta = wasm_f32x4_add(c05, corr);
  // Shift input by 0.5 away from 0.
  const __f32x4 fixed = wasm_f32x4_add(v.raw, delta);
  return Vec128<int32_t, N>{wasm_i32x4_trunc_saturate_f32x4(fixed)};
}

// ================================================== MISC

// ------------------------------ Mask

template <typename T>
HWY_API bool AllFalse(const Mask128<T> v) {
  return !wasm_i8x16_any_true(v.raw);
}
HWY_API bool AllFalse(const Mask128<float> v) {
  return !wasm_i32x4_any_true(v.raw);
}

template <typename T>
HWY_API bool AllTrue(const Mask128<T> v) {
  return wasm_i8x16_all_true(v.raw);
}
HWY_API bool AllTrue(const Mask128<float> v) {
  return wasm_i32x4_all_true(v.raw);
}

namespace impl {

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<1> /*tag*/, const Mask128<T> mask) {
  const __i8x16 slice =
      wasm_i8x16_make(1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8);
  // Each u32 lane has byte[i] = (1 << i) or 0.
  const __i8x16 v8_4_2_1 = wasm_v128_and(mask.raw, slice);
  // OR together 4 bytes of each u32 to get the 4 bits.
  const __i16x8 v2_1_z_z = wasm_i32x4_shl(v8_4_2_1, 16);
  const __i16x8 v82_41_2_1 = wasm_v128_or(v8_4_2_1, v2_1_z_z);
  const __i16x8 v41_2_1_0 = wasm_i32x4_shl(v82_41_2_1, 8);
  const __i16x8 v8421_421_21_10 = wasm_v128_or(v82_41_2_1, v41_2_1_0);
  const __i16x8 nibble_per_u32 = wasm_i32x4_shr(v8421_421_21_10, 24);
  // Assemble four nibbles into 16 bits.
  alignas(16) uint32_t lanes[4];
  wasm_v128_store(lanes, nibble_per_u32);
  return lanes[0] | (lanes[1] << 4) | (lanes[2] << 8) | (lanes[3] << 12);
}

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<2> /*tag*/, const Mask128<T> mask) {
  // Remove useless lower half of each u16 while preserving the sign bit.
  const __i16x8 zero = wasm_i16x8_splat(0);
  const Mask128<T> mask8{wasm_i8x16_narrow_i16x8(mask.raw, zero)};
  return BitsFromMask(hwy::SizeTag<1>(), mask8);
}

template <typename T>
HWY_API uint64_t BitsFromMask(hwy::SizeTag<4> /*tag*/, const Mask128<T> mask) {
  const __i32x4 mask_i = static_cast<__i32x4>(mask.raw);
  const __i32x4 slice = wasm_i32x4_make(1, 2, 4, 8);
  const __i32x4 sliced_mask = wasm_v128_and(mask_i, slice);
  alignas(16) uint32_t lanes[4];
  wasm_v128_store(lanes, sliced_mask);
  return lanes[0] | lanes[1] | lanes[2] | lanes[3];
}

}  // namespace impl

template <typename T>
HWY_API uint64_t BitsFromMask(const Mask128<T> mask) {
  return impl::BitsFromMask(hwy::SizeTag<sizeof(T)>(), mask);
}

template <typename T>
HWY_API size_t CountTrue(const Mask128<T> v) {
  const __i32x4 mask =
      wasm_i32x4_make(0x01010101, 0x01010101, 0x02020202, 0x02020202);
  const __i8x16 shifted_bits = wasm_v128_and(v.raw, mask);
  alignas(16) uint64_t lanes[2];
  wasm_v128_store(lanes, shifted_bits);
  return hwy::PopCount(lanes[0] | lanes[1]) / sizeof(T);
}

HWY_API size_t CountTrue(const Mask128<float> v) {
  const __i32x4 var_shift = wasm_i32x4_make(1, 2, 4, 8);
  const __i32x4 shifted_bits = wasm_v128_and(v.raw, var_shift);
  alignas(16) uint64_t lanes[2];
  wasm_v128_store(lanes, shifted_bits);
  return hwy::PopCount(lanes[0] | lanes[1]);
}

// ------------------------------ Horizontal sum (reduction)

// TODO(eustas): optimize
// Returns 64-bit sums of 8-byte groups.
HWY_API Vec128<uint64_t> SumsOfU8x8(const Vec128<uint8_t> v) {
  alignas(16) uint8_t lanes[16];
  wasm_v128_store(lanes, v.raw);
  uint32_t sums[2] = {0};
  for (size_t i = 0; i < 16; ++i) {
    sums[i / 8] += lanes[i];
  }
  return Vec128<uint64_t>{wasm_i32x4_make(sums[0], 0, sums[1], 0)};
}

namespace {

// For u32/i32/f32.
template <typename T, size_t N>
HWY_API Vec128<T, N> horz_sum_impl(hwy::SizeTag<4> /* tag */,
                                   const Vec128<T, N> v3210) {
  const Vec128<T> v1032 = Shuffle1032(v3210);
  const Vec128<T> v31_20_31_20 = v3210 + v1032;
  const Vec128<T> v20_31_20_31 = Shuffle0321(v31_20_31_20);
  return v20_31_20_31 + v31_20_31_20;
}

// For u64/i64/f64.
template <typename T, size_t N>
HWY_API Vec128<T, N> horz_sum_impl(hwy::SizeTag<8> /* tag */,
                                   const Vec128<T, N> v10) {
  const Vec128<T> v01 = Shuffle01(v10);
  return v10 + v01;
}

}  // namespace

// Supported for u/i/f 32/64. Returns the sum in each lane.
template <typename T, size_t N>
HWY_API Vec128<T, N> SumOfLanes(const Vec128<T, N> v) {
  return horz_sum_impl(hwy::SizeTag<sizeof(T)>(), v);
}
