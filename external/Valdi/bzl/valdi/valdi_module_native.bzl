COMPILER_FLAGS = [
    "-Wno-unused-label",
    "-Wno-unused-variable",
    "-fno-exceptions",
    "-Wno-unused-but-set-variable",
    "-Wno-unknown-warning-option",
    "-Os",  # TSN generated code needs more optimization than the default -Oz
]

def valdi_module_native(name, srcs, deps):
    native.cc_library(
        name = name,
        hdrs = [],
        includes = [],
        copts = COMPILER_FLAGS,
        alwayslink = 1,
        srcs = srcs if srcs else [],
        deps = ["@valdi//tsn"] + deps,
        visibility = ["//visibility:public"],
    )
