def _modulemap_command(modulemap, module_name):
    """ Generate a shell script to create modulemap.

        The modulemap format is specified at: https://clang.llvm.org/docs/Modules.html#header-declaration

        Example:
        module {ModuleName} {
            header "header1.h"
            header "header2.h"
            export *
        }
    """

    # A header path inside a modulemap should be relative to the modulemap file
    slashes_count = modulemap.path.count("/")
    relative_path = "".join(["../"] * slashes_count)

    command = """
echo "module {module_name} {{" >> "{output_file}"

for arg in "$@"; do
    if [[ "$arg" == *.h ]]; then
        echo "  header \\"{relative_path}$arg\\"" >> "{output_file}"
    fi
done

echo "  export *" >> "{output_file}"
echo "}}" >> "{output_file}"

""".format(
        module_name = module_name,
        output_file = modulemap.path,
        relative_path = relative_path,
    )

    return command

def _modulemap_impl(ctx):
    modulemap = ctx.outputs.out

    # Collect all header files and folders
    inputs = []
    for hdr in ctx.attr.hdrs:
        for f in hdr.files.to_list():
            # Unfortunately Bazel rules don't support label_list which would allow files and folders filters
            # Filter here explicitly to mimic the behavior
            if f.is_directory or f.basename.endswith(".h"):
                inputs.append(f)

    inputs = depset(inputs).to_list()

    # Pass them as arguments to the generated shell command
    args = ctx.actions.args()
    args.add_all(inputs, expand_directories = True)
    ctx.actions.run_shell(
        command = _modulemap_command(modulemap, ctx.attr.module_name),
        arguments = [args],
        inputs = inputs,
        outputs = [modulemap],
        mnemonic = "ModuleMapGen",
        progress_message = "Generating modulemap for {}".format(ctx.attr.module_name),
    )

    objc_provider = apple_common.new_objc_provider(
        module_map = depset([modulemap]),
    )
    outputs = [modulemap]
    compilation_context = cc_common.create_compilation_context(
        headers = depset(transitive = [depset(outputs)]),
    )
    cc_info = CcInfo(
        compilation_context = compilation_context,
    )

    return [
        DefaultInfo(
            files = depset([modulemap]),
        ),
        objc_provider,
        cc_info,
    ]

_modulemap = rule(
    implementation = _modulemap_impl,
    attrs = {
        "module_name": attr.string(
            mandatory = True,
            doc = "The name of the module.",
        ),
        "hdrs": attr.label_list(
            allow_files = True,
            doc = "The list Objective-C header files used to construct the module map.",
        ),
        "out": attr.output(
            doc = "The name of the output module map file.",
        ),
    },
)

def modulemap(name, **kwargs):
    _modulemap(
        name = name,
        out = name + ".modulemap",
        **kwargs
    )
