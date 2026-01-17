
SNAPCHAT_CFLAGS = [
    "-Os",
    "-Wno-unknown-warning-option",
    "-Wno-unused-variable",
    "-Wno-shorten-64-to-32",
    "-Wno-deprecated-declarations",
    "-Wno-gnu-conditional-omitted-operand",
    "-Wno-c99-extensions",
    "-Wno-c11-extensions",
    "-Wno-variadic-macros",
    "-Wno-deprecated",
    "-Wno-gnu-statement-expression",
    "-Wno-implicit-fallthrough",
    "-Wno-vla-extension",
    "-Wno-gnu-zero-variadic-macro-arguments",
    "-Wno-extra-semi",
    "-Wno-error=unused-property-ivar",
    "-Wno-unguarded-availability-new",
    "-Wno-zero-length-array",
    "-Wno-undefined-var-template",
    "-Wno-ambiguous-reversed-operator",
    "-Wno-range-loop-construct",
    "-Wno-string-conversion",
    "-Wno-null-pointer-subtraction",
    "-Werror",
]

cc_library(
    name = "websocketpp",
    hdrs = glob([
        "websocketpp/**/*.h",
        "websocketpp/**/*.hpp",
    ]),
    copts = SNAPCHAT_CFLAGS + [
        "-DENABLE_CPP11",
        "-DASIO_NO_TYPEID",
    ] + select({
        "@valdi//bzl/conditions:ios": ["-DBOOST_ASIO_SEPARATE_COMPILATION=1"],
        "//conditions:default": [],
    }),
    includes = ["."],
    visibility = ["//visibility:public"],
)
