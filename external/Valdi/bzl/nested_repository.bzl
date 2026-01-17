def _nested_repository_impl(ctx):
    resolved_target_dir = "../{}/{}".format(ctx.attr.source_repo, ctx.attr.target_dir)

    res = ctx.execute([
        "sh",
        "-c",
        "ls -1 {}".format(resolved_target_dir),
    ])
    if res.return_code != 0:
        fail("Failed to resolve files in target dir {}: {}".format(resolved_target_dir, res.stderr))

    file_list = res.stdout.strip().split("\n")

    for f in file_list:
        ctx.symlink("{}/{}".format(resolved_target_dir, f), f)

nested_repository = repository_rule(
    implementation = _nested_repository_impl,
    attrs = {
        "source_repo": attr.string(mandatory = True),
        "target_dir": attr.string(mandatory = True),
    },
)
