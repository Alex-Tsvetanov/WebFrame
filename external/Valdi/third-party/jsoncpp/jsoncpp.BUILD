cc_library(
    name = "jsoncpp",
    srcs = [
        "src/lib_json/json_reader.cpp",
        "src/lib_json/json_tool.h",
        "src/lib_json/json_value.cpp",
        "src/lib_json/json_valueiterator.inl",
        "src/lib_json/json_writer.cpp",
    ],
    hdrs = [
        "include/json/allocator.h",
        "include/json/assertions.h",
        "include/json/config.h",
        "include/json/features.h",
        "include/json/forwards.h",
        "include/json/reader.h",
        "include/json/value.h",
        "include/json/version.h",
        "include/json/writer.h",
    ],
    copts = [
        "-Wno-implicit-fallthrough",
        "-Wno-unneeded-internal-declaration",
        "-Wno-deprecated-declarations",
    ],
    defines = ["JSON_USE_NULLREF=0"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
