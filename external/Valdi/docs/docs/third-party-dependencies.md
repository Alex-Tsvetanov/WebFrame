# Third Party dependencies

## Introduction

Valdi modules can be published so that other users can leverage them. This process is currently a bit more complicated that we'd like, and we are working on making it better.

## Consuming a third-party dependency

Adding a dependency is usually done in three steps:
- Make the dependency available within the Bazel Workspace.
- Configure the dependency (optional, depends on whether the dependency requires additional configuration or dependencies)
- Reference the dependency in one of your Valdi module so that you can make use of it.

You make a dependency available in Bazel by adding a `git_repository()` or `http_archive()` rule that points to the dependency within your `WORKSPACE` file. The library owner should provide instructions on what to add in your `WORKSPACE`. Here is an example on how it can look like:
```python
# Make valdi_widgets available through @valdi_widgets in Bazel build files
git_repository(
    name = "valdi_widgets",
    commit = "465111c7ae19a6da5bd610139f873e730052b042",
    remote = "git@github.com:Snapchat/Valdi_Widgets.git",
)
```
If the dependency requires some additional configuration, you might need to add some additional instructions in your `WORKSPACE` file. Here is an example:
```python
# Load the setup function from our Valdi widgets repository
load("@valdi_widgets//:setup.bzl", "setup_valdi_widgets")
# Invoke it
setup_valdi_widgets()
```

Lastly, add a dependency in the Valdi module which wants to access that library:
```python
valdi_module(
    name = "my_module",
    srcs = glob([
        "src/**/*.ts",
        "src/**/*.tsx",
    ]),
    deps = [
        "@valdi//src/valdi_modules/src/valdi/valdi_core",
        "@valdi//src/valdi_modules/src/valdi/valdi_tsx",
        # We now depend on valdi_modules/widgets from the valdi_widgets repository
        "@valdi_widgets//valdi_modules/widgets",
    ]
)
```
Please note that the target (`@valdi_widgets//valdi_modules/widgets` in this example) must be a `valdi_module()`.
You can consume Android specific targets using `android_deps`, iOS specific targets using `ios_deps`, C++ targets using `native_deps`.

> **See also:** [Valdi Module](./core-module.md) for complete BUILD.bazel configuration options.

Once the dependency is added in your `valdi_module()`, run `valdi projectsync` to fetch it and get autocompletion in VSCode.

## Publishing a library

Valdi libraries made available to external developers should have the following:
- A `WORKSPACE` copy-pastable content that developers can use to reference your library.
- The list of Valdi module targets exported as part of your library. They should have `visibility = ["//visibility:public"]` so that the targets are visible.

Please use a workspace name that is unique and is unlikely to conflict with other workspaces.
Published libraries can be backed by native code, or be pure TypeScript code. If they are backed by native code, the native code should be built automatically when the module is included. Please follow the instructions on [Polyglot Modules](./native-polyglot.md) if you need further instructions about embedding native code in your module.

## See Also

- [Valdi Module](./core-module.md) - Module configuration and BUILD.bazel options
- [Polyglot Modules](./native-polyglot.md) - Embedding native code in modules
- [Valdi rules for Bazel](./workflow-bazel.md) - Bazel build system integration
