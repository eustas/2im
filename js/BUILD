package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # MIT

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_library")

closure_js_library(
    name = "CjkDecoder",
    srcs = ["CjkDecoder.js"],
    deps = [":CjkTransform"],
)

closure_js_library(
    name = "CjkEncoder",
    srcs = ["CjkEncoder.js"],
    deps = [":CjkTransform"],
)

closure_js_library(
    name = "CjkTransform",
    srcs = ["CjkTransform.js"],
)

closure_js_library(
    name = "RangeDecoder",
    srcs = ["RangeDecoder.js"],
)

closure_js_library(
    name = "RangeEncoder",
    srcs = ["RangeEncoder.js"],
)

closure_js_library(
    name = "main",
    srcs = ["main.js"],
    deps = [":CjkDecoder"],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_binary")

closure_js_binary(
    name = "app",
    deps = [":main"],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_test")

closure_js_test(
    name = "CjkTest",
    srcs = ["Cjk_test.js"],
    deps = [
        ":CjkDecoder",
        ":CjkEncoder",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "RangeTest",
    srcs = ["Range_test.js"],
    deps = [
        ":RangeDecoder",
        ":RangeEncoder",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)
