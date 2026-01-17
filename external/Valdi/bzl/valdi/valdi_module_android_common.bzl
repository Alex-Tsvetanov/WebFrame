# Deps every Valdi module target depends on.

COMMON_ANDROIDX_DEPS = [
    "@android_mvn//:androidx_annotation_annotation",
    "@android_mvn//:androidx_appcompat_appcompat",
    "@android_mvn//:androidx_core_core",
    "@android_mvn//:javax_inject_javax_inject",
]

COMMON_FRAMEWORK_DEPS = [
    "@valdi//valdi_core:valdi_core_java",
    "@valdi//valdi:valdi_java",
]
