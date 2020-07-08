# API synopsis / quick reference

[[_TOC_]]

## Usage modes

A project or module using SIMD can choose to integrate into its callers via
static or dynamic dispatch. Examples of both are provided in examples/.

Static dispatch means choosing the single CPU target at compile-time. This does
not require any setup nor per-call overhead.

Dynamic dispatch means generating implementations for multiple targets and
choosing the best available at runtime. Uses the same source code as static,
plus `#define HWY_TARGET_INCLUDE` and `#include <hwy/foreach_target.h>`.

## Headers

The public headers are:

*   hwy/base.h: used internally, and potentially also useful for applications
    that want to use its compiler/platform-dependent functionality, typically
   `HWY_ALIGN_MAX` and/or `kMaxVectorSize`.

*   hwy/foreach_target.h: re-includes the translation unit (specified by
    `HWY_TARGET_INCLUDE`) once per enabled target to generate code from the
    same source code. Only needed if dynamic dispatch is desired.

*   hwy/aligned_allocator.h: defines functions for allocating memory with
    alignment suitable for `Load`/`Store`.

*   hwy/cache_control.h: defines stand-alone functions to control caching (e.g.
    prefetching) and memory barriers, independent of actual SIMD.

SIMD implementations must be preceded and followed by two #includes:
```
#include <hwy/before_namespace-inl.h>  // at file scope
namespace project {  // optional
#include <hwy/begin_target-inl.h>

// implementation

#include <hwy/end_target-inl.h>
}  // namespace - if used above
#include <hwy/after_namespace-inl.h>
```

## Preprocessor macros

*   `HWY_ALIGN`: Ensures an array is aligned and suitable for Load()/Store()
    functions. Example: `HWY_ALIGN T lanes[MaxLanes(d)];` This can only be used
    between including begin/end_target-inl.h.

*   `HWY_ALIGN_MAX`: As `HWY_ALIGN`, but aligns to an upper bound suitable for
    all targets on this platform. Use this for caller of SIMD modules, e.g. for
    arrays used as arguments.

## Vector and descriptor types

SIMD vectors consist of one or more 'lanes' of the same built-in type `T =
uint##_t, int##_t, float or double` for `## = 8, 16, 32, 64`. Highway provides
vectors with `N <= kMaxVectorSize / sizeof(T)` lanes, where N is a power of two.

Platforms such as x86 support multiple vector types, and other platforms require
that vectors are built-in types. Thus the Highway API consists of overloaded
functions selected via a zero-sized tag parameter `d` of type `D = Simd<T, N>`.
These are typically constructed using aliases:

*   `const HWY_FULL(T) d;` chooses the maximum N for the current target;
*   `const HWY_CAPPED(T, N) d;` for up to `N` lanes.

The type `T` may be accessed as D::T (prefixed with typename if D is a template
argument).

There are three possibilities for the template parameter `N`:
1.  Equal to the hardware vector width. This is the most common case, e.g. when
    using `HWY_FULL(T)` on a target with compile-time constant vectors.

2.  Less than the hardware vector width. This is the result of a compile-time
    decision by the user, i.e. using `HWY_CAPPED(T, N)` to limit the number of
    lanes, even when the hardware vector width could be greater.

3.  Greater or equal to the hardware vector width, e.g. when the hardware vector
    width is not known at compile-time.

In all cases, `Lanes(d)` returns the actual number of lanes, i.e. the amount by
which to advance loop counters (at most `N`). The constexpr `MaxLanes(d)`
returns an upper bound, typically used as a template argument to another
descriptor. Using it as an array size is discouraged because the bound may be
loose in case 3, leading to excessive stack usage. Where possible, prefer
aligned dynamic allocation of `Lanes(d)` elements.

To construct vectors, call factory functions (see "Initialization" below) with
a tag parameter `d`.

