load("//bzl/valdi:suffixed_deps.bzl", "get_suffixed_deps")
load("//bzl/valdi:valdi_android_application.bzl", "valdi_android_application")
load("//bzl/valdi:valdi_ios_application.bzl", "valdi_ios_application")
load("//bzl/valdi:valdi_macos_application.bzl", "valdi_macos_application")
load("//bzl/valdi:valdi_module.bzl", "valdi_hotreload")

def valdi_application(
        name,
        title,
        root_component_path,
        ios_bundle_id = None,
        ios_info_plist = None,
        ios_families = None,
        ios_minimum_os_version = None,
        ios_provisioning_profile = None,
        ios_app_icons = None,
        android_package = None,
        android_assets = None,
        android_assets_dir = None,
        android_resource_files = None,
        android_app_manifest = None,
        android_app_icon_name = None,
        android_round_app_icon_name = None,
        android_activity_theme_name = None,
        desktop_window_width = 600,
        desktop_window_height = 800,
        desktop_window_resizable = True,
        version = None,
        deps = []):
    resolved_ios_bundle_id = ios_bundle_id if ios_bundle_id else "com.snap.valdi.{}".format(name)
    resolved_android_package = android_package if android_package else "com.snap.valdi.{}".format(name)

    valdi_ios_application(
        name = "{}_ios".format(name),
        title = title,
        root_component_path = root_component_path,
        bundle_id = resolved_ios_bundle_id,
        info_plist = ios_info_plist,
        families = ios_families,
        minimum_os_version = ios_minimum_os_version,
        provisioning_profile = ios_provisioning_profile,
        app_icons = ios_app_icons,
        version = version,
        deps = get_suffixed_deps(deps, "_objc"),
    )

    valdi_android_application(
        name = "{}_android".format(name),
        title = title,
        root_component_path = root_component_path,
        package = resolved_android_package,
        assets = android_assets,
        assets_dir = android_assets_dir,
        app_manifest = android_app_manifest,
        resource_files = android_resource_files,
        icon_name = android_app_icon_name,
        round_icon_name = android_round_app_icon_name,
        activity_theme_name = android_activity_theme_name,
        deps = get_suffixed_deps(deps, "_kt"),
        native_deps = get_suffixed_deps(deps, "_native"),
    )

    valdi_macos_application(
        name = "{}_macos".format(name),
        title = title,
        root_component_path = root_component_path,
        bundle_id = resolved_ios_bundle_id,
        window_width = desktop_window_width,
        window_height = desktop_window_height,
        window_resizable = desktop_window_resizable,
        deps = get_suffixed_deps(deps, "_native"),
    )

    valdi_hotreload(
        name = "{}_hotreload".format(name),
        targets = deps,
        tags = ["valdi_application"],
    )
