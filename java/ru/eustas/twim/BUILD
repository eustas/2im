package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

java_library(
    name = "cjkbytes",
    srcs = ["CjkBytes.java"],
)

java_test(
    name = "CjkBytesTest",
    srcs = ["CjkBytesTest.java"],
    deps = [
        ":cjkbytes",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "range",
    srcs = ["Range.java"],
)

java_test(
    name = "RangeTest",
    srcs = ["RangeTest.java"],
    deps = [
        ":range",
        "@junit_junit//jar",
    ]
)

java_library(
    name = "utf8",
    srcs = ["Utf8.java"],
)

java_test(
    name = "Utf8Test",
    srcs = ["Utf8Test.java"],
    deps = [
        ":utf8",
        "@junit_junit//jar",
    ]
)
