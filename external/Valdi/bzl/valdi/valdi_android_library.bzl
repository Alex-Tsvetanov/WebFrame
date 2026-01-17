load("@rules_kotlin//kotlin:android.bzl", "kt_android_library")

def valdi_android_library(
        name,
        **kwargs):
    kt_android_library(name = name, plugins = [
        "@valdi//valdi:annotation_processor",
    ], **kwargs)
