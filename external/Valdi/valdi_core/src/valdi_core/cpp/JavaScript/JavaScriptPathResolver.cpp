//
//  JavaScriptPathResolver.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/8/18.
//

#include "valdi_core/cpp/JavaScript/JavaScriptPathResolver.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <boost/functional/hash.hpp>

namespace Valdi {

Result<ResourceId> pathToResourceId(Path importPath) {
    importPath.normalize();

    if (importPath.empty()) {
        return Error(STRING_FORMAT("Invalid import path '{}', need at least one component", importPath.toString()));
    }

    if (importPath.getComponents().size() == 1) {
        // Import of file from the root bundle
        return ResourceId(StringBox(), StringCache::getGlobal().makeString(importPath.toString()));
    }

    // Import of file from an external bundle

    auto bundle = importPath.getFirstComponent();
    importPath.removeFirstComponent();
    auto fileToImport = importPath.toString();

    return ResourceId(StringCache::getGlobal().makeStringFromLiteral(bundle),
                      StringCache::getGlobal().makeString(std::move(fileToImport)));
}

Result<ResourceId> JavaScriptPathResolver::resolveResourceId(const StringBox& importString) {
    return resolveResourceId(ResourceId(StringBox(), StringBox()), importString);
}

Result<ResourceId> JavaScriptPathResolver::resolveResourceId(const ResourceId& currentResourceId,
                                                             const StringBox& importString) {
    // The resolved import path is in format <bundleName>/<path_to_file>

    auto importPath = Path(importString.toStringView());

    if (importPath.empty()) {
        return Error(STRING_FORMAT("Invalid import path '{}', no components found", importPath.toString()));
    }

    // Absolute import
    if (importPath.isAbsolute()) {
        importPath.removeFirstComponent();
        return pathToResourceId(std::move(importPath));
    }

    if (importPath.getFirstComponent() != ".." && importPath.getFirstComponent() != ".") {
        // TypeScript style absolute import
        return pathToResourceId(std::move(importPath));
    }

    // Relative imports
    auto resolvedPath = Path(currentResourceId.bundleName.toStringView());
    resolvedPath.append(currentResourceId.resourcePath.toStringView());
    // Move up to working directory
    resolvedPath.removeLastComponent();

    // Append import path
    resolvedPath.append(importPath);

    return pathToResourceId(std::move(resolvedPath));
}

} // namespace Valdi
