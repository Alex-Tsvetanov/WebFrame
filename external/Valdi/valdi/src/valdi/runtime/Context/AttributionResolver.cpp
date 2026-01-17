// Copyright © 2023 Snap, Inc. All rights reserved.

#include "valdi/runtime/Context/AttributionResolver.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

AttributionResolver::AttributionResolver(const AttributionResolver::MappingEntry* mapping,
                                         size_t length,
                                         ILogger& logger)
    : _logger(logger) {
    _mapping.reserve(length);

    auto& stringCache = StringCache::getGlobal();
    FlatMap<AttributionFunction, StringBox> allCallbacks;
    allCallbacks.reserve(length);

    for (size_t i = 0; i < length; i++) {
        const auto& it = mapping[i];
        auto moduleName = stringCache.makeString(std::string_view(it.moduleName));
        auto& ownerWithCallback = _mapping[moduleName];
        ownerWithCallback.owner = stringCache.makeString(std::string_view(it.owner));
        ownerWithCallback.callback = it.callback;

        const auto& callbackIt = allCallbacks.find(it.callback);
        SC_ABORT_UNLESS(
            callbackIt == allCallbacks.end(),
            fmt::format("Duplicate attribution function between {} and {}", callbackIt->second, moduleName));
        allCallbacks[it.callback] = moduleName;
    }
}

AttributionResolver::~AttributionResolver() = default;

Attribution AttributionResolver::resolveAttribution(const StringBox& moduleName) {
    const auto& it = _mapping.find(moduleName);
    if (it == _mapping.end()) {
        if (!moduleName.isEmpty()) {
            VALDI_WARN(_logger, "Could not resolve attribution for module '{}'", moduleName);
        }
        return AttributionResolver::getDefaultAttribution();
    }

    return Attribution(moduleName, it->second.owner, it->second.callback);
}

VALDI_ATTRIBUTION_FN(valdi, runtime);

Attribution AttributionResolver::getDefaultAttribution() {
    return Attribution(STRING_LITERAL("runtime"), STRING_LITERAL("VALDI"), &VALDI_ATTRIBUTION_FN_NAME(valdi, runtime));
}

} // namespace Valdi
