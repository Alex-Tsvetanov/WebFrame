# Generates the AndroidManifest.xml for the Valdi module's kt_android_library target
def _generate_android_manifest_impl(ctx):
    out = ctx.actions.declare_file("AndroidManifest.xml")
    ctx.actions.expand_template(
        output = out,
        template = ctx.file._manifest_template,
        substitutions = {
            "{PACKAGE}": ctx.attr.package,
        },
    )
    return [DefaultInfo(files = depset([out]))]

generate_android_manifest = rule(
    implementation = _generate_android_manifest_impl,
    attrs = {
        "_manifest_template": attr.label(
            allow_single_file = True,
            default = "AndroidManifest.xml.tpl",
        ),
        "package": attr.string(),
    },
)
