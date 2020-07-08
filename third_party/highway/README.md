## Efficient and performance-portable SIMD wrapper

This library provides type-safe and source-code portable wrappers over existing
platform-specific intrinsics. Its design aims for simplicity, reliable
efficiency across platforms, and immediate usability with current compilers.

## Current status

Implemented for scalar/SSE4/AVX2/AVX-512/ARMv8 targets, each with unit tests.

Tested on GCC 8.3.0 and Clang 6.0.1.

A [quick-reference page](quick_reference.md) briefly lists all operations
and their parameters.

## Design philosophy

*   Performance is important but not the sole consideration. Anyone who goes to
    the trouble of using SIMD clearly cares about speed. However, portability,
    maintainability and readability also matter, otherwise we would write in
    assembly. We aim for performance within 10-20% of a hand-written assembly
    implementation on the development platform.

*   The guiding principles of C++ are "pay only for what you use" and "leave no
    room for a lower-level language below C++". We apply these by defining a
    SIMD API that ensures operation costs are visible, predictable and minimal.

*   Performance portability is important, i.e. the API should be efficient on
    all target platforms. Unfortunately, common idioms for one platform can be
    inefficient on others. For example: summing lanes horizontally versus
    shuffling. Documenting which operations are expensive does not prevent their
    use, as evidenced by widespread use of `HADDPS`. Performance acceptance
    tests may detect large regressions, but do not help choose the approach
    during initial development. Analysis tools can warn about some potential
    inefficiencies, but likely not all. We instead provide [a carefully chosen
    set of vector types and operations that are efficient on all target
    platforms][instmtx] (PPC8, SSE4/AVX2+, ARMv8).

*   Future SIMD hardware features are difficult to predict. For example, AVX2
    came with surprising semantics (almost no interaction between 128-bit
    blocks) and AVX-512 added two kinds of predicates (writemask and zeromask).
    To ensure the API reflects hardware realities, we suggest a flexible
    approach that adds new operations as they become commonly available.

*   Masking is not yet widely supported on current CPUs. It is difficult to
    define an interface that provides access to all platform features while
    retaining performance portability. The P0214R5 proposal lacks support for
    AVX-512/ARM SVE zeromasks. We suggest limiting usage of masks to the
    IfThen[Zero]Else[Zero] functions until the community has gained more
    experience with them.

