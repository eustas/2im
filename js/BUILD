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

#load("@io_bazel_rules_closure//closure:defs.bzl", "closure_js_test")
#closure_js_test(
#    name = "all_tests",
#    srcs = ["decode_test.js"],
#    deps = [
#        ":decode",
#        ":polyfill",
#        "@io_bazel_rules_closure//closure/library:testing",
#    ],
#)
