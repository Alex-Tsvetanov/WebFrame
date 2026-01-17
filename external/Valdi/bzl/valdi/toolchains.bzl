def register_valdi_toolchains():
    native.register_toolchains(
        "@valdi//bzl/valdi:valdi_toolchain",
    )
