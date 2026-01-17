def android_aar_platforms():
    return select({
        "@snap_platforms//conditions:client_repo_arm64": ["@snap_platforms//os:android_arm64"],
        "//conditions:default": [],
    }) + select({
        "@snap_platforms//conditions:client_repo_x86_64": ["@snap_platforms//os:android_x86_64"],
        "//conditions:default": [],
    }) + select({
        "@snap_platforms//conditions:client_repo_arm32": ["@snap_platforms//os:android_arm32"],
        "//conditions:default": [],
    })

def _impl(settings, attr):
    _ignore = settings

    result = {}

    cpus = {
        Label("@snap_platforms//os:android_arm32"): "armeabi-v7a",
        Label("@snap_platforms//os:android_arm64"): "arm64-v8a",
        Label("@snap_platforms//os:android_x86_64"): "x86_64",
    }

    for platform in attr.platforms:
        result[cpus[platform]] = {
            "//command_line_option:platforms": [platform],
        }

    return result

# Transition for the native libs into desired arches
platform_transition = transition(
    implementation = _impl,
    inputs = [],
    outputs = [
        "//command_line_option:platforms",
    ],
)
