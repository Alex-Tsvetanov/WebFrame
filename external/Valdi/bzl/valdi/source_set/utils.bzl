# Valdi Modules-specific helper functions for configuring a Starlark select({...}) with string values
# that include a {source_set} format placeholder.
#
# Additional format substitutions should be provided via the `substitutions` parameter
#
# For example:
# _source_set_pattern([ "{source_set}/some_path/to/file" ]) will configure a select that picks
# the formatted path with {source_set} replaced by the currently configured Valdi Modules source set.
# E.g. if current Bazel build/run invocation is configured for the 'debug' source set, the selected
# value would be "debug/some_path/to/file"
def source_set_patterns(pattern_formats, should_glob, exclude = [], release_exclude = [], **substitutions):
    debug_patterns = []
    release_patterns = []
    for pattern_format in pattern_formats:
        debug_pattern = pattern_format.format(source_set = "debug", **substitutions)
        debug_patterns.append(debug_pattern)
        release_pattern = pattern_format.format(source_set = "release", **substitutions)
        release_patterns.append(release_pattern)

    if should_glob:
        debug_patterns = native.glob(debug_patterns, exclude = exclude)
        release_patterns = native.glob(release_patterns, exclude = exclude + release_exclude)

    return source_set_select(
        debug = debug_patterns,
        release = release_patterns,
    )

# Convenience around source_set_patterns allowing it to be a drop-in replacement for native.glob
def source_set_glob(pattern_formats, exclude = [], release_exclude = [], **substitutions):
    return source_set_patterns(pattern_formats, should_glob = True, exclude = exclude, release_exclude = release_exclude, **substitutions)

# Helper function wrapping Starlark select(...)
def source_set_select(debug, release):
    return select({
        "@valdi//bzl/valdi/source_set:debug": debug,
        "@valdi//bzl/valdi/source_set:release": release,
        "@valdi//bzl/valdi/source_set:infer_debug": debug,
        "@valdi//bzl/valdi/source_set:infer_release": release,
    })
