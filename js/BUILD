package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # MIT

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_library")

closure_js_library(
    name = "cjkbytes",
    srcs = ["cjkbytes.js"],
)

closure_js_library(
    name = "range",
    srcs = ["range.js"],
)

closure_js_library(
    name = "main",
    srcs = ["main.js"],
    deps = [
        ":cjkbytes",
        ":range",
    ],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_binary")

closure_js_binary(
    name = "app",
    deps = [":main"],
)

load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_test")

closure_js_test(
    name = "cjkbytes_test",
    srcs = ["cjkbytes_test.js"],
    deps = [
        ":cjkbytes",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)

closure_js_test(
    name = "range_test",
    srcs = ["range_test.js"],
    deps = [
        ":range",
        "@io_bazel_rules_closure//closure/library:testing",
    ],
)
