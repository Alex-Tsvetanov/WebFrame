COMPILER_FLAGS = [
    "-Os",  # Uncomment to enable optimizations
    "-Wno-unused-label",
    "-Wno-unused-variable",
    "-Wno-unused-but-set-variable",
    "-fno-exceptions",
]

def tsn_library(name, srcs, deps = [], visibility = None):
    name_compiled = "{}_compiled".format(name)
    native.genrule(
        name = name_compiled,
        srcs = srcs,
        outs = ["compiled_{}.c".format(name)],
        cmd = "$(location //tsn:scripts/compile_single.sh) -s {} -b $(BINDIR) -c $(location //compiler/companion:bin_wrapper) -o $@ $(SRCS)".format(native.package_name()),
        tools = [
            "//tsn:scripts/compile_single.sh",
            "//compiler/companion:bin_wrapper",
        ],
    )

    native.cc_library(
        name = name,
        hdrs = [],
        includes = [],
        copts = COMPILER_FLAGS,
        alwayslink = 1,
        srcs = [":{}".format(name_compiled)],
        deps = ["//tsn:tsn"] + deps,
        visibility = visibility,
    )

def tsn_binary(name, entry_points, deps = [], srcs = None, visibility = None):
    resolved_deps = [d for d in deps]
    if srcs:
        lib_name = "{}_lib".format(name)
        tsn_library(lib_name, srcs)
        resolved_deps.append(":{}".format(lib_name))

    native.cc_binary(
        name = name,
        args = entry_points,
        deps = ["//tsn:main"] + resolved_deps,
        visibility = visibility,
    )