*   "Width-agnostic" SIMD is more future-proof than user-specified fixed sizes.
    For example, valarray-like code can iterate over a 1D array with a
    library-specified vector width. This will result in better code when vector
    sizes increase, and matches the direction taken by
    [ARM SVE](https://alastairreid.github.io/papers/sve-ieee-micro-2017.pdf) and
    RiscV hardware as well as Agner Fog's
    [ForwardCom instruction set proposal](https://goo.gl/CFizWu). However, some
    applications may require fixed sizes, so we also guarantee support for
    128-bit vectors in each instruction set.

*   The API and its implementation should be usable and efficient with commonly
    used compilers. Some of our open-source users cannot upgrade, so we need to
    support ~4 year old compilers. For example, we write `ShiftLeft<3>(v)`
    instead of `v << 3` because MSVC 2017 (ARM64) does not propagate the literal
    (https://godbolt.org/g/rKx5Ga). However, we do require function-specific
    target attributes, supported by GCC 4.9 / Clang 3.9 / MSVC 2015.

*   Efficient and safe runtime dispatch is important. Modules such as image or
    video codecs are typically embedded into larger applications such as
    browsers, so they cannot require separate binaries for each CPU. Libraries
    also cannot predict whether the application already uses AVX2 (and pays the
    frequency throttling cost), so this decision must be left to the
    application. Using only the lowest-common denominator instructions
    sacrifices too much performance.
    Therefore, we provide code paths for multiple instruction sets and choose
    the most suitable at runtime. To reduce overhead, dispatch should be hoisted
    to higher layers instead of checking inside every low-level function.
    Highway supports inlining functions in the same file or in *-inl.h headers.
    We generate all code paths from the same source to reduce implementation-
    and debugging cost.

*   Not every CPU need be supported. For example, pre-SSE4.1 CPUs are
    increasingly rare and the AVX instruction set is limited to floating-point
    operations. To reduce code size and compile time, we provide specializations
    for SSE4, AVX2 and AVX-512 instruction sets on x86, plus a scalar fallback.

*   Access to platform-specific intrinsics is necessary for acceptance in
    performance-critical projects. We provide conversions to and from intrinsics
    to allow utilizing specialized platform-specific functionality, and simplify
    incremental porting of existing code.

*   The core API should be compact and easy to learn. We provide only the few
    dozen operations which are necessary and sufficient for most of the 150+
    SIMD applications we examined.

## Differences versus [P0214R5 proposal](https://goo.gl/zKW4SA)

1.  Adding widely used and portable operations such as `AndNot`, `AverageRound`,
    bit-shift by immediates and `IfThenElse`.

1.  Designing the API to avoid or minimize overhead on AVX2/AVX-512 caused by
    crossing 128-bit 'block' boundaries.

1.  Avoiding the need for non-native vectors. By contrast, P0214R5's `simd_cast`
    returns `fixed_size<>` vectors which are more expensive to access because
    they reside on the stack. We can avoid this plus additional overhead on
    ARM/AVX2 by defining width-expanding operations as functions of a vector
    part, e.g. promoting half a vector of `uint8_t` lanes to one full vector of
    `uint16_t`, or demoting full vectors to half vectors with half-width lanes.

1.  Guaranteeing access to the underlying intrinsic vector type. This ensures
    all platform-specific capabilities can be used. P0214R5 instead only
    'encourages' implementations to provide access.

1.  Enabling safe runtime dispatch and inlining in the same binary. P0214R5 is
    based on the Vc library, which does not provide assistance for linking
    multiple instruction sets into the same binary. The Vc documentation
    suggests compiling separate executables for each instruction set or using
    GCC's ifunc (indirect functions). The latter is compiler-specific and risks
    crashes due to ODR violations when compiling the same function with
    different compiler flags. We solve this problem via target-specific
    attributes (see HOWTO section below). We also permit a mix of static
    target selection and runtime dispatch for hotspots that may benefit from
    newer instruction sets if available.

1.  Using built-in PPC vector types without a wrapper class. This leads to much
    better code generation with GCC 6.3: https://godbolt.org/z/pd2PNP.
    By contrast, P0214R5 requires a wrapper. We avoid this by using only the
    member operators provided by the PPC vectors; all other functions and
    typedefs are non-members. 2019-04 update: Clang power64le does not have
    this issue, so we simplified get_part(d, v) to GetLane(v).

*   Omitting inefficient or non-performance-portable operations such as `hmax`,
    `operator[]`, and unsupported integer comparisons. Applications can often
    replace these operations at lower cost than emulating that exact behavior.

*   Omitting `long double` types: these are not commonly available in hardware.

*   Ensuring signed integer overflow has well-defined semantics (wraparound).

*   Simple header-only implementation and less than a tenth of the size of the
    Vc library from which P0214 was derived (98,000 lines in
    https://github.com/VcDevel/Vc according to the gloc Chrome extension).

*   Avoiding hidden performance costs. P0214R5 allows implicit conversions from
    integer to float, which costs 3-4 cycles on x86. We make these conversions
    explicit to ensure their cost is visible.

## Prior API designs

The author has been writing SIMD code since 2002: first via assembly language,
then intrinsics, later Intel's `F32vec4` wrapper, followed by three generations
of custom vector classes. The first used macros to generate the classes, which
reduces duplication but also readability. The second used templates instead.
The third (used in highwayhash and PIK) added support for AVX2 and runtime
dispatch. The current design (used in JPEG XL) enables code generation for
multiple platforms and/or instruction sets from the same source, and improves
runtime dispatch.

## Other related work

*   [Neat SIMD](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=7568423)
    adopts a similar approach with interchangeable vector/scalar types and
    a compact interface. It allows access to the underlying intrinsics, but
    does not appear to be designed for other platforms than x86.

*   UME::SIMD ([code](https://goo.gl/yPeVZx), [paper](https://goo.gl/2xpZrk))
    also adopts an explicit vectorization model with vector classes.
    However, it exposes the union of all platform capabilities, which makes the
    API harder to learn (209-page spec) and implement (the estimated LOC count
    is [500K](https://goo.gl/1THFRi)). The API is less performance-portable
    because it allows applications to use operations that are inefficient on
    other platforms.

*   Inastemp ([code](https://goo.gl/hg3USM), [paper](https://goo.gl/YcTU7S))
    is a vector library for scientific computing with some innovative features:
    automatic FLOPS counting, and "if/else branches" using lambda functions.
    It supports IBM Power8, but only provides float and double types.

## Overloaded function API

Most C++ vector APIs rely on class templates. However, the ARM SVE vector
type is sizeless and cannot be wrapped in a class. We instead rely on overloaded
functions. Overloading based on vector types is also undesirable because SVE
vectors cannot be default-constructed. We instead use a dedicated 'descriptor'
type `Simd` for overloading, abbreviated to `D` for template arguments and
`d` in lvalues.

Note that generic function templates are possible (see begin_target-inl.h).

## Masks

AVX-512 introduced a major change to the SIMD interface: special mask registers
(one bit per lane) that serve as predicates. It would be expensive to force
AVX-512 implementations to conform to the prior model of full vectors with lanes
set to all one or all zero bits. We instead provide a Mask type that emulates
a subset of this functionality on other platforms at zero cost.

Masks are returned by comparisons and `TestBit`; they serve as the input to
`IfThen*`. We provide conversions between masks and vector lanes. For clarity
and safety, we use FF..FF as the definition of true. To also benefit from
x86 instructions that only require the sign bit of floating-point inputs to be
set, we provide a special `ZeroIfNegative` function.

## Use cases and HOWTO

Whenever possible, use full-width descriptors for maximum performance
portability: `HWY_FULL(float)`. When necessary, applications may also rely on
128-bit vectors: `Simd<float, 4>`. There is also the option of a vector of up
to N lanes: `HWY_CAPPED(float, N)` (the length is always a power of two).

Functions using Highway must reside between `#include <hwy/begin_target-inl.h>`
and `#include <hwy/end_target-inl.h>`.

*   For static dispatch, `HWY_TARGET` will be the best available target among
    `HWY_BASELINE_TARGETS`, i.e. those allowed for use by the compiler (see
    [quick-reference](quick_reference.md)). Functions defined between
    begin/end_target can be called using `HWY_STATIC_DISPATCH(func)(args)`
    within the same module they are defined in. This `HWY_STATIC_DISPATCH` call
    can be wrapped in a regular function and expose it to other modules
    declaring it in a header file.

*   For dynamic dispatch, a table of function pointers is generated via the
    `HWY_EXPORT` macro that is used by `HWY_DYNAMIC_DISPATCH(func)(args)` to
    call the best function pointer for the current CPU supported targets. A
    module is automatically compiled for each target in `HWY_TARGETS` (see
    [quick-reference](quick_reference.md)) if `HWY_TARGET_INCLUDE` is defined
    and foreach_target.h is included:
```c++
#include "project/module.h"
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "project/module.cc"
#include <hwy/foreach_target.h>
// INSERT other headers

#include <hwy/before_namespace-inl.h>
namespace project {
#include <hwy/begin_target-inl.h>

// INSERT implementations
bool Func1(int x) { ... }

#include <hwy/end_target-inl.h>
}  // namespace project
#include <hwy/after_namespace-inl.h>

#if HWY_ONCE
namespace project {
HWY_EXPORT(Func1)
HWY_EXPORT(Func2)

// Optional wrapper to call Func1 from outside this module using a regular
// function declared in module.h.
bool Func1(int x) {
  return  HWY_DYNAMIC_DISPATCH(Func1)(x);
}

// INSERT any non-SIMD definitions (optional)
}  // namespace project
#endif  // HWY_ONCE
```

See also the `skeleton` examples inside examples/.

Note that tests would preferably cover all supported targets. However, the
default is to skip any baseline targets (e.g. scalar) that are superseded by
another baseline target. To avoid regressions, any patches should be tested
with `-DHWY_COMPILE_ALL_ATTAINABLE` for all files, which will include all
targets. A separate build of only the missing target(s) would also work.

## Installation

This project uses cmake to generate and build and
[googletest](https://github.com/google/googletest) for running unit tests of
this library. In a Debian-based system you can install them with the following
command:

```bash
sudo apt install cmake libgtest-dev
```

To build and test the library the standard cmake workflow can be used:

```bash
mkdir build
cd build
cmake ..
make -j
make test
```

When testing patches on all the attainable targets for your platform use
`cmake .. -DCMAKE_CXX_FLAGS="-DHWY_COMPILE_ALL_ATTAINABLE"` to configure the
project.

## Additional resources

*   [Overview of instructions per operation on different architectures][instmtx]

[instmtx]: instruction_matrix.pdf

This is not an officially supported Google product.
Contact: janwas@google.com
