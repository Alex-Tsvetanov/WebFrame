load("@bazel_skylib//lib:selects.bzl", bzl_selects = "selects")
load(":custom_selects.bzl", "custom_selects")

def _config_setting_combinations(
        prefix = "",
        settings = [],
        constants = [],
        visibility = None):
    """Creates a config_setting_group for every combination of the provided config_settings.

    The provided settings are a list of dictionaries where keys are config_settings and values
    are "tags". Each dictionary forms a group of mutually-exclusive config_settings. One
    config_setting is chosen from each dictionary to AND together a config_setting_group. In this
    way, this function creates a config_setting_group for every possible combination of ANDing
    together one config_setting from each dictionary. The name for each resulting
    config_setting_group is the concatenation of all of the "tags" mapped to by the config_settings
    that were selected to form it, in the order in which they appear in the list of dictionaries.
    If a prefix is provided, the prefix is prepended to each name.

    For simplicity, a dictionary is allowed to contain only 1 config_setting/tag pair. This implies
    a dictionary of size 2 where the second element is the logical inverse of the first, and the
    "tag" is empty, meaning when the inverse is selected, it does not contributed to the resulting
    config_setting_group name.

    Example 1:
      config_setting_combinations(
          [
              {
                  "@snap_platforms//conditions:android_arm32": "android_arm32",
                  "@snap_platforms//conditions:android_arm64": "android_arm64",
                  "@snap_platforms//conditions:ios": "ios",
              },
              {
                  "@snap_client_toolchains//:sc_build_flag_lto": "lto",
              },
          ],
          prefix = "example1"
      )

      creates the following config_setting_groups:

      ":example1_android_arm32_lto"
      ":example1_android_arm32"
      ":example1_android_arm64_lto"
      ":example1_android_arm64"
      ":example1_ios_lto"
      ":example1_ios"

    Example 2:
      config_setting_combinations(
          [
              {
                  "@snap_client_toolchains//:sc_build_flag_lto": "lto",
              },
              {
                  "//bzl/conditions/grpc:tracers_disabled": "tracers_off",
              },
          ],
          prefix = "example2"
      )

      creates the following config_setting_groups:

      ":example2_lto_tracers_off"
      ":example2_lto"
      ":example2_tracers_off"
      ":example2"

    Args:
      prefix: The prefix to prepend to each resulting config_setting_group name.
      settings: A list of dictionaries each representing a group of mutually-exclusive configs,
        mapping config_setting to the "tag" that should appear in the config_setting_group names
        that it is used to construct.
      constants: A list of config_settings that are ANDed into every resulting config_setting_group
        combination.
      visibility: The visibility of the resulting config_setting_groups.
      """
    if not settings:
        fail("config_setting_combinations: the settings list cannot be empty.")
    name = prefix
    selections = [0] * len(settings)
    num_combinations = 1
    for setting_group in settings:
        # If there is only 1 config_setting in the group, add an inverse config_setting with empty tag.
        if not setting_group:
            fail("config_setting_combinations: the settings list cannot contain empty dicts.")
        if len(setting_group) == 1:
            setting = setting_group.keys()[0]
            scratch_inverse_name = setting.split(":")[1] + "_inverse" + \
                                   "".join([str(len(dict)) for dict in settings]) + prefix
            custom_selects.config_setting_inverse(
                scratch_inverse_name,
                inverse_of = setting,
                visibility = visibility,
            )
            setting_group[":" + scratch_inverse_name] = ""
        num_combinations *= len(setting_group)

    for i in range(num_combinations):
        _config_setting_combination(prefix, settings, selections, constants, visibility)
        _increment_selections(selections, settings)

def _increment_selections(selections, settings):
    for i in reversed(range(len(selections))):
        num_options = len(settings[i]) if len(settings[i]) > 1 else 2
        if selections[i] == num_options - 1:
            selections[i] = 0
        else:
            selections[i] += 1
            return

def _config_setting_combination(prefix, settings, selections, constants, visibility):
    name = prefix
    applied_settings = list(constants)
    for i in range(len(settings)):
        setting = settings[i].keys()[selections[i]]
        tag = settings[i].values()[selections[i]]
        name += ("_" if name != "" and tag != "" else "") + tag
        applied_settings.append(setting)
    bzl_selects.config_setting_group(
        name,
        match_all = applied_settings,
        visibility = visibility,
    )

selects = struct(
    config_setting_inverse = custom_selects.config_setting_inverse,
    config_setting_combinations = _config_setting_combinations,
    # re-exports bazel-skylib's selects
    with_or = bzl_selects.with_or,
    with_or_dict = bzl_selects.with_or_dict,
    config_setting_group = bzl_selects.config_setting_group,
)
