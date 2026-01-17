load(
    "@rules_android//rules:rules.bzl",
    _aar_import = "aar_import",
    _android_binary = "android_binary",
    _android_library = "android_library",
)

# TODO (yhuang6): The non native methods are not able to compile/link Theme.AppCompat.Light.NoActionBar
STARLARK_RULES_ANDROID_ENABLED = True

def android_binary(**kwargs):
    if STARLARK_RULES_ANDROID_ENABLED:
        _android_binary(**kwargs)
        return
    native.android_binary(**kwargs)

def android_library(**kwargs):
    if STARLARK_RULES_ANDROID_ENABLED:
        _android_library(**kwargs)
        return
    native.android_library(**kwargs)

def aar_import(**kwargs):
    patched_kwargs = dict(kwargs)
    existing = patched_kwargs.get("deps", [])
    patched_kwargs["deps"] = existing + [
        "@rules_kotlin//kotlin/compiler:kotlin-stdlib",
    ]

    if STARLARK_RULES_ANDROID_ENABLED:
        _aar_import(**patched_kwargs)
        return
    native.aar_import(**patched_kwargs)
