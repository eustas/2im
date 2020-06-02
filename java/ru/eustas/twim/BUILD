package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

java_library(
    name = "codec_params",
    srcs = ["CodecParams.java"],
    deps = [
        ":range_decoder",
        ":range_encoder",
        ":sin_cos",
    ],
)

java_library(
    name = "crc64",
    srcs = ["Crc64.java"],
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

java_library(
    name = "region",
    srcs = [
        "DistanceRange.java",
        "Region.java",
    ],
    deps = [
        ":codec_params",
        ":sin_cos",
    ],
)

java_library(
    name = "sin_cos",
    srcs = ["SinCos.java"],
)

java_library(
    name = "utf_decoder",
    srcs = ["UtfDecoder.java"],
)

java_library(
    name = "utf_encoder",
    srcs = ["UtfEncoder.java"],
)

# binaries

java_binary(
    name = "Main",
    srcs = ["Main.java"],
    main_class = "ru.eustas.twim.Main",
    deps = [
        ":decoder",
        ":encoder",
    ],
)

java_binary(
    name = "MovieMain",
    srcs = ["MovieMain.java"],
    main_class = "ru.eustas.twim.MovieMain",
    deps = [
        ":codec_params",
        ":decoder",
        ":encoder",
    ],
)

# tests

java_library(
    name = "test_lib",
    srcs = [
        "CjkTest.java",
        "Crc64Test.java",
        "DecoderTest.java",
        "EncoderTest.java",
        "RangeTest.java",
        "RegionTest.java",
        "SinCosTest.java",
        "UtfTest.java",
    ],
    deps = [
        ":codec_params",
        ":cjk_decoder",
        ":cjk_encoder",
        ":cjk_transform",
        ":crc64",
        ":decoder",
        ":encoder",
        ":range_code",
        ":range_decoder",
        ":range_encoder",
        ":region",
        ":sin_cos",
        ":utf_decoder",
        ":utf_encoder",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "CjkTest",
    test_class = "ru.eustas.twim.CjkTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "Crc64Test",
    test_class = "ru.eustas.twim.Crc64Test",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "DecoderTest",
    test_class = "ru.eustas.twim.DecoderTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "EncoderTest",
    test_class = "ru.eustas.twim.EncoderTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "RangeTest",
    test_class = "ru.eustas.twim.RangeTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "RegionTest",
    test_class = "ru.eustas.twim.RegionTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "SinCosTest",
    test_class = "ru.eustas.twim.SinCosTest",
    runtime_deps = [":test_lib"],
)

java_test(
    name = "UtfTest",
    test_class = "ru.eustas.twim.UtfTest",
    runtime_deps = [":test_lib"],
)
