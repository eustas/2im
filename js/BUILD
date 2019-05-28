package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # MIT

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_library")

closure_js_library(
    name = "CodecParams",
    srcs = ["CodecParams.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":Mini",
        ":SinCos",
    ],
)

closure_js_library(
    name = "CjkDecoder",
    srcs = ["CjkDecoder.js"],
    suppress = ["moduleLoad"],
    deps = [":CjkTransform"],
)

closure_js_library(
    name = "Crc64",
    srcs = ["Crc64.js"],
)

closure_js_library(
    name = "CjkEncoder",
    srcs = ["CjkEncoder.js"],
    suppress = ["moduleLoad"],
    deps = [":CjkTransform"],
)

closure_js_library(
    name = "CjkTransform",
    srcs = ["CjkTransform.js"],
)

closure_js_library(
    name = "Decoder",
    srcs = ["Decoder.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":CodecParams",
        ":RangeDecoder",
        ":Mini",
        ":Region",
        ":SinCos",
    ],
)

closure_js_library(
    name = "Mini",
    srcs = ["Mini.js"],
)

closure_js_library(
    name = "RangeDecoder",
    srcs = ["RangeDecoder.js"],
    suppress = ["moduleLoad"],
    deps = [":Mini"],
)

closure_js_library(
    name = "RangeEncoder",
    srcs = ["RangeEncoder.js"],
)

closure_js_library(
    name = "Region",
    srcs = ["Region.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":CodecParams",
        ":Mini",
        ":SinCos",
    ],
)

closure_js_library(
    name = "SinCos",
    srcs = ["SinCos.js"],
    suppress = ["moduleLoad"],
    deps = [":Mini"],
)

closure_js_library(
    name = "main",
    srcs = ["main.js"],
    suppress = ["moduleLoad", "reportUnknownTypes"],
    #deps = [
    #    ":CjkDecoder",
    #    ":Decoder",
    #],
)

closure_js_library(
    name = "twim-lib",
    srcs = ["twim-lib.js"],
    suppress = ["moduleLoad"],
    deps = [":Decoder"],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_binary")

closure_js_binary(
    name = "app",
    entry_points = ["js/main.js"],
    deps = [":main"],
    language = "ECMASCRIPT_2015",
)

closure_js_binary(
    name = "twim",
    entry_points = ["js/twim-lib.js"],
    deps = [":twim-lib"],
    language = "ECMASCRIPT_2015",
    output_wrapper = "(function(scope){%output%})(window);",
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_test")

closure_js_test(
    name = "Crc64Test",
    srcs = ["Crc64_test.js"],
    entry_points = ["js/Crc64_test.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":Crc64",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "CjkTest",
    srcs = ["Cjk_test.js"],
    entry_points = ["js/Cjk_test.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":CjkDecoder",
        ":CjkEncoder",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "RegionTest",
    srcs = ["Region_test.js"],
    entry_points = ["js/Region_test.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":CodecParams",
        ":Region",
        ":SinCos",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "RangeTest",
    srcs = ["Range_test.js"],
    entry_points = ["js/Range_test.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":RangeDecoder",
        ":RangeEncoder",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "SinCosTest",
    srcs = ["SinCos_test.js"],
    entry_points = ["js/SinCos_test.js"],
    suppress = ["moduleLoad"],
    deps = [
        ":Crc64",
        ":SinCos",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)
