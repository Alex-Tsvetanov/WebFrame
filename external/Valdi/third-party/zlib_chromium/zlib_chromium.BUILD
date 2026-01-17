gzip_src = [
    "gzlib.c",
    "gzread.c",
    "gzwrite.c",
    "gzclose.c",
]

cc_library(
    name = "zlib_chromium",
    srcs = [
        "adler32.c",
        "compress.c",
        "cpu_features.c",
        "crc32.c",
        "deflate.c",
        "infback.c",
        "inffast.c",
        "inflate.c",
        "inftrees.c",
        "trees.c",
        "uncompr.c",
        "zutil.c",
        "contrib/optimizations/insert_string.h",
    ] + select({
        "@platforms//os:linux": gzip_src,
        "@platforms//os:macos": gzip_src,
        "//conditions:default": [],
    }) + select({
        "@platforms//cpu:armv7": ["contrib/optimizations/slide_hash_neon.h"],
        "@platforms//cpu:aarch64": ["contrib/optimizations/slide_hash_neon.h"],
        "//conditions:default": [],
    }),
    hdrs = glob(["*.h"]),
    copts = [
        "-Werror",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-Wc99-extensions",
        "-Wno-unused-command-line-argument",
        "-fdiagnostics-color",
        "-DGRPC_USE_PROTO_LITE",
        "-Wno-error=sign-compare",
        "-Wno-sign-compare",
        "-Wno-char-subscripts",
        "-Wno-missing-field-initializers",
        "-Wno-implicit-fallthrough",
        "-Wno-pedantic",
        "-Wno-shorten-64-to-32",
        "-Wno-unreachable-code",
        "-Wno-unused-variable",
        "-Wno-unused-function",
        "-Wno-deprecated-non-prototype",  # ADL-18113
        "-Wno-unknown-warning-option",  # because of -Wno-deprecated-non-prototype
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "zlib_chromium_headers",
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
)
