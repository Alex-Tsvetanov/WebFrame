# Borrowed from third-party/fmt

load("@valdi//bzl:valdi_library.bzl", "COMMON_COMPILE_FLAGS")

cc_library(
    name = "fmt",
    srcs = [
        "src/format.cc",
        "src/os.cc",
    ],
    hdrs = glob(["include/fmt/**/*.h"]),
    copts = COMMON_COMPILE_FLAGS,
    includes = ["include"],
    tags = ["exported"],
    visibility = ["//visibility:public"],
)
