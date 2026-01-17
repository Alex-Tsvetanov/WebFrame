def get_suffixed_deps(deps, suffix):
    out = []
    for d in deps:
        label = native.package_relative_label(d)
        out.append(label.same_package_label(label.name + suffix))
    return out
