// Copyright 2020 Google LLC
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

#ifndef HWY_BASE_H_
#define HWY_BASE_H_

// For SIMD module implementations and their callers, target-independent.

#include <stddef.h>
#include <stdint.h>
#include <atomic>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// Add to #if conditions to prevent IDE from graying out code.
#if (defined __CDT_PARSER__) || (defined __INTELLISENSE__) || \
    (defined Q_CREATOR_RUN)
#define HWY_IDE 1
#else
#define HWY_IDE 0
#endif

//------------------------------------------------------------------------------
// Detect compiler using predefined macros

#ifdef _MSC_VER
#define HWY_COMPILER_MSVC _MSC_VER
#else
#define HWY_COMPILER_MSVC 0
#endif

#ifdef __GNUC__
#define HWY_COMPILER_GCC (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#define HWY_COMPILER_GCC 0
#endif

// Clang can masquerade as MSVC/GCC, in which case both are set.
#ifdef __clang__
#define HWY_COMPILER_CLANG (__clang_major__ * 100 + __clang_minor__)
#else
#define HWY_COMPILER_CLANG 0
#endif

// More than one may be nonzero, but we want at least one.
#if !HWY_COMPILER_MSVC && !HWY_COMPILER_GCC && !HWY_COMPILER_CLANG
#error "Unsupported compiler"
#endif

//------------------------------------------------------------------------------
// Compiler-specific definitions

#if HWY_COMPILER_MSVC

#include <intrin.h>

#define HWY_RESTRICT __restrict
#define HWY_INLINE __forceinline
#define HWY_NOINLINE __declspec(noinline)
#define HWY_FLATTEN
#define HWY_NORETURN __declspec(noreturn)
#define HWY_LIKELY(expr) (expr)
#define HWY_UNLIKELY(expr) (expr)
#define HWY_DIAGNOSTICS(tokens) __pragma(warning(tokens))
#define HWY_DIAGNOSTICS_OFF(msc, gcc) HWY_DIAGNOSTICS(msc)
#define HWY_MAYBE_UNUSED
#define HWY_HAS_ASSUME_ALIGNED 0
#if (_MSC_VER >= 1700)
#define HWY_MUST_USE_RESULT _Check_return_
#else
#define HWY_MUST_USE_RESULT
#endif

#else

#define HWY_RESTRICT __restrict__
#define HWY_INLINE inline __attribute__((always_inline))
#define HWY_NOINLINE __attribute__((noinline))
#define HWY_FLATTEN __attribute__((flatten))
#define HWY_NORETURN __attribute__((noreturn))
#define HWY_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define HWY_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define HWY_PRAGMA(tokens) _Pragma(#tokens)
#define HWY_DIAGNOSTICS(tokens) HWY_PRAGMA(GCC diagnostic tokens)
#define HWY_DIAGNOSTICS_OFF(msc, gcc) HWY_DIAGNOSTICS(gcc)
// Encountered "attribute list cannot appear here" when using the C++17
// [[maybe_unused]], so only use the old style attribute for now.
#define HWY_MAYBE_UNUSED __attribute__((unused))
#define HWY_MUST_USE_RESULT __attribute__((warn_unused_result))

#endif  // !HWY_COMPILER_MSVC

//------------------------------------------------------------------------------
// Builtin/attributes

#ifdef __has_builtin
#define HWY_HAS_BUILTIN(name) __has_builtin(name)
#else
#define HWY_HAS_BUILTIN(name) 0
#endif

#ifdef __has_attribute
#define HWY_HAS_ATTRIBUTE(name) __has_attribute(name)
#else
#define HWY_HAS_ATTRIBUTE(name) 0
#endif

// Enables error-checking of format strings.
#if HWY_HAS_ATTRIBUTE(__format__)
#define HWY_FORMAT(idx_fmt, idx_arg) \
  __attribute__((__format__(__printf__, idx_fmt, idx_arg)))
#else
#define HWY_FORMAT(idx_fmt, idx_arg)
#endif

// Returns a void* pointer which the compiler then assumes is N-byte aligned.
// Example: float* HWY_RESTRICT aligned = (float*)HWY_ASSUME_ALIGNED(in, 32);
//
// The assignment semantics are required by GCC/Clang. ICC provides an in-place
// __assume_aligned, whereas MSVC's __assume appears unsuitable.
#if HWY_HAS_BUILTIN(__builtin_assume_aligned)
#define HWY_ASSUME_ALIGNED(ptr, align) __builtin_assume_aligned((ptr), (align))
#else
#define HWY_ASSUME_ALIGNED(ptr, align) (ptr) /* not supported */
#endif

