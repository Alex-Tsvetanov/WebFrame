def snap_client_ios_library(
        name,
        hdrs,
        srcs,
        deps = [],
        implementation_deps = [],
        tags = [],
        generated_objects = [],
        **kwargs):
    native.objc_library(
        name = name,
        deps = deps,
        implementation_deps = implementation_deps,
        srcs = srcs,
        hdrs = hdrs,
        tags = tags,
        **kwargs
    )
