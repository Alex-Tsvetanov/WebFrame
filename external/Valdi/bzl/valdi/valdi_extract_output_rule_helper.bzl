load(":valdi_compiled.bzl", "ValdiModuleInfo")

def extract_valdi_output_rule(implementation, attrs):
    # TODO(simon): We use cfg = "exec" to avoid compiling multiple
    # times for each platform as Valdi compilation is already cross-platform.
    # This eliminates the need for compiling a Valdi module 3 times on Android:
    # once for the Kotlin output, once for C output meant to compile for arm64,
    # once for C output meant to compile for armv7. It might be better to use a
    # Bazel transition instead at some point.

    attrs["compiled_module"] = attr.label(
        mandatory = True,
        cfg = "exec",
        providers = [ValdiModuleInfo],
    )

    return rule(implementation = implementation, attrs = attrs)