//------------------------------------------------------------------------------
// Detect architecture using predefined macros

#if defined(__i386__) || defined(_M_IX86)
#define HWY_ARCH_X86_32 1
#else
#define HWY_ARCH_X86_32 0
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define HWY_ARCH_X86_64 1
#else
#define HWY_ARCH_X86_64 0
#endif

#if HWY_ARCH_X86_32 || HWY_ARCH_X86_64
#define HWY_ARCH_X86 1
#else
#define HWY_ARCH_X86 0
#endif

#if defined(__powerpc64__) || defined(_M_PPC)
#define HWY_ARCH_PPC 1
#else
#define HWY_ARCH_PPC 0
#endif

#if defined(__arm__) || defined(_M_ARM) || defined(__aarch64__)
#define HWY_ARCH_ARM 1
#else
#define HWY_ARCH_ARM 0
#endif

// There isn't yet a standard __wasm or __wasm__.
#ifdef __EMSCRIPTEN__
#define HWY_ARCH_WASM 1
#else
#define HWY_ARCH_WASM 0
#endif

#if HWY_ARCH_X86 + HWY_ARCH_PPC + HWY_ARCH_ARM + HWY_ARCH_WASM != 1
#error "Must detect exactly one platform"
#endif

//------------------------------------------------------------------------------
// Macros

#define HWY_STR_IMPL(macro) #macro
#define HWY_STR(macro) HWY_STR_IMPL(macro)

#define HWY_CONCAT_IMPL(a, b) a##b
#define HWY_CONCAT(a, b) HWY_CONCAT_IMPL(a, b)

#define HWY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define HWY_MAX(a, b) ((a) < (b) ? (b) : (a))

// Alternative for asm volatile("" : : : "memory"), which has no effect.
#define HWY_FENCE std::atomic_thread_fence(std::memory_order_acq_rel)

// 4 instances of a given literal value, useful as input to LoadDup128.
#define HWY_REP4(literal) literal, literal, literal, literal

// Only for "debug" builds (test_util-inl has always-on HWY_ASSERT for tests)
#if !defined(NDEBUG) || defined(ADDRESS_SANITIZER) || \
    defined(MEMORY_SANITIZER) || defined(THREAD_SANITIZER)
#define HWY_DASSERT(condition)                                         \
  do {                                                                 \
    if (!(condition)) {                                                \
      ::hwy::Abort(__FILE__, __LINE__, "Debug Assert %s", #condition); \
    }                                                                  \
  } while (0)
#else
#define HWY_DASSERT(condition) \
  do {                         \
  } while (0)
#endif

//------------------------------------------------------------------------------

namespace hwy {

#if HWY_ARCH_X86
static constexpr size_t kMaxVectorSize = 64;  // AVX-512
#define HWY_ALIGN_MAX alignas(64)
#else
static constexpr size_t kMaxVectorSize = 16;
#define HWY_ALIGN_MAX alignas(16)
#endif

HWY_NORETURN void HWY_FORMAT(3, 4)
    Abort(const char* file, int line, const char* format, ...);

template <typename T>
constexpr bool IsFloat() {
  return T(1.25) != T(1);
}

template <typename T>
constexpr bool IsSigned() {
  return T(0) > T(-1);
}

// Largest/smallest representable integer values.
template <typename T>
constexpr T LimitsMax() {
  return IsSigned<T>() ? T((1ULL << (sizeof(T) * 8 - 1)) - 1)
                       : static_cast<T>(~0ull);
}
template <typename T>
constexpr T LimitsMin() {
  return IsSigned<T>() ? T(-1) - LimitsMax<T>() : T(0);
}

// Manual control of overload resolution (SFINAE).
template <bool Condition, class T>
struct EnableIfT {};
template <class T>
struct EnableIfT<true, T> {
  using type = T;
};

template <bool Condition, class T = void>
using EnableIf = typename EnableIfT<Condition, T>::type;

// Undefined results for x == 0.
static HWY_INLINE HWY_MAYBE_UNUSED size_t
NumZeroBitsBelowLSBNonzero32(const uint32_t x) {
#ifdef _MSC_VER
  unsigned long index;
  _BitScanForward(&index, x);
  return index;
#else
  return static_cast<size_t>(__builtin_ctz(x));
#endif
}

}  // namespace hwy

#endif  // HWY_BASE_H_
