COMPILER_FLAGS = [
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wc99-extensions",
    "-Wno-unused-command-line-argument",
    "-fdiagnostics-color",
    "-DGRPC_USE_PROTO_LITE",
    "-Wno-gnu-zero-variadic-macro-arguments",
    "-fno-exceptions",
] + select(
    {
        # needed for building on Mac x86_64
        "@valdi//bzl/conditions:macos_x86_64": ["-DZSTD_DISABLE_ASM"],
        "@valdi//bzl/conditions:ios_x86_64": ["-DZSTD_DISABLE_ASM"],
        "@valdi//bzl/conditions:android_x64": ["-DZSTD_DISABLE_ASM"],
        "@valdi//bzl/conditions:linux_x64": ["-DZSTD_DISABLE_ASM"],
        "//conditions:default": [],
    },
)

cc_library(
    name = "zstd",
    srcs = glob(["lib/**/*.c"]),
    hdrs = glob(["lib/**/*.h"]),
    copts = COMPILER_FLAGS,
    include_prefix = "zstd",
    strip_include_prefix = "lib",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":common_headers",
        ":compress_headers",
        ":decompress_headers",
        ":system_headers",
        ":zstd_exported_headers",
        "@xxhash",
    ],
)

cc_library(
    name = "zstd_exported_headers",
    hdrs = ["lib/zstd.h"],
    strip_include_prefix = "lib",
    tags = ["exported"],
)

cc_library(
    name = "decompress_headers",
    hdrs = glob(["lib/decompress/*.h"]),
    strip_include_prefix = "lib/decompress",
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "compress_headers",
    hdrs = glob(["lib/compress/*.h"]),
    strip_include_prefix = "lib/compress",
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "common_headers",
    hdrs = glob(["lib/common/*.h"]),
    strip_include_prefix = "lib/common",
    visibility = [
        "//visibility:private",
    ],
)

cc_library(
    name = "system_headers",
    hdrs = glob(["lib/**/*.h"]),
    includes = ["src"],
)
