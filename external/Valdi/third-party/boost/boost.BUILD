load("@valdi//bzl:valdi_library.bzl", "COMMON_COMPILE_FLAGS")

cc_library(
    name = "boost",
    srcs = glob([
        "asio.cpp",
        "asio-ssl.cpp",
    ]),
    hdrs = glob([
        "boost/**/*.hpp",
        "boost/**/*.ipp",
        "boost/**/*.h",
    ]),
    copts = COMMON_COMPILE_FLAGS + [
        "-Wno-c11-extensions",
        "-Wno-deprecated-copy",
        "-Wno-deprecated-declarations",
        "-Wno-extra-semi",
        "-Wno-shorten-64-to-32",
        "-Wno-sign-compare",
        "-Wno-undefined-var-template",
        "-Wno-deprecated-builtins",  # CPP-5057
        "-Wno-unknown-warning-option",
    ],
    defines = [
        "BOOST_ASIO_SEPARATE_COMPILATION",
        "_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION",  # CPP-5057
    ] + select({
        # Required for iOS 12
        "@platforms//os:ios": ["BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC"],
        "//conditions:default": [],
    }),
    includes = ["."],
    tags = ["exported"],
    visibility = ["//visibility:public"],
    deps = ["@boringssl//:ssl"],
)

cc_library(
    name = "boost_thread",
    srcs = glob(
        [
            "src/libs/thread/src/pthread/*.cpp",
            "src/libs/thread/src/pthread/*.hpp",
        ],
        exclude = [
            "src/libs/thread/src/pthread/once_atomic.cpp",
        ],
    ),
    # once_atomic.cpp is directly included by once.cpp
    hdrs = ["src/libs/thread/src/pthread/once_atomic.cpp"],
    deps = [":boost"],
)

cc_library(
    name = "boost_pc",
    srcs = glob(
        [
            "src/libs/filesystem/src/*.cpp",
            "src/libs/filesystem/src/*.hpp",
            "src/libs/log/src/attribute_name.cpp",
            "src/libs/log/src/attribute_set.cpp",
            "src/libs/log/src/attribute_value_set.cpp",
            "src/libs/log/src/code_conversion.cpp",
            "src/libs/log/src/core.cpp",
            "src/libs/log/src/date_time_format_parser.cpp",
            "src/libs/log/src/default_attribute_names.cpp",
            "src/libs/log/src/default_sink.cpp",
            "src/libs/log/src/dump.cpp",
            "src/libs/log/src/event.cpp",
            "src/libs/log/src/exceptions.cpp",
            "src/libs/log/src/global_logger_storage.cpp",
            "src/libs/log/src/once_block.cpp",
            "src/libs/log/src/record_ostream.cpp",
            "src/libs/log/src/severity_level.cpp",
            "src/libs/log/src/text_file_backend.cpp",
            "src/libs/log/src/text_ostream_backend.cpp",
            "src/libs/log/src/thread_id.cpp",
            "src/libs/log/src/thread_specific.cpp",
            "src/libs/log/src/threadsafe_queue.cpp",
            "src/libs/log/src/*.hpp",
            "src/libs/program_options/src/*.cpp",
        ],
    ),
    copts = [
        "-Wno-c11-extensions",
        "-Wno-unused-local-typedef",
        "-Wno-deprecated-anon-enum-enum-conversion",
        "-Wno-deprecated-copy",
        "-Wno-deprecated-declarations",
        "-Wno-unknown-warning-option",
        "-Wno-shorten-64-to-32",
        "-Wno-implicit-fallthrough",
        "-Wno-#pragma-messages",
        "-Wno-unknown-warning-option",
    ],
    local_defines = ["BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF"],
    visibility = ["//visibility:public"],
    deps = [
        ":boost_thread",
    ],
)
