cc_library(
    name = "xxhash",
    hdrs = glob(["src/xxhash/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":xxhash_internal",
    ],
)

default_copts = [
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wc99-extensions",
    "-Wno-unused-command-line-argument",
    "-fdiagnostics-color",
    "-DGRPC_USE_PROTO_LITE",
    "-Wno-implicit-fallthrough",
]

cc_library(
    name = "xxhash_internal",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    include_prefix = "xxhash",
    hdrs = glob(["*.h"]),
    copts = default_copts,
)
