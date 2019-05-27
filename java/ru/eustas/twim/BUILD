package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

java_library(
    name = "codec_params",
    srcs = ["CodecParams.java"],
    deps = [":sin_cos"],
)

java_library(
    name = "crc64",
    srcs = ["Crc64.java"],
)

java_test(
    name = "Crc64Test",
    srcs = ["Crc64Test.java"],
    deps = [
        ":crc64",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "cjk_decoder",
    srcs = ["CjkDecoder.java"],
    deps = [":cjk_transform"],
)

java_library(
    name = "cjk_encoder",
    srcs = ["CjkEncoder.java"],
    deps = [":cjk_transform"],
)

java_library(
    name = "cjk_transform",
    srcs = ["CjkTransform.java"],
)

java_test(
    name = "CjkTest",
    srcs = ["CjkTest.java"],
    deps = [
        ":cjk_decoder",
        ":cjk_encoder",
        ":cjk_transform",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "decoder",
    srcs = ["Decoder.java"],
    deps = [
        ":codec_params",
        ":range_decoder",
        ":region",
        ":sin_cos",
    ],
)

java_binary(
    name = "DecoderMain",
    srcs = ["DecoderMain.java"],
    deps = [":decoder"],
)

java_test(
    name = "DecoderTest",
    srcs = ["DecoderTest.java"],
    deps = [":decoder"],
)

java_library(
    name = "encoder",
    srcs = ["Encoder.java"],
    deps = [
        ":codec_params",
        ":range_encoder",
        ":region",
        ":sin_cos",
    ],
)

java_binary(
    name = "EncoderMain",
    srcs = ["EncoderMain.java"],
    deps = [":encoder"],
)

java_test(
    name = "EncoderTest",
    srcs = ["EncoderTest.java"],
    deps = [":encoder"],
)

java_binary(
    name = "MovieMain",
    srcs = ["MovieMain.java"],
    deps = [
        ":codec_params",
        ":decoder",
        ":encoder",
    ],
)

java_library(
    name = "range_code",
    srcs = ["RangeCode.java"],
)

java_library(
    name = "range_decoder",
    srcs = ["RangeDecoder.java"],
    deps = [":range_code"],
)

java_library(
    name = "range_encoder",
    srcs = ["RangeEncoder.java"],
    deps = [":range_code"],
)

java_test(
    name = "RangeTest",
    srcs = ["RangeTest.java"],
    deps = [
        ":range_code",
        ":range_decoder",
        ":range_encoder",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "region",
    srcs = ["Region.java"],
    deps = [
        ":codec_params",
        ":sin_cos",
    ],
)

java_test(
    name = "RegionTest",
    srcs = ["RegionTest.java"],
    deps = [
        ":region",
        ":sin_cos",
    ],
)

java_library(
    name = "sin_cos",
    srcs = ["SinCos.java"],
)

java_test(
    name = "SinCosTest",
    srcs = ["SinCosTest.java"],
    deps = [
        ":crc64",
        ":sin_cos",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "utf_decoder",
    srcs = ["UtfDecoder.java"],
)

java_library(
    name = "utf_encoder",
    srcs = ["UtfEncoder.java"],
)

java_test(
    name = "UtfTest",
    srcs = ["UtfTest.java"],
    deps = [
        ":utf_decoder",
        ":utf_encoder",
        "@junit_junit//jar",
    ]
)
