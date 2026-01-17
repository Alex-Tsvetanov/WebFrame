def _dest(rel):
    if "web_native" in rel:
        return "native"

    if "protodecl_collapsed" in rel:
        return "src"

    prefix = "src/valdi_modules/src/valdi/"
    if rel.startswith(prefix):
        rel2 = rel[len(prefix):]
    else:
        rel2 = rel

    parts = rel2.split("/")
    for i in range(len(parts) - 3):
        if (parts[i + 1] == "web" and
            parts[i + 2] in ["debug", "release"] and
            parts[i + 3] in ["assets", "res"]):
            tail = "/".join(parts[i + 4:])
            return "src/{}".format(tail)
    return rel

def _impl(ctx):
    outdir = ctx.actions.declare_directory(ctx.label.name)

    # build a small manifest of src → dest
    manifest = ctx.actions.declare_file(ctx.label.name + ".manifest")
    lines = []
    for f in ctx.files.srcs:
        lines.append("{}\t{}".format(f.path, _dest(f.short_path)))
    ctx.actions.write(manifest, "\n".join(lines))

    # tiny shell copier
    sh = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.write(
        output = sh,
        is_executable = True,
        content = """#!/usr/bin/env bash
        set -euo pipefail
        OUT="$1"; MAN="$2"
        rm -rf "$OUT"; mkdir -p "$OUT"

        while IFS=$'\\t' read -r SRC DEST; do
        [ -z "$SRC" ] && continue

        # If SRC is a directory (tree artifact), copy its *contents* into DEST
        if [ -d "$SRC" ]; then
            mkdir -p "$OUT/$DEST"
            # copy contents, not the top-level dir
            cp -R "$SRC/." "$OUT/$DEST/"
        else
            D="$OUT/$(dirname "$DEST")"
            mkdir -p "$D"
            cp -f "$SRC" "$OUT/$DEST"
        fi
        done < "$MAN"
        """,
    )

    ctx.actions.run(
        inputs = [manifest] + ctx.files.srcs,
        outputs = [outdir],
        tools = [sh],
        executable = sh,
        arguments = [outdir.path, manifest.path],
        progress_message = "Collapsing web paths into {}".format(outdir.path),
    )
    return [DefaultInfo(files = depset([outdir]))]

collapse_web_paths = rule(
    implementation = _impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
    },
)

def _dest_native(rel):
    parts = rel.split("/")

    # 2) Keep "<parent>/web/<tail>" where "web" is the marker
    for i, seg in enumerate(parts):
        if seg == "web":
            parent = parts[i - 1] if i > 0 else ""
            tail = "/".join(parts[i + 1:])
            base = (parent + "/web") if parent else "web"
            return base + ("/" + tail if tail else "")

    # 3) If there's no "web" segment, just return the path
    return "/".join(parts)

def _impl_native(ctx):
    outdir = ctx.actions.declare_directory(ctx.label.name)

    # Build a manifest of: SRC \t DEST
    manifest = ctx.actions.declare_file(ctx.label.name + ".manifest")
    lines = []
    for f in ctx.files.srcs:
        lines.append("{}\t{}".format(f.path, _dest_native(f.short_path)))
    ctx.actions.write(manifest, "\n".join(lines))

    # Tiny shell that copies into the declared directory
    sh = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.write(
        output = sh,
        is_executable = True,
        content = """#!/usr/bin/env bash
            set -euo pipefail
            OUT="$1"; MAN="$2"
            rm -rf "$OUT"; mkdir -p "$OUT"
            while IFS=$'\\t' read -r SRC DEST; do
            [ -z "$SRC" ] && continue
            D="$OUT/$(dirname "$DEST")"
            mkdir -p "$D"
            cp -rf "$SRC" "$OUT/$DEST"
            done < "$MAN"
        """,
    )

    ctx.actions.run(
        inputs = [manifest] + ctx.files.srcs,
        outputs = [outdir],
        tools = [sh],
        executable = sh,
        arguments = [outdir.path, manifest.path],
        progress_message = "Collapsing native paths into {}".format(outdir.path),
    )
    return [DefaultInfo(files = depset([outdir]))]

collapse_native_paths = rule(
    implementation = _impl_native,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
    },
)
