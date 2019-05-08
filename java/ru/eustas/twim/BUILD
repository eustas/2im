package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

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
