def _impl(ctx):
    out = ctx.actions.declare_file(ctx.attr.output)
    substitutions = dict(zip(ctx.attr.substitution_keys, ctx.attr.substitution_values))
    ctx.actions.expand_template(
        output = out,
        template = ctx.file.src,
        substitutions = substitutions,
    )
    return [DefaultInfo(files = depset([out]))]

_expand_template = rule(
    implementation = _impl,
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
        "output": attr.string(mandatory = True),
        "substitution_keys": attr.string_list(mandatory = True),
        "substitution_values": attr.string_list(mandatory = True),
    },
)

def expand_template(name, src, output, substitutions = dict(), select_based_substitutions = dict()):
    # this dance is necessary because select() doesn't get resolved when we
    # would like it to; see:
    # https://github.com/bazelbuild/bazel/issues/8171
    # https://github.com/bazelbuild/bazel/issues/3902#issuecomment-424934188
    # the solution is to split the keys and values into separate attrs,
    # so that we can use select's special treatment of '+', which allows
    # select to work directly at the rule invocation, and then zip them
    # back together inside the rule.

    substitution_keys = list(select_based_substitutions)
    substitution_values = list(select_based_substitutions.values())

    selected = []
    for select_dict in substitution_values:
        selected = selected + select(select_dict)

    _expand_template(
        name = name,
        src = src,
        output = output,
        substitution_keys = substitution_keys + list(substitutions),
        substitution_values = selected + list(substitutions.values()),
    )
