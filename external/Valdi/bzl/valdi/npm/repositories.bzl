"Configure the npm dependencies used by valdi npm modules in the @valdi_npm repository"

load("@aspect_rules_js//npm:repositories.bzl", _npm_translate_lock = "npm_translate_lock")

def valdi_repositories():
    _npm_translate_lock(
        name = "valdi_npm",
        pnpm_lock = "@valdi//bzl/valdi/npm:pnpm-lock.yaml",
        lifecycle_hooks = {
            "better-sqlite3@9.6.0": [],
            "remotedebug-ios-webkit-adapter@0.4.2": [],
        },
        link_workspace = "valdi",
    )
