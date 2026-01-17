# Valdi NPM Module Dependencies

Configures the `@valdi` workspace's npm dependency tree using `aspect_rules_js`. The dependencies store configuration in the `@valdi_npm` repository (see [`link_workspace`][link_workspace]).

[link_workspace]: https://github.com/aspect-build/rules_js/blob/03bde54fd8e3de4c57e84247dffef9c450d956d5/docs/npm_translate_lock.md#npm_translate_lock-link_workspace

## Updating Dependencies

The `rules_js` macros use the PNPM dependency resolution system to manage npm dependencies.

Each package that needs its npm dependencies to be available in `bazel` will need to be listed in `pnpm-workspace.yaml`.

If the dependencies for one of those packages changes (e.g. `"express"` is added to `"dependencies":` in a `package.json`), the `pnpm-lock.yaml` file will need to be updated.

```
bzl run -- @pnpm//:pnpm --dir $PWD install --lockfile-only
```

## Using `@valdi` Bazel Workspace Dependencies

In order to use the `bazel` targets from `@valdi` from outside the `@valdi` workspace, it will be necessary to run the macros to set up the `npm` dependency configuration.

In another Bazel `WORKSPACE`:

```python
# Load the configuration macro from the @valdi workspace
load("@valdi//bzl/valdi/npm:repositories.bzl", "valdi_repositories")
# Sets up the @valdi_npm dependencies in their own namespace
valdi_repositories()

# Load npm_repositories macro for the @valdi_npm namespace
load("@valdi_npm//:repositories.bzl", valdi_npm_repositories = "npm_repositories")

# Call the repository macro that sets up the npm dependency tree
valdi_npm_repositories()
```