Local variables typically use auto for type deduction. If `d` is
`HWY_FULL(int32_t)`, users may instead use the full-width vector alias `I32xN`
(or `U16xN', `F64xN' etc.) to document the types used.

For function arguments, it is often sufficient to return the same type as the
argument: `template<class V> V Squared(V v) { return v * v; }`. Otherwise, use
the alias `Vec<D>`.

## Operations

In the following, the argument or return type `V` denotes a vector with `N`
lanes. Operations limited to certain vector types begin with a constraint of the
form `V`: `{u,i,f}{8/16/32/64}` to denote unsigned/signed/floating-point types,
possibly with the specified size in bits of `T`.

### Initialization

*   <code>V **Zero**(D)</code>: returns N-lane vector with all bits set to 0.
*   <code>V **Set**(D, T)</code>: returns N-lane vector with all lanes equal to
    the given value of type `T`.
*   <code>V **Undefined**(D)</code>: returns uninitialized N-lane vector, e.g.
    for use as an output parameter.
*   <code>V **Iota**(D, T)</code>: returns N-lane vector where the lane with
    index `i` has the given value of type `T` plus `i`. The least significant
    lane has index 0. (include test_util-inl.h)

### Arithmetic

*   <code>V **operator+**(V a, V b)</code>: returns `a[i] + b[i]` (mod 2^bits).
*   <code>V **operator-**(V a, V b)</code>: returns `a[i] - b[i]` (mod 2^bits).

*   `V`: `ui8/16` \
    <code>V **SaturatedAdd**(V a, V b)</code> returns `a[i] + b[i]` saturated
    to the minimum/maximum representable value.
*   `V`: `ui8/16` \
    <code>V **SaturatedSub**(V a, V b)</code> returns `a[i] - b[i]` saturated
    to the minimum/maximum representable value.

*   `V`: `u8/16` \
    <code>V **AverageRound**(V a, V b)</code> returns `(a[i] + b[i] + 1) / 2`.

*   `V`: `i8/16/32`, `f` \
    <code>V **Abs**(V a)</code> returns the absolute value of `a[i]`;
    for integers, `LimitsMin()` maps to `LimitsMax() + 1`.

*   `V`: `ui8/16/32`, `f` \
    <code>V **Min**(V a, V b)</code>: returns `min(a[i], b[i])`.

*   `V`: `ui8/16/32`, `f` \
    <code>V **Max**(V a, V b)</code>: returns `max(a[i], b[i])`.

*   `V`: `ui8/16/32`, `f` \
    <code>V **Clamp**(V a, V lo, V hi)</code>: returns `a[i]` clamped to
    `[lo[i], hi[i]]`.

*   `V`: `f` \
    <code>V **operator/**(V a, V b)</code>: returns `a[i] / b[i]` in each lane.

*   `V`: `f` \
    <code>V **Sqrt**(V a)</code>: returns `sqrt(a[i])`.

*   `V`: `f32` \
    <code>V **ApproximateReciprocalSqrt**(V a)</code>: returns an approximation
    of `1.0 / sqrt(a[i])`. `sqrt(a) ~= ApproximateReciprocalSqrt(a) * a`. x86
    and PPC provide 12-bit approximations but the error on ARM is closer to 1%.

*   `V`: `f32` \
    <code>V **ApproximateReciprocal**(V a)</code>: returns an approximation of
    `1.0 / a[i]`.

*   `V`: `f32` \
    <code>V **AbsDiff**(V a, V b)</code>: returns `|a[i] - b[i]|` in each
    lane.

#### Multiply

*   `V`: `ui16/32` \
    <code>V <b>operator*</b>(V a, V b)</code>: returns the lower half of
    `a[i] * b[i]` in each lane.

*   `V`: `f` \
    <code>V <b>operator*</b>(V a, V b)</code>: returns `a[i] * b[i]` in each
    lane.

*   `V`: `i16` \
    <code>V **MulHigh**(V a, V b)</code>: returns the upper half of
    `a[i] * b[i]` in each lane.

*   `V`: `ui32` \
    <code>V **MulEven**(V a, V b)</code>: returns double-wide result of
    `a[i] * b[i]` for every even `i`, in lanes `i` (lower) and `i + 1` (upper).

#### Fused multiply-add

When implemented using special instructions, these functions are more precise
and faster than separate multiplication followed by addition. The `*Sub`
variants are somewhat slower on ARM; it is preferable to replace them with
`MulAdd` using a negated constant.

*   `V`: `f` \
    <code>V **MulAdd**(V a, V b, V c)</code>: returns `a[i] * b[i] + c[i]`.

*   `V`: `f` \
    <code>V **NegMulAdd**(V a, V b, V c)</code>: returns `-a[i] * b[i] + c[i]`.

*   `V`: `f` \
    <code>V **MulSub**(V a, V b, V c)</code>: returns `a[i] * b[i] - c[i]`.

*   `V`: `f` \
    <code>V **NegMulSub**(V a, V b, V c)</code>: returns
    `-a[i] * b[i] - c[i]`.

#### Shifts

**Note**: it is generally fastest to shift by a compile-time constant number of
bits. ARM requires the count be less than the lane size.

*   `V`: `ui16/32/64` \
    <code>V **ShiftLeft**&lt;int&gt;(V a)</code> returns `a[i] <<` a
    compile-time constant count.

*   `V`: `u16/32/64`, `i16/32` \
    <code>V **ShiftRight**&lt;int&gt;(V a)</code> returns `a[i] >>` a compile-time
    constant count. Inserts zero or sign bit(s) depending on `V`.

**Note**: independent shifts are only available if `HWY_CAP_VARIABLE_SHIFT`:

*   `V`: `ui32/64` \
    <code>V **operator<<**(V a, V b)</code> returns `a[i] << b[i]`, which is
    zero when `b[i] >= sizeof(T)*8`.

*   `V`: `u32/64`, `i32` \
    <code>V **operator>>**(V a, V b)</code> returns `a[i] >> b[i]`, which is
    zero when `b[i] >= sizeof(T)*8`. Inserts zero or sign bit(s).

**Note**: the following are only provided if `!HWY_CAP_VARIABLE_SHIFT`:

*   `V`: `ui16/32/64` \
    <code>V **ShiftLeftSame**(V a, int bits)</code> returns `a[i] << bits`.

*   `V`: `u16/32/64`, `i16/32` \
    <code>V **ShiftRightSame**(V a, int bits)</code> returns `a[i] >> bits`.
    Inserts 0 or sign bit(s).

#### Floating-point rounding

*   `V`: `f` \
    <code>V **Round**(V a)</code>: returns `a[i]` rounded towards the nearest
    integer, with ties to even.

*   `V`: `f` \
    <code>V **Trunc**(V a)</code>: returns `a[i]` rounded towards zero
    (truncate).

*   `V`: `f` \
    <code>V **Ceil**(V a)</code>: returns `a[i]` rounded towards positive
    infinity (ceiling).

*   `V`: `f` \
    <code>V **Floor**(V a)</code>: returns `a[i]` rounded towards negative
    infinity.

### Logical

These operate on individual bits within each lane.

*   <code>V **operator&**(V a, V b)</code>: returns `a[i] & b[i]`.

*   <code>V **operator|**(V a, V b)</code>: returns `a[i] | b[i]`.

*   <code>V **operator^**(V a, V b)</code>: returns `a[i] ^ b[i]`.

*   <code>V **AndNot**(V a, V b)</code>: returns `~a[i] & b[i]`.

For floating-point types, builtin operators are not always available, so we
provide non-operator functions:
*   <code>V **And**(V a, V b)</code>: returns `a[i] & b[i]`.

*   <code>V **Or**(V a, V b)</code>: returns `a[i] | b[i]`.

*   <code>V **Xor**(V a, V b)</code>: returns `a[i] ^ b[i]`.

*   <code>V **AndNot**(V a, V b)</code>: returns `~a[i] & b[i]`.

### Masks

Let `M` denote a mask capable of storing true/false for each lane.

*   <code>M **MaskFromVec**(V v)</code>: returns false in lane `i` if
    `v[i] == 0`, or true if `v[i]` has all bits set.

*   <code>V **VecFromMask**(M m)</code>: returns 0 in lane `i` if
    `m[i] == false`, otherwise all bits set.

*   <code>V **IfThenElse**(M mask, V yes, V no)</code>:
    returns `mask[i] ? yes[i] : no[i]`.
*   <code>V **IfThenElseZero**(M mask, V yes)</code>:
    returns `mask[i] ? yes[i] : 0`.
*   <code>V **IfThenZeroElse**(M mask, V no)</code>:
    returns `mask[i] ? 0 : no[i]`.

*   <code>V **ZeroIfNegative**(V v)</code>: returns `v[i] < 0 ? 0 : v[i]`.

*   <code>bool **AllTrue**(M m)</code>: returns whether all `m[i]` are
    true.
*   <code>bool **AllFalse**(M m)</code>: returns whether all `m[i]` are
    false.

*   <code>uint64_t **BitsFromMask**(M m)</code>: returns `sum{1 << i}`
    for all indices `i` where `m[i]` is true.

*   <code>size_t **CountTrue**(M m)</code>: returns how many of `m[i]` are
    true [0, N]. This is typically more expensive than AllTrue/False.

### Comparisons

These return a mask (see above) indicating whether the condition is true.

*   <code>M **operator==**(V a, V b)</code>: returns `a[i] == b[i]`.

*   `V`: `if` \
    <code>M **operator&lt;**(V a, V b)</code>: returns `a[i] < b[i]`.
*   `V`: `if` \
    <code>M **operator&gt;**(V a, V b)</code>: returns `a[i] > b[i]`.

*   `V`: `f` \
    <code>M **operator&lt;=**(V a, V b)</code>: returns `a[i] <= b[i]`.
*   `V`: `f` \
    <code>M **operator&gt;=**(V a, V b)</code>: returns `a[i] >= b[i]`.

*   `V`: `ui` \
    <code>M **TestBit**(V v, V bit)</code>: returns `(v[i] & bit[i]) == bit[i]`.
    `bit[i]` must have exactly one bit set.

### Memory

Memory operands are little-endian, otherwise their order would depend on the
lane configuration. Pointers are the addresses of `N` consecutive `T` values,
either naturally-aligned (`aligned`) or possibly unaligned (`p`).

#### Load

*   <code>VT&lt;D&gt; **Load**(D, const T* aligned)</code>: returns
    `aligned[i]`.
*   <code>VT&lt;D&gt; **LoadU**(D, const T* p)</code>: returns `p[i]`.

*   <code>VT&lt;D&gt; **LoadDup128**(D, const T* p)</code>: returns one
    128-bit block loaded from `p` and broadcasted into all 128-bit block\[s\].
    This enables a specialized `U32FromU8` that avoids a 3-cycle overhead on
    AVX2/AVX-512. This may be faster than broadcasting single values, and is
    more convenient than preparing constants for the maximum vector length.

#### Gather

**Note**: only available if `HWY_CAP_GATHER`:

*   `V`,`VI`: (`uif32,i32`), (`uif64,i64`) \
    <code>VT&lt;D&gt; **GatherOffset**(D, const T* base, VI offsets)</code>.
    Returns elements of base selected by signed/possibly repeated *byte*
    `offsets[i]`.

*   `V`,`VI`: (`uif32,i32`), (`uif64,i64`) \
    <code>VT&lt;D&gt; **GatherIndex**(D, const T* base, VI indices)</code>.
    Returns vector of `base[indices[i]]`. Indices are signed and need not be
    unique.

#### Store

*   <code>void **Store**(VT&lt;D&gt; a, D, T* aligned)</code>: copies `a[i]`
    into `aligned[i]`, which must be naturally aligned. Writes exactly
    N * sizeof(T) bytes.
*   <code>void **StoreU**(VT&lt;D&gt; a, D, T* p)</code>: as Store, but
    without the alignment requirement.

### Cache control

All functions except Stream are defined in cache_control.h.

*   <code>void **Stream**(VT&lt;D&gt; a, D, const T* aligned)</code>: copies
    `a[i]` into `aligned[i]` with non-temporal hint on x86 (for good
    performance, call for all consecutive vectors within the same cache line).
    (Over)writes a multiple of HWY_STREAM_MULTIPLE bytes.

*   <code>void **LoadFence**()</code>: delays subsequent loads until prior loads
    are visible. Also a full fence on Intel CPUs. No effect on non-x86.

*   <code>void **StoreFence**()</code>: ensures previous non-temporal stores are
    visible. No effect on non-x86.

*   <code>void **FlushCacheline**(const void* p)</code>: invalidates and flushes
    the cache line containing "p". No effect on non-x86.

*   <code>void **Prefetch**(const T* p)</code>: begins loading the cache line
    containing "p".

### Type conversion

*   <code>VT&lt;D&gt; **BitCast**(D, V)</code>: returns the bits of `V`
    reinterpreted as type `Vec<D>`.

*   `V`,`D`: (`u8,i16`), (`u8,i32`), (`u16,i32`), (`i8,i16`), (`i8,i32`),
    (`i16,i32`), (`f32,f64`) \
    <code>VT&lt;D&gt; **PromoteTo**(D, V part)</code>: returns `part[i]`
    zero- or sign-extended to the wider `D::T` type.

*   `V`,`D`: (`u8,u32`) \
    <code>VT&lt;D&gt; **U32FromU8**(V)</code>: special-case `u8` to `u32` conversion
    when all blocks of `V` are identical, e.g. from `LoadDup128`.

*   `V`,`D`: (`u32,u8`) \
    <code>VT&lt;D&gt; **U8FromU32**(V)</code>: special-case `u32` to `u8` conversion
    when all lanes of `V` are already clamped to `[0, 256)`.

*   `V`,`D`: (`i16,i8`), (`i32,i8`), (`i32,i16`), (`i16,u8`), (`i32,u8`),
    (`i32,u16`), (`f64,f32`) \
    <code>VT&lt;D&gt; **DemoteTo**(D, V a)</code>: returns `a[i]` after packing
    with signed/unsigned saturation, i.e. a vector with narrower type `D::T`.

*   `V`,`D`: (`i32`,`f32`), (`i64`,`f64`) \
    <code>VT&lt;D&gt; **ConvertTo**(D, V)</code>: converts an integer value to
    same-sized floating point.

*   `V`,`D`: (`f32`,`i32`), (`f64`,`i64`) \
    <code>VT&lt;D&gt; **ConvertTo**(D, V)</code>: rounds floating point towards
    zero and converts the value to same-sized integer.

*   `V`: `f32`; `Ret`: `i32` \
    <code>Ret **NearestInt**(V a)</code>: returns the integer nearest to `a[i]`.

### Swizzle

*   <code>T **GetLane**(V)</code>: returns lane 0 within `V`. This is useful
    for extracting `SumOfLanes` results.

*   <code>V2 **Upper/LowerHalf**(V)</code>: returns upper or lower half of
    the vector `V`.

*   <code>V **OddEven**(V a, V b)</code>: returns a vector whose odd lanes are
    taken from `a` and the even lanes from `b`.

**Note**: if vectors are larger than 128 bits, the following operations split
their operands into independently processed 128-bit *blocks*.

*   `V`: `ui16/32/64`, `f` \
    <code>V **Broadcast**&lt;int i&gt;(V)</code>: returns individual *blocks*,
    each with lanes set to `input_block[i]`, `i = [0, 16/sizeof(T))`.

*   `Ret`: double-width `u/i`; `V`: `u8/16/32`, `i8/16/32` \
    <code>Ret **ZipLower**(V a, V b)</code>: returns the same bits as InterleaveLower,
    except that `Ret` is a vector with double-width lanes (required in order to
    use this operation with `scalar`).

**Note**: the following are only available for full vectors (`N > 1), and split
their operands into independently processed 128-bit *blocks*:

*   `Ret`: double-width u/i; `V`: `u8/16/32`, `i8/16/32` \
    <code>Ret **ZipUpper**(V a, V b)</code>: returns the same bits as InterleaveUpper,
    except that `Ret` is a vector with double-width lanes (required in order to
    use this operation with `scalar`).

*   `V`: `ui` \
    <code>V **ShiftLeftBytes**&lt;int&gt;(V)</code>: returns the result of
    shifting independent *blocks* left by `int` bytes \[1, 15\].

*   `V`: `ui` \
    <code>V **ShiftLeftLanes**&lt;int&gt;(V)</code>: returns the result of
    shifting independent *blocks* left by `int` lanes \[1, 15\].

*   `V`: `ui` \
    <code>V **ShiftRightBytes**&lt;int&gt;(V)</code>: returns the result of
    shifting independent *blocks* right by `int` bytes \[1, 15\].

*   `V`: `ui` \
    <code>V **ShiftRightLanes**&lt;int&gt;(V)</code>: returns the result of
    shifting independent *blocks* right by `int` lanes \[1, 15\].

*   `V`: `ui` \
    <code>V **CombineShiftRightBytes**&lt;int&gt;(V hi, V lo)</code>: returns
    the result of shifting two concatenated *blocks* `hi[i] || lo[i]` right by
    `int` bytes \[1, 15\].

*   `V`: `ui`; `VI`: `ui` \
    <code>V **TableLookupBytes**(V bytes, VI from)</code>: returns *blocks* with
    `bytes[from[i]]`, or zero if bit 7 of byte `from[i]` is set.

*   `V`: `uif32` \
    <code>V **Shuffle2301**(V)</code>: returns *blocks* with 32-bit halves
    swapped inside 64-bit halves.

*   `V`: `uif32` \
    <code>V **Shuffle1032**(V)</code>: returns *blocks* with 64-bit halves
    swapped.

*   `V`: `uif64` \
    <code>V **Shuffle01**(V)</code>: returns *blocks* with 64-bit halves
    swapped.

*   `V`: `uif32` \
    <code>V **Shuffle0321**(V)</code>: returns *blocks* rotated right (toward
    the lower end) by 32 bits.

*   `V`: `uif32` \
    <code>V **Shuffle2103**(V)</code>: returns *blocks* rotated left (toward the
    upper end) by 32 bits.

*   `V`: `uif32` \
    <code>V **Shuffle0123**(V)</code>: returns *blocks* with lanes in reverse
    order.

*   <code>V **InterleaveLower**(V a, V b)</code>: returns *blocks* with alternating
    lanes from the lower halves of `a` and `b` (`a[0]` in the least-significant
    lane).

*   <code>V **InterleaveUpper**(V a, V b)</code>: returns *blocks* with alternating
    lanes from the upper halves of `a` and `b` (`a[N/2]` in the
    least-significant lane).

**Note**: the following operations cross block boundaries, which is typically
more expensive on AVX2/AVX-512 than within-block operations.

*   <code>V **ConcatLowerLower**(V hi, V lo)</code>: returns the concatenation of the
    lower halves of `hi` and `lo` without splitting into blocks.

*   <code>V **ConcatUpperUpper**(V hi, V lo)</code>: returns the concatenation of the
    upper halves of `hi` and `lo` without splitting into blocks.

*   <code>V **ConcatLowerUpper**(V hi, V lo)</code>: returns the inner half of the
    concatenation of `hi` and `lo` without splitting into blocks. Useful for
    swapping the two blocks in 256-bit vectors.

*   <code>V **ConcatUpperLower**(V hi, V lo)</code>: returns the outer quarters of the
    concatenation of `hi` and `lo` without splitting into blocks. Unlike the
    other variants, this does not incur a block-crossing penalty on AVX2.

*   `V`: `uif32` \
    <code>V **TableLookupLanes**(V a, VI)</code> returns a vector of
    `a[indices[i]]`, where `VI` is from `SetTableIndices(D, &indices[0])`.

*   <code>VI **SetTableIndices**(D, int* idx)</code> prepares for
    `TableLookupLanes` with lane indices `idx = [0, N)` (need not be unique).

### Misc

**Note**: the following are only available for full vectors (`N > 1`):

*   `V`: `u8`; `Ret`: `u64` \
    <code>Ret **SumsOfU8x8**(V)</code>: returns the sums of 8 consecutive
    bytes in each 64-bit lane.

*   `V`: `uif32/64` \
    <code>V **SumOfLanes**(V v)</code>: returns the sum of all lanes in
    each lane; to obtain the result, use `GetLane(horz_sum_result)`. This is a
    "reduction" (horizontally across lanes), which is less efficient than
    normal ("vertical") SIMD operations.

## Advanced macros

Let `Target` denote an instruction set: `SCALAR/SSE4/AVX2/AVX3/PPC8/NEON/WASM`.
Targets are only used if enabled (i.e. not broken nor disabled). Baseline means
the compiler is allowed to generate such instructions (implying the target CPU
would have to support them).

*   `HWY_Target=##` are powers of two uniquely identifying `Target`.

*   `HWY_STATIC_TARGET` is the best enabled baseline `HWY_Target`, and matches
    `HWY_TARGET` in static dispatch mode. This is useful even in dynamic
    dispatch mode for deducing and printing the compiler flags.

*   `HWY_TARGETS` indicates which targets to generate for dynamic dispatch, and
    which headers to include. It is determined by configuration macros and
    always includes `HWY_STATIC_TARGET`.

*   `HWY_SUPPORTED_TARGETS` is the set of targets available at runtime. Expands
    to a literal if only a single target is enabled, or SupportedTargets().

*   `HWY_TARGET`: which `HWY_Target` is currently being compiled. This is
    initially identical to `HWY_STATIC_TARGET` and remains so in static dispatch
    mode. For dynamic dispatch, this changes before each re-inclusion and
    finally reverts to `HWY_STATIC_TARGET`. Can be used in `#if` expressions to
    provide an alternative to functions which are not supported by HWY_SCALAR.

*   `HWY_LANES(T)`: how many lanes of type `T` in a full vector (>= 1). Used by
    HWY_FULL/CAPPED. Note: cannot be used in #if because it uses sizeof.

*   `HWY_IDE` is 0 except when parsed by IDEs; adding it to conditions such as
    `#if HWY_TARGET != HWY_SCALAR || HWY_IDE` avoids code appearing greyed out.

The following signal capabilities and expand to 1 or 0.
*   `HWY_CAP_GATHER`: whether the current target supports GatherIndex/Offset.
*   `HWY_CAP_VARIABLE_SHIFT`: whether the current target supports variable
    shifts, i.e. per-lane shift amounts (v1 << v2).
*   `HWY_CAP_INT64`: whether the current target supports 64-bit integers.
*   `HWY_CAP_CMP64`: whether the current target supports 64-bit signed
    comparisons.
*   `HWY_CAP_DOUBLE`: whether the current target supports double-precision
    vectors.
*   `HWY_CAP_GE256`: the current target supports vectors of >= 256 bits.
*   `HWY_CAP_GE512`: the current target supports vectors of >= 512 bits.

## Detecting supported targets

`SupportedTargets()` returns a cached (initialized on-demand) bitfield of the
targets supported on the current CPU, detected using CPUID on x86 or equivalent.
This may include targets that are not in `HWY_TARGETS`, and vice versa. If
there is no overlap the binary will likely crash. This can only happen if:

*   the specified baseline is not supported by the current CPU, which
    contradicts the definition of baseline, so the configuration is invalid; or
*   the baseline does not include the enabled/attainable target(s), which are
    also not supported by the current CPU, and baseline targets (in particular
    `HWY_SCALAR`) were explicitly disabled.

## Advanced configuration macros

The following macros govern which targets to generate. Unless specified
otherwise, they may be defined per translation unit, e.g. to disable >128 bit
vectors in modules that do not benefit from them (if bandwidth-limited or only
called occasionally). This is safe because `HWY_TARGETS` always includes at
least one baseline target which `HWY_EXPORT` can use.

*   `HWY_DISABLE_CACHE_CONTROL` makes the cache-control functions no-ops.
*   `HWY_DISABLE_BMI2_FMA` prevents emitting BMI/BMI2/FMA instructions. This
    allows using AVX2 in VMs that do not support the other instructions, but
    only if defined for all translation units.

The following `*_TARGETS` are zero or more `HWY_Target` bits and can be defined
as an expression, e.g. `-DHWY_DISABLED_TARGETS=(HWY_SSE4|HWY_AVX3)`.

*   `HWY_BROKEN_TARGETS` defaults to a blacklist of known compiler bugs.
    Defining to 0 disables the blacklist.

*   `HWY_DISABLED_TARGETS` defaults to zero. This allows explicitly disabling
    targets without interfering with the blacklist.

*   `HWY_BASELINE_TARGETS` defaults to the set whose predefined macros are
    defined (i.e. those for which the corresponding flag, e.g. -mavx2, was
    passed to the compiler). If specified, this should be the same for all
    translation units, otherwise the safety check in SupportedTargets (that all
    enabled baseline targets are supported) may be inaccurate.

Zero or one of the following macros may be defined to replace the default
policy for selecting `HWY_TARGETS`:

*   `HWY_COMPILE_ONLY_SCALAR` selects only `HWY_SCALAR`, which disables SIMD.
*   `HWY_COMPILE_ONLY_STATIC` selects only `HWY_STATIC_TARGET`, which
    effectively disables dynamic dispatch.
*   `HWY_COMPILE_ALL_ATTAINABLE` selects all attainable targets (i.e. enabled
    and permitted by the compiler, independently of autovectorization), which
    maximizes coverage in tests.

If none are defined, the default is to select all attainable targets except any
non-best baseline (typically `HWY_SCALAR`), which reduces code size.

## Compiler support

Clang and GCC require e.g. -mavx2 flags in order to use SIMD intrinsics.
However, this enables AVX2 instructions in the entire translation unit, which
may violate the one-definition rule and cause crashes. Instead, we use
target-specific attributes introduced via #pragma. Function using SIMD must
reside between `#include <begin/end_target-inl.h>`.

Immediates (compile-time constants) are specified as template arguments to avoid
constant-propagation issues with Clang on ARM.

## Type traits

*   `IsFloat<T>()` returns true if the T is a floating-point type.
*   `IsSigned<T>()` returns true if the T is a signed or floating-point type.
*   `LimitsMin/Max<T>()` return the smallest/largest value representable in T.
*   `SizeTag<N>` is an empty struct, used to select overloaded functions
    appropriate for N bytes.

## Memory allocation

`AllocateAligned<T>(items)` returns a unique pointer to newly allocated memory
for `items` elements of type `T`. The start address is aligned as required by
`Load/Store`. Furthermore, successive allocations are not congruent modulo a
platform-specific alignment. This helps prevent false dependencies or cache
conflicts.
`AllocateSingleAligned<T>()` is as above, but returns a pointer to a single
POD item (typically a struct containing multiple members with `HWY_ALIGN`, or
arrays whose lengths are known to be a multiple of the vector size).
