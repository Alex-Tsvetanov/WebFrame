# The design of the android_library aar target is not the same as buck's.
# It doesn't create a monolithic package but instead create a single aar
# designed to be packaged with it's dependencies externally...i.e. we'd have
# to deploy packages for every single dependency to maven. Instead we
# package this aar monolithically using this custom rule. It takes a base AAR,
# we use valdi one since it's the only one with actual resources, and inject
# the native libraries and replace the class jar with a monolithic one.
# Probably a good idea to write a better rule for this in the future.

load("//bzl/android:platform_transition.bzl", "platform_transition")

# From developer.android.com/studio/projects/android-library#aar-contents.
AAR_KNOWN_FILES = [
    "classes.jar",
    "R.txt",
    "public.txt",
    "proguard.txt",
    "lint.jar",
    "api.jar",
]

AAR_KNOWN_DIRS = [
    "res",
    "assets",
    "libs",
    "prefab",
]

def _impl(ctx):
    dso_inputs = []
    cp_dsos_commands = []

    aar_dir = ctx.actions.declare_directory("{}_aar_dir".format(ctx.label.name))

    for arch, targets in ctx.split_attr.native_libs.items():
        for target in targets:
            dsos = target.default_runfiles.files.to_list()
            dso_inputs.extend(dsos)

            compilation_mode = getattr(ctx.fragments.cpp, "compilation_mode", "fastbuild")
            strip_cmd = "{strip} {input_file} -o {target_folder}/{basename}"
            native_lib_placement_cmd = strip_cmd
            if ctx.attr._strip_native_libs == "never" or compilation_mode == "dbg":
                native_lib_placement_cmd = "cp {input_file} {target_folder}/{basename}"

            copy_workflow = [
                "mkdir -p {target_folder}",
                native_lib_placement_cmd,
            ]

            if (ctx.attr.compress_dsos):
                copy_workflow.append(
                    "{compressor} {compression_args} " +
                    "-o {target_folder}/{compressed_basename} --rm " +
                    "{target_folder}/{basename}",
                )

            for f in dsos:
                target_folder = "{aar_dir}/jni/{arch}".format(
                    aar_dir = aar_dir.path,
                    arch = arch,
                )

                # This is temporary (so.zst would make much more sense, but
                # is used in phase 0 implementation).
                compressed_basename = f.basename.replace(".so", ".zst.so")

                cp_dsos_commands.append(" && ".join(copy_workflow).format(
                    target_folder = target_folder,
                    input_file = f.path,
                    basename = f.basename,
                    compressed_basename = compressed_basename,
                    strip = ctx.file._stripper.path,
                    compressor = ctx.executable._compressor.path,
                    compression_args = " ".join(ctx.attr.compression_args),
                ))

    inputs = [ctx.file.aar, ctx.file.classes_jar] + dso_inputs

    if ctx.file.proguard_spec:
        proguard_spec_command = "cp {proguard_spec} {output_dir}/proguard.txt".format(
            output_dir = aar_dir.path,
            proguard_spec = ctx.file.proguard_spec.path,
        )
        inputs.append(ctx.file.proguard_spec)
    else:
        proguard_spec_command = "echo 'no proguard spec specified'"

    # copy any additional files provided into the rule
    extra_cp_commands = []
    extra_mkdir_commands = []
    mkdirs = {}
    for additional_file, additional_file_output_path in ctx.attr.additional_files.items():
        additional_files = additional_file.files.to_list()
        if len(additional_files) > 1:
            fail("package_aar: additional_files item with output path {} has more than 1 item: {}"
                .format(additional_file_output_path, additional_file))

        if _is_known_aar_file(additional_file_output_path):
            fail("package_aar: additional_files item with output path {} is a known aar file: {}"
                .format(additional_file_output_path, additional_file))

        first_file = additional_files[0]
        inputs.append(first_file)

        if "/" in additional_file_output_path:
            mkdirs[additional_file_output_path.rsplit("/", 1)[0]] = True

        extra_cp_commands.append("cp {additional_file} {output_dir}/{output_path}".format(
            additional_file = first_file.path,
            output_dir = aar_dir.path,
            output_path = additional_file_output_path,
        ))

    mkdirs["libs"] = True
    for additional_jar in ctx.attr.additional_jars:
        additional_jars = additional_jar.files.to_list()

        first_file = additional_jars[0]
        inputs.append(first_file)

        extra_cp_commands.append("cp {additional_jar} {output_dir}/libs/".format(
            additional_jar = first_file.path,
            output_dir = aar_dir.path,
        ))

    mkdirs["assets"] = True
    for additional_asset in ctx.attr.additional_assets:
        additional_assets = additional_asset.files.to_list()

        for file in additional_assets:
            inputs.append(file)
            extra_cp_commands.append("unzip -qq {additional_asset} -d {output_dir}/assets/".format(
                additional_asset = file.path,
                output_dir = aar_dir.path,
            ))

    for d in mkdirs:
        extra_mkdir_commands.append("mkdir -p {output_dir}/{additional_file_dir}".format(
            output_dir = aar_dir.path,
            additional_file_dir = d,
        ))

    ctx.actions.run_shell(
        outputs = [aar_dir],
        inputs = inputs,
        tools = [ctx.executable._zipper, ctx.file._stripper, ctx.executable._compressor] + ctx.files._stripper_libs,
        command = """
set -euo pipefail
{zipper} x {input_aar} -d {output_dir}
cp {deploy_jar} {output_dir}/classes.jar
{proguard_spec_command}
{extra_mkdir_commands}
{extra_cp_commands}
{cp_commands}
""".format(
            zipper = ctx.executable._zipper.path,
            input_aar = ctx.file.aar.path,
            output_dir = aar_dir.path,
            deploy_jar = ctx.file.classes_jar.path,
            proguard_spec_command = proguard_spec_command,
            extra_mkdir_commands = "\n".join(extra_mkdir_commands),
            extra_cp_commands = "\n".join(extra_cp_commands),
            cp_commands = "\n".join(cp_dsos_commands),
        ),
    )

    output_aar_file = ctx.actions.declare_file("{name}.aar".format(name = ctx.attr.name))
    args = ctx.actions.args()
    args.add(ctx.executable._zipper.path)
    args.add(aar_dir.path)
    args.add(output_aar_file.path)
    args.add_all([aar_dir])

    ctx.actions.run(
        outputs = [output_aar_file],
        inputs = [aar_dir],
        executable = ctx.executable._zip_relative,
        tools = [ctx.executable._zipper],
        arguments = [args],
    )

    # Use an empty jar to satisfy the JavaInfo provider. We could use the
    # classes.jar from the original aar, but that would be a behavior change
    # since the rule currently does not propagate it and doing would mean
    # potentially invalidating caches when the classes.jar is changed.
    return [
        DefaultInfo(files = depset([output_aar_file])),
        JavaInfo(ctx.file._empty_jar, ctx.file._empty_jar),
    ]

