load("@valdi//bzl:valdi_library.bzl", "COMMON_COMPILE_FLAGS")

# Generate a djinni_generated target from another target, to make a module
# with hand written JNI/Objective-C/C++ code compatible with pure djinni targets
def generate_djinni_compat_target(target, source_target):
    native.cc_library(
        name = "djinni_generated_{}".format(target),
        srcs = [],
        hdrs = {},
        visibility = ["//visibility:public"],
        deps = [
            ":{}".format(source_target),
        ],
        tags = ["exported"],
        alwayslink = True,
        copts = COMMON_COMPILE_FLAGS,
        strip_include_prefix = None,
    )
