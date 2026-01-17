def _rewrite_hdrs_impl(ctx):
    """
    Rewrites headers into a given module name
    """
    output_files = []
    transformer_exe = ctx.executable._transformer

    for header in ctx.files.srcs:
        out = ctx.actions.declare_file("output/{}".format(header.basename))

        args = [
            "rewrite_header",
            "-m",
            ctx.attr.module_name,
            "-i",
            header.path,
            "-o",
            out.path,
        ]

        if ctx.attr.flatten_paths:
            args.append("-f")

        for preserved_module_name in ctx.attr.preserved_module_names:
            args.append("-p")
            args.append(preserved_module_name)

        ctx.actions.run(
            inputs = [header, transformer_exe],
            outputs = [out],
            arguments = args,
            progress_message = "Transforming header: %s" % header.path,
            executable = transformer_exe,
        )

        output_files.append(out)

    return [
        DefaultInfo(
            files = depset(output_files),
        ),
    ]

rewrite_hdrs = rule(
    implementation = _rewrite_hdrs_impl,
    attrs = {
        "srcs": attr.label_list(
            cfg = "exec",
        ),
        "module_name": attr.string(),
        "preserved_module_names": attr.string_list(),
        "flatten_paths": attr.bool(),
        "_transformer": attr.label(
            executable = True,
            cfg = "exec",
            default = Label("@valdi_toolchain//:valdi_compiler_toolbox"),
        ),
    },
)
