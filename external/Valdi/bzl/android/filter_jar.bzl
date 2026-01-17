# filter_jar.bzl
def _filter_jar_impl(ctx):
    in_jar = ctx.file.jar
    out_jar = ctx.actions.declare_file(ctx.label.name + ".jar")
    pattern_f = ctx.actions.declare_file(ctx.label.name + "_patterns.txt")
    scratch = ctx.actions.declare_directory("{}_tempdir".format(ctx.label.name))

    # 1. Write the user-supplied regex list to a file (one per line)
    ctx.actions.write(
        output = pattern_f,
        content = "\n".join(ctx.attr.excluded_class_path_patterns),
        is_executable = False,
    )

    # 2. Shell action that:
    #    a) unzips the JAR to a temp dir
    #    b) deletes every .class whose path matches any regex
    #    c) re-zips the remainder into the output JAR
    #       (zip -q ensures deterministic order)
    ctx.actions.run_shell(
        inputs = [in_jar, pattern_f],
        outputs = [out_jar, scratch],
        command = """
set -euo pipefail
tmp="$4"

# Expand the input jar
unzip -qq "$1" -d "$tmp"

chmod -R u+rwX "$tmp"

# Build a single alternation pattern:  (rx1)|(rx2)|...
pat=$(paste -sd'|' "$2")

# Remove .class files whose INTERNAL path matches the regex
# (grep works on "/" paths; users can write com/android/.* or com\\.android\\..*)
find "$tmp" -type f | grep -E "$pat" | xargs -r rm -f
find "$tmp" -depth -type d -empty -exec rmdir -- "{}" +

# Re-create the jar
abs_dest="$PWD/$3"
echo $abs_dest
(cd "$tmp" && zip -qqr "$abs_dest" .)
""",
        arguments = [
            in_jar.path,  # $1
            pattern_f.path,  # $2
            out_jar.path,  # $3
            scratch.path,  # $4
        ],
        mnemonic = "FilterJar",
        progress_message = "Filtering JAR %s" % (in_jar.basename),
    )

    return [DefaultInfo(files = depset([out_jar]))]

filter_jar = rule(
    implementation = _filter_jar_impl,
    attrs = {
        "jar": attr.label(
            allow_single_file = [".jar"],
            doc = "Input JAR to be filtered",
            mandatory = True,
        ),
        "excluded_class_path_patterns": attr.string_list(
            doc = "Regexes (applied to internal /.class paths) to exclude",
            mandatory = True,
        ),
    },
    doc = "Produces a JAR with classes **not** matching any excluded_class_path_patterns.",
)
