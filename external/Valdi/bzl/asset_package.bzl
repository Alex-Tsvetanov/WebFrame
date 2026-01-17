# asset_package can be used to bundle arbitary assets as Valdi modules.
# Package's `data` will be packaged as `clien/res` files in the Valdi archive. Applies to unlinked (archived) Valdi builds only.
# It's meant to be used as a dep module of client_objc_library, cc_library, or any other module type supported by package_ios.bzl.
#
# asset_package(
#     name = "module_assets",
#     data = [...],
# )
#
# client_objc_library(
#   ...,
#   deps = [":module_assets"]
# )

AssetPackage = provider(fields = ["data", "deps"])

def _asset_package_impl(ctx):
    compilation_context = cc_common.create_compilation_context()
    cc_info = CcInfo(compilation_context = compilation_context)

    return [
        AssetPackage(data = depset(ctx.files.data), deps = depset()),
        cc_info,
    ]

asset_package = rule(
    implementation = _asset_package_impl,
    attrs = {
        "data": attr.label_list(allow_files = True),
    },
)
