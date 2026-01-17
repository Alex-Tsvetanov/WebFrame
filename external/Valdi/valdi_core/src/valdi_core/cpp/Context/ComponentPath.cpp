//
//  ComponentPath.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/16/19.
//

#include "valdi_core/cpp/Context/ComponentPath.hpp"
#include "valdi_core/cpp/JavaScript/JavaScriptPathResolver.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

ComponentPath::ComponentPath() noexcept : _resourceId(StringBox()) {}
ComponentPath::ComponentPath(ResourceId resourceId, StringBox symbolName) noexcept
    : _resourceId(std::move(resourceId)), _symbolName(std::move(symbolName)) {}

ComponentPath ComponentPath::parse(const StringBox& componentPathString) {
    static constexpr char targetSuffix[] = ".valdi";
    if (componentPathString.hasSuffix(targetSuffix)) {
        // Backward compatibility
        return parse(componentPathString
                         .substring(0, componentPathString.length() - std::string::traits_type::length(targetSuffix))
                         .append(".vue.generated")
                         .prepend("ComponentClass@"));
    }

    StringBox resolvedSymbolName;
    StringBox resolvedComponentPath;

    auto symbolSeparator = componentPathString.indexOf('@');
    if (symbolSeparator) {
        auto parts = componentPathString.split(*symbolSeparator);
        resolvedSymbolName = std::move(parts.first);
        resolvedComponentPath = std::move(parts.second);
    } else {
        resolvedComponentPath = componentPathString;
    }

    auto result = JavaScriptPathResolver::resolveResourceId(resolvedComponentPath);
    if (!result) {
        return ComponentPath();
    }
    return ComponentPath(result.moveValue(), std::move(resolvedSymbolName));
}

const ResourceId& ComponentPath::getResourceId() const {
    return _resourceId;
}

const StringBox& ComponentPath::getSymbolName() const {
    return _symbolName;
}

std::string ComponentPath::toString() const {
    if (_symbolName.isEmpty()) {
        return _resourceId.toAbsolutePath();
    } else {
        return _symbolName.slowToString() + "@" + _resourceId.toAbsolutePath();
    }
}

bool ComponentPath::isEmpty() const {
    return _symbolName.isEmpty() && _resourceId == ResourceId(StringBox());
}

std::ostream& operator<<(std::ostream& os, const ComponentPath& d) noexcept {
    return os << d.toString();
}

} // namespace Valdi
