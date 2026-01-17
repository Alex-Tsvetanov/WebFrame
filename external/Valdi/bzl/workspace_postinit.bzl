load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")
load("@rules_jvm_external//:setup.bzl", "rules_jvm_external_setup")

def valdi_post_initialize_workspace():
    llvm_register_toolchains()
    rules_jvm_external_setup()
