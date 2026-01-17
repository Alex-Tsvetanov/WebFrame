load("@rules_android//:prereqs.bzl", "rules_android_prereqs")
load("@rules_java//java:repositories.bzl", "rules_java_dependencies", "rules_java_toolchains")
load("@rules_kotlin//kotlin:repositories.bzl", "kotlin_repositories", "kotlinc_version")

def valdi_preinitialize_workspace():
    rules_android_prereqs()

    kotlin_repositories(compiler_release = kotlinc_version(
        release = "1.8.10",
        sha256 = "4c3fa7bc1bb9ef3058a2319d8bcc3b7196079f88e92fdcd8d304a46f4b6b5787",
    ))

    rules_java_dependencies()
    rules_java_toolchains()
