package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

java_library(
    name = "codec_params",
    srcs = ["CodecParams.java"],
    deps = [
        ":xrange_decoder",
        ":xrange_encoder",
        ":sin_cos",
    ],
)

java_library(
    name = "crc64",
    srcs = ["Crc64.java"],
)

java_library(
    name = "decoder",
    srcs = ["Decoder.java"],
    deps = [
        ":codec_params",
        ":xrange_decoder",
        ":region",
        ":sin_cos",
    ],
)

java_library(
    name = "encoder",
    srcs = ["Encoder.java"],
    deps = [
        ":codec_params",
        ":xrange_encoder",
        ":region",
        ":sin_cos",
    ],
)

java_library(
    name = "xrange_code",
    srcs = ["XRangeCode.java"],
)

java_library(
    name = "xrange_decoder",
    srcs = ["XRangeDecoder.java"],
    deps = [":xrange_code"],
)

java_library(
    name = "xrange_encoder",
    srcs = ["XRangeEncoder.java"],
    deps = [":xrange_code"],
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

# tests

java_library(
    name = "test_lib",
    srcs = [
        "Crc64Test.java",
        "DecoderTest.java",
        "EncoderTest.java",
        "XRangeTest.java",
        "RegionTest.java",
        "SinCosTest.java",
    ],
    deps = [
        ":codec_params",
        ":crc64",
        ":decoder",
        ":encoder",
        ":xrange_code",
        ":xrange_decoder",
        ":xrange_encoder",
        ":region",
        ":sin_cos",
        "@maven//:junit_junit",
    ],
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
    name = "XRangeTest",
    test_class = "ru.eustas.twim.XRangeTest",
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
