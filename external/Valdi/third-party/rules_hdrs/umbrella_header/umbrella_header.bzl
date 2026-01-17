def _umbrella_header_impl(ctx):
    actions = ctx.actions

    umbrella_header = actions.declare_file(
        "{umbrella_header_name}.h".format(
            umbrella_header_name = ctx.attr.umbrella_header_name,
        ),
    )

    content = actions.args()
    content.set_param_file_format("multiline")
    content.add_all(ctx.files.hdrs, format_each = '#import "%s"')
    actions.write(
        content = content,
        output = umbrella_header,
    )
    outputs = [umbrella_header]
    outputs_depset = depset(outputs)

    compilation_context = cc_common.create_compilation_context(
        headers = outputs_depset,
        direct_public_headers = outputs,
    )

    return [
        DefaultInfo(files = outputs_depset),
        CcInfo(compilation_context = compilation_context),
    ]

umbrella_header = rule(
    attrs = {
        "hdrs": attr.label_list(
            allow_files = True,
            doc = """
The list of C, C++, Objective-C, and Objective-C++ header files published by
this library to be included by sources in dependent rules.
""",
        ),
        "umbrella_header_name": attr.string(
            doc = "The name of the umbrella header.",
            mandatory = True,
        ),
    },
    doc = "Creates an umbrella header for an ObjC target.",
    implementation = _umbrella_header_impl,
)
