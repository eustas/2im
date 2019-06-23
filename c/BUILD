package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

cc_library(
    name = "codec",
    srcs = ["sincos.cc"],
    hdrs = [
        "platform.h",
        "sincos.h"
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [":codec"],
)
