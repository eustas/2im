package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

cc_library(
    name = "platform",
    hdrs = ["platform.h"],
)

cc_library(
    name = "image",
    hdrs = ["image.h"],
    deps = [":platform"],
)

cc_library(
    name = "sincos",
    srcs = ["sincos.cc"],
    hdrs = ["sincos.h"],
    deps = [":platform"],
)

cc_library(
    name = "crc64",
    hdrs = ["crc64.h"],
    deps = [":platform"],
)

cc_library(
    name = "range_code",
    hdrs = ["range_code.h"],
    deps = [":platform"],
)

cc_library(
    name = "range_decoder",
    srcs = ["range_decoder.cc"],
    hdrs = ["range_decoder.h"],
    deps = [
        ":platform",
        ":range_code",
    ],
)

cc_library(
    name = "range_encoder",
    srcs = ["range_encoder.cc"],
    hdrs = ["range_encoder.h"],
    deps = [
        ":platform",
        ":range_code",
    ],
)

cc_library(
    name = "region",
    srcs = ["region.cc"],
    hdrs = ["region.h"],
    deps = [
        ":platform",
        ":sincos",
    ],
)

cc_library(
    name = "codec_params",
    srcs = ["codec_params.cc"],
    hdrs = ["codec_params.h"],
    deps = [
        ":range_decoder",
        ":range_encoder",
        ":platform",
        ":sincos",
    ],
)

cc_library(
    name = "distance_range",
    srcs = ["distance_range.cc"],
    hdrs = ["distance_range.h"],
    deps = [
        ":codec_params",
        ":platform",
        ":sincos",
    ],
)

cc_library(
    name = "decoder",
    srcs = ["decoder.cc"],
    hdrs = ["decoder.h"],
    deps = [
        ":codec_params",
        ":distance_range",
        ":image",
        ":platform",
        ":range_decoder",
        ":region",
        ":sincos",
    ],
)

cc_library(
    name = "encoder",
    srcs = ["encoder.cc"],
    hdrs = ["encoder.h"],
    deps = [
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
        ":crc64",
        "@gtest//:main",
    ],
)

cc_test(
    name = "range_test",
    srcs = ["range_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":range_decoder",
        ":range_encoder",
        "@gtest//:main",
    ],
)

cc_test(
    name = "region_test",
    srcs = ["region_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":codec_params",
        ":distance_range",
        ":platform",
        ":region",
        ":sincos",
        "@gtest//:main",
    ],
)

cc_test(
    name = "sincos_test",
    srcs = ["sincos_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    deps = [
        ":crc64",
        ":sincos",
        "@gtest//:main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        ":decoder",
        ":io",
        ":platform",
    ],
)