def _is_known_aar_file(path):
    # disallow any file listed in developer.android.com/studio/projects/android-library#aar-contents.
    # Pushing files under those directories can create conflicts with preexisting files or other tools.
    if path in AAR_KNOWN_FILES:
        return True

    base_dir = path.split("/")[0]
    return base_dir in AAR_KNOWN_DIRS

# IMPORTANT NOTE: If you are renaming this, make sure to update the constant from the CLI
# in cli/src/core/constants.ts .
_package_aar_internal = rule(
    implementation = _impl,
    attrs = {
        "aar": attr.label(allow_single_file = True, mandatory = True),
        "platforms": attr.label_list(mandatory = True),
        "classes_jar": attr.label(allow_single_file = True, mandatory = True),
        "native_libs": attr.label_list(
            cfg = platform_transition,
            providers = [],
        ),

        # This is an internal private attr overriden by the the rule macro.
        # It is a hack in order to make the native libraries visible to the ijwb aspect
        # which is not aware of the `native_libs` attr so it does not propagate through it.
        "deps": attr.label_list(
            cfg = platform_transition,
            providers = [],
            mandatory = False,
        ),
        "proguard_spec": attr.label(
            default = None,
            mandatory = False,
            allow_single_file = True,
        ),
        "compress_dsos": attr.bool(
            default = False,
            doc = "Whether or not DSO files should be compressed in the generated AAR.",
            mandatory = False,
        ),
        "compression_args": attr.string_list(
            default = ["-f", "--ultra", "-22"],
            doc = "Settings for compression tool.",
            mandatory = False,
            allow_empty = True,
        ),
        "_strip_native_libs": attr.string(
            default = "always",
            doc = "Whether or not native libraries should be stripped of debug symbols in the generated AAR.",
            mandatory = False,
        ),
        "additional_files": attr.label_keyed_string_dict(
            mandatory = False,
            default = {},
            allow_files = True,
            doc = (
                "Additional files to package in the aar. " +
                "Note that any file listed in developer.android.com/studio/projects/android-library#aar-contents " +
                "is explicitly disallowd to avoid conflicts with other tools."
            ),
        ),
        "additional_jars": attr.label_list(),
        "additional_assets": attr.label_list(),
        "_zip_relative": attr.label(
            default = "//bzl/android:zip_relative",
            executable = True,
            cfg = "exec",
        ),
        "_zipper": attr.label(
            default = "@bazel_tools//tools/zip:zipper",
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        # This is bad form, and regrettably fragile; it means that we have to touch
        # this anytime we want to update the toolchain.  More elegant solutions might be:
        # 1) We could depend directly on the "<target>.stripped" output.  If we
        #    pursue that path, remember also to set --stripopts; the default strip
        #    settings are not right for us.
        # 2) We could perhaps get strip from the toolchain indirectly, so that
        #    we would always have the right stripper for the active toolchain.  One
        #    example is https://github.com/bazelbuild/rules_cc/blob/master/examples/my_c_archive/my_c_archive.bzl,
        #    except that it doesn't handle the transition - we want the android stripper,
        #    not the host's.
        "_stripper": attr.label(
            default = "@snap_client_toolchains//:llvm_ndk_28_0_13004108_strip",
            allow_single_file = True,
        ),
        "_stripper_libs": attr.label(
            default = "@snap_client_toolchains//:llvm_ndk_28_0_13004108_strip_libs",
            allow_files = True,
        ),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
        "_compressor": attr.label(
            default = "//tools/zstd:zstd",
            executable = True,
            cfg = "exec",
        ),
        "_empty_jar": attr.label(
            default = "//bzl/android:empty_jar",
            allow_single_file = True,
        ),
    },
    fragments = ["cpp"],
)

def package_aar(**kwargs):
    if "deps" in kwargs:
        fail("The deps attribute in package_aar should not be used directly.")
    patched_kwargs = dict(**kwargs)
    patched_kwargs["$strip_native_libs"] = select({
        "@valdi//bzl/conditions:strip_always": "always",
        "@valdi//bzl/conditions:strip_never": "never",
        "@valdi//bzl/conditions:strip_sometimes": "sometimes",
        "//conditions:default": "always",
    })
    patched_kwargs["deps"] = kwargs.get("native_libs", [])
    _package_aar_internal(
        **patched_kwargs
    )
