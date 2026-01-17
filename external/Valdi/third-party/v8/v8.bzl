# TODO(simon): Re-introduce libs folder if we decide to invest into v8 further.

def v8_module(module):
    name = "v8_{}".format(module)

    native.cc_import(
        name = name,
        tags = ["app_size_owners=VALDI"],
        static_library = select({
            "//bzl/conditions:macos_arm64": "libs/macos/arm64/libv8_{}.a".format(module),
            "//bzl/conditions:macos_x86_64": "libs/macos/x86_64/libv8_{}.a".format(module),
            "//bzl/conditions:ios_arm64": "libs/ios/arm64/libv8_{}.a".format(module),
            "//bzl/conditions:ios_x86_64": "libs/ios_simulator/x86_64/libv8_{}.a".format(module),
            "//bzl/conditions:ios_arm64_sim": "libs/ios_simulator/arm64/libv8_{}.a".format(module),
            "//bzl/conditions:android_arm32": "libs/android/armeabi-v7a/libv8_{}.a".format(module),
            "//bzl/conditions:android_arm64": "libs/android/arm64-v8a/libv8_{}.a".format(module),
            "//bzl/conditions:android_x64": "libs/android/x86_64/libv8_{}.a".format(module),
            "//bzl/conditions:linux": "libs/linux/x86_64/libv8_{}.a".format(module),
        }),
        visibility = ["//visibility:public"],
    )
