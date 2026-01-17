cc_library(
    name = "zoo",
    hdrs = glob([
        "inc/zoo/**/*.h",
    ]),
    strip_include_prefix = "inc",
    tags = ["exported"],
    visibility = ["//visibility:public"],
    deps = [":additional_headers"],
)

cc_library(
    name = "additional_headers",
    hdrs = glob([
        "src/zoo/*.h",
    ]),
    strip_include_prefix = "src",
    tags = ["exported"],
    visibility = ["//visibility:public"],
)

alias(
    name = "zoo_additional_headers",
    actual = ":additional_headers",
    visibility = ["//visibility:public"],
)
