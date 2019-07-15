package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

cc_library(
    name = "platform",
    hdrs = [
        "platform.h",
    ],
)

cc_library(
    name = "image",
    hdrs = [
        "image.h",
    ],
)

cc_library(
    name = "codec",
    srcs = [
        "codec_params.cc",
        "decoder.cc",
        "distance_range.cc",
        "range_decoder.cc",
        "range_encoder.cc",
        "region.cc",
        "sincos.cc",
    ],
    hdrs = [
        "codec_params.h",
        "crc64.h",
        "decoder.h",
        "distance_range.h",
        "range_code.h",
        "range_decoder.h",
        "range_encoder.h",
        "region.h",
        "sincos.h",
    ],
    deps = [
        ":image",
        ":platform",
    ],
)

cc_library(
    name = "io",
    srcs = ["io.cc"],
    hdrs = ["io.h"],
    deps = [
        ":image",
        ":platform",
        "@png//:png",
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

cc_test(
    name = "range_test",
    srcs = ["range_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":codec",
        "@gtest//:main",
    ],
)

cc_test(
    name = "region_test",
    srcs = ["region_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":codec",
        "@gtest//:main",
    ],
)
cc_test(
    name = "sincos_test",
    srcs = ["sincos_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":codec",
        "@gtest//:main",
    ],
)
cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        ":codec",
        ":io",
    ],
)
