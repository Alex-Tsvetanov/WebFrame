load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")

pkg_files(
    name = "test262_harness_files",
    srcs = glob([
        "src/test262/harness/**/*.js",
    ]),
    strip_prefix = strip_prefix.from_pkg("src/test262"),
)

pkg_files(
    name = "test262_test_files",
    srcs = glob(
        [
            "src/test262/test/**/*.js",
        ],
        exclude = [
            "**/staging/**",
            "**/intl402/**",
            "**/BigInt/**",
        ],
    ),
    strip_prefix = strip_prefix.from_pkg("src/test262"),
)

filegroup(
    name = "zipper",
    srcs = [
        "@valdi//third-party/test262/scripts:zip.sh",
    ],
)

genrule(
    name = "test262_harness",
    srcs = [":test262_harness_files"],
    outs = ["test262_harness.zip"],
    cmd = "$(location :zipper) $@ harness",
    tools = [
        ":zipper",
    ],
    visibility = ["//visibility:public"],
)

genrule(
    name = "test262_tests",
    srcs = [":test262_test_files"],
    outs = ["test262_tests.zip"],
    cmd = "$(location :zipper) $@ test",
    tools = [
        ":zipper",
    ],
    visibility = ["//visibility:public"],
)
