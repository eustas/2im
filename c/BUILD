package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

DEFAULT_COPTS = ["-march=native"]

TEST_COPTS = DEFAULT_COPTS + ["-Iexternal/gtest/include"]

cc_library(
    name = "platform",
    hdrs = ["platform.h"],
    copts = DEFAULT_COPTS,
)

cc_library(
    name = "image",
    srcs = ["image.cc"],
    hdrs = ["image.h"],
    copts = DEFAULT_COPTS,
    deps = [":platform"],
)

cc_library(
    name = "sin_cos",
    srcs = ["sin_cos.cc"],
    hdrs = ["sin_cos.h"],
    copts = DEFAULT_COPTS,
    deps = [":platform"],
)

cc_library(
    name = "crc64",
    hdrs = ["crc64.h"],
    copts = DEFAULT_COPTS,
    deps = [":platform"],
)

cc_library(
    name = "range_code",
    hdrs = ["range_code.h"],
    copts = DEFAULT_COPTS,
    deps = [":platform"],
)

cc_library(
    name = "range_decoder",
    srcs = ["range_decoder.cc"],
    hdrs = ["range_decoder.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":platform",
        ":range_code",
    ],
)

cc_library(
    name = "range_encoder",
    srcs = ["range_encoder.cc"],
    hdrs = ["range_encoder.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":platform",
        ":range_code",
    ],
)

cc_library(
    name = "region",
    srcs = ["region.cc"],
    hdrs = ["region.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":platform",
        ":sin_cos",
    ],
)

cc_library(
    name = "codec_params",
    srcs = ["codec_params.cc"],
    hdrs = ["codec_params.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":platform",
        ":range_decoder",
        ":range_encoder",
        ":sin_cos",
    ],
)

cc_library(
    name = "distance_range",
    srcs = ["distance_range.cc"],
    hdrs = ["distance_range.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":codec_params",
        ":platform",
        ":sin_cos",
    ],
)

cc_library(
    name = "decoder",
    srcs = ["decoder.cc"],
    hdrs = ["decoder.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":codec_params",
        ":distance_range",
        ":image",
        ":platform",
        ":range_decoder",
        ":region",
        ":sin_cos",
    ],
)

cc_library(
    name = "encoder",
    srcs = ["encoder.cc"],
    hdrs = [
        "encoder.h",
        "encoder_internal.h",
    ],
    copts = DEFAULT_COPTS,
    deps = [
        ":codec_params",
        ":distance_range",
        ":image",
        ":platform",
        ":region",
    ],
)

cc_library(
    name = "io",
    srcs = ["io.cc"],
    hdrs = ["io.h"],
    copts = DEFAULT_COPTS,
    deps = [
        ":image",
        ":platform",
        "@png",
    ],
)

cc_test(
    name = "crc64_test",
    srcs = ["crc64_test.cc"],
    copts = TEST_COPTS,
    deps = [
        ":crc64",
        "@gtest//:main",
    ],
)

cc_test(
    name = "range_test",
    srcs = ["range_test.cc"],
    copts = TEST_COPTS,
    deps = [
        ":range_decoder",
        ":range_encoder",
        "@gtest//:main",
    ],
    shard_count = 10,
)

cc_test(
    name = "region_test",
    srcs = ["region_test.cc"],
    copts = TEST_COPTS,
    deps = [
        ":codec_params",
        ":distance_range",
        ":platform",
        ":region",
        ":sin_cos",
        "@gtest//:main",
    ],
)

cc_test(
    name = "sin_cos_test",
    srcs = ["sin_cos_test.cc"],
    copts = TEST_COPTS,
    deps = [
        ":crc64",
        ":sin_cos",
        "@gtest//:main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    copts = DEFAULT_COPTS,
    linkopts = ["-pthread"],
    deps = [
        ":decoder",
        ":encoder",
        ":io",
        ":platform",
    ],
)
