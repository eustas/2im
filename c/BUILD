package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

cc_library(
    name = "codec",
    srcs = ["sincos.cc"],
    hdrs = [
        "crc64.h",
        "platform.h",
        "sincos.h"
    ],
)

cc_test(
    name = "crc64_test",
    srcs = ["crc64_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":codec",
        "@gtest//:main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [":codec"],
)
