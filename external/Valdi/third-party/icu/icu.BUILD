# Work in progress

cc_library(
    name = "icu",
    srcs = glob([
        "source/common/**/*.cpp",
    ]),
    hdrs = glob(
        [
            "source/common/**/*.h",
        ],
    ),
    copts = [
        "-Wno-deprecated-pragma",
        "-Wno-unused-but-set-variable",
        "-Wno-unused-private-field",
        "-Wno-deprecated-anon-enum-enum-conversion",
        "-Wno-unused-variable",
        "-Wno-deprecated-declarations",
        "-Wno-unknown-warning-option",
        "-Wno-deprecated-enum-enum-conversion",
    ],
    local_defines = [
        "UCONFIG_NO_BREAK_ITERATION=1",
        "UCONFIG_NO_LEGACY_CONVERSION=1",
        "U_CHARSET_IS_UTF8=1",
        "U_CHAR_TYPE=uint_least16_t",
        "U_HAVE_ATOMIC=1",
        "U_HAVE_STRTOD_L=0",
        "_REENTRANT",
        "U_COMMON_IMPLEMENTATION",
    ],
    strip_include_prefix = "source/common",
    visibility = ["//visibility:public"],
    deps = [],
)
