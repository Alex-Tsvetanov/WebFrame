# Boy, Bazel's way of defining custom bzl build flags is verbose...
ValdiModulesSourceSetProvider = provider(fields = ["source_set"])

values = [
    "debug",
    "release",
    "infer",
]

def _impl(ctx):
    raw_value = ctx.build_setting_value
    if raw_value not in values:
        fail(str(ctx.label) + " build setting allowed to take values {" +
             ", ".join(values) + "} but was set to unallowed value " +
             raw_value)
    return ValdiModulesSourceSetProvider(source_set = raw_value)

source_set = rule(
    implementation = _impl,
    build_setting = config.string(flag = True),
)
