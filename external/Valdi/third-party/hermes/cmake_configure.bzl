load("//bzl:expand_template.bzl", "expand_template")

# Replicates cmake's configure_file function.
# 'cmakedefines' takes a dictionary containing the #cmakedefines to enable, where
# each key is the list of conditions for which the #cmakedefines should be enabled.
# 'cmake_vars' takes a dictionary with the list of cmake variables (${}) to replace, where
# the value is a dictionary containing all the conditions associated with their replacement
# values.
# 'cmakedefines_01' is similar to 'cmakedefines', but will instead replace #cmakedefines01 macros
# by either a 1 or 0 define if the condition is true.
def cmake_configure(name, src, output, cmakedefines = {}, cmake_vars = {}, cmakedefines_01 = {}):
    all_select_substitutions = {}

    for key in cmakedefines:
        value = cmakedefines[key]
        select_param = {}
        has_default = False
        for condition in value:
            if condition == "//conditions:default":
                has_default = True
            select_param[condition] = ["#define {}".format(key)]
        if not has_default:
            select_param["//conditions:default"] = ["// #undef {}".format(key)]
        all_select_substitutions["#cmakedefine {}".format(key)] = select_param

    for key in cmake_vars:
        value = cmake_vars[key]
        select_param = {}
        for condition in value:
            select_param[condition] = [value[condition]]
        all_select_substitutions["${{{}}}".format(key)] = select_param

    for key in cmakedefines_01:
        value = cmakedefines_01[key]
        select_param = {}
        has_default = False
        for condition in value:
            if condition == "//conditions:default":
                has_default = True
            select_param[condition] = ["#define {} 1".format(key)]
        if not has_default:
            select_param["//conditions:default"] = ["#define {} 0".format(key)]
        all_select_substitutions["#cmakedefine01 {}".format(key)] = select_param

    expand_template_name = "expand_template-{}".format(name)

    expand_template(
        name = expand_template_name,
        src = src,
        output = output,
        select_based_substitutions = all_select_substitutions,
    )

    native.cc_library(
        name = name,
        hdrs = [":{}".format(expand_template_name)],
        includes = ["."],
    )
