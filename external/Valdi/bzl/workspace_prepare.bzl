load("//bzl:additional_dependencies.bzl", "setup_additional_dependencies")
load("//bzl:dependencies.bzl", "setup_dependencies")

def valdi_prepare_workspace(self_workspace_root = None):
    setup_dependencies(self_workspace_root)
    setup_additional_dependencies()
