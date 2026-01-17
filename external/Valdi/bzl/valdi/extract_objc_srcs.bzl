load(":valdi_compiled.bzl", "ValdiModuleInfo")
load(":valdi_extract_output_rule_helper.bzl", "extract_valdi_output_rule")

# Extracts the generated sources directory tree artifact from the compiled_module target's providers and
# exposes it as using this target's `DefaultInfo` provider, so it can be consumed by the rule that generates
# the kt_android_library/objc_library target.
#
# NOTE: we might not need this if we start using named outputs for the iOS side of things, just like we do on android.
#       see comment inside valdi_compiled.bzl
def _extract_objc_srcs_impl(ctx):
    module = ctx.attr.compiled_module[ValdiModuleInfo]
    selected_source_set = ctx.attr.selected_source_set
    api_objc_srcs = None
    objc_srcs = None

    if selected_source_set == "debug":
        api_objc_srcs = module.ios_debug_api_generated_srcs
        objc_srcs = module.ios_debug_generated_srcs
    else:
        api_objc_srcs = module.ios_release_api_generated_srcs
        objc_srcs = module.ios_release_generated_srcs

    # NOTE: Some modules will not have generated Obj-C/Kotlin sources, but the
    #       Bazel rule has no way of knowing that ahead of time.
    #       If we try to configure an objc_library target without sources that will fail
    #       to build when using the objc_srcs.m hack (see below).
    #
    #       So, we add an empty .c implementation file just to make the build system happy.
    #
    #       We can make builds faster if the BUILD.bazel files specify some flags
    #       for modules that are guaranteed not to generate native sources.
    #       In those cases we could just not emit anything, including the objc_srcs.m hack
    empty_source_files = [] if ctx.attr.extension == ".h" else ctx.files._empty_source

    if ctx.attr.api_only:
        # objc_library rule expects the srcs to have certain extensions (.m, .h, etc.) So this hack add a .m extension
        # to the directory that contains the generated files to trigger processing all the files within that directory.
        hack_ios_dir = _prep_hack_ios_dir(
            ctx,
            selected_source_set,
            for_api = True,
            api_objc_srcs = api_objc_srcs,
            objc_srcs = objc_srcs,
        )

    else:
        hack_ios_dir = _prep_hack_ios_dir(
            ctx,
            selected_source_set,
            for_api = False,
            api_objc_srcs = api_objc_srcs,
            objc_srcs = objc_srcs,
        )

    return [
        DefaultInfo(
            files = depset(
                [hack_ios_dir] + empty_source_files,
            ),
        ),
    ]

def _prep_hack_ios_dir(ctx, selected_source_set, for_api, api_objc_srcs, objc_srcs):
    if for_api:
        input = api_objc_srcs
        output = ctx.actions.declare_directory("{}_api_objc_srcs{}".format(selected_source_set, ctx.attr.extension))
    else:
        input = objc_srcs
        output = ctx.actions.declare_directory("{}_objc_srcs{}".format(selected_source_set, ctx.attr.extension))

    if input:
        ctx.actions.run_shell(
            inputs = [input],
            outputs = [output],
            command = """
mkdir -p {output}; find -L {input} -type f -name "*{extension}" -exec cp -a {{}} {output} \\;
""".format(input = input.path, extension = ctx.attr.extension, output = output.path),
            mnemonic = "PrepValdiObjCSrcs",
        )
    else:
        # Generate empty directory if the module provides no sources
        ctx.actions.run_shell(
            inputs = [],
            outputs = [output],
            command = "mkdir -p {out}".format(
                out = output.path,
            ),
            mnemonic = "PrepValdiObjCDir",
        )
    return output

extract_objc_srcs = extract_valdi_output_rule(
    implementation = _extract_objc_srcs_impl,
    attrs = {
        "extension": attr.string(
            mandatory = True,
            values = [".m", ".h"],
        ),
        "api_only": attr.bool(
            mandatory = True,
        ),
        "selected_source_set": attr.string(
            mandatory = True,
        ),
        "_empty_source": attr.label(
            default = Label("//bzl/valdi:empty.c"),
            allow_single_file = True,
        ),
    },
)
