def _config_setting_inverse(name, inverse_of, visibility = None):
    """Returns the logical inverse of the provided config_setting."""
    native.alias(
        name = name,
        actual = select({
            inverse_of: "@valdi//bzl/constants:false",
            "//conditions:default": "@valdi//bzl/constants:true",
        }),
        visibility = visibility,
    )

custom_selects = struct(
    config_setting_inverse = _config_setting_inverse,
)
