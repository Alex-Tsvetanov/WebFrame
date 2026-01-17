//
//  RemoteModuleResources.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/29/19.
//

#include "valdi/runtime/Resources/Remote/RemoteModuleResources.hpp"

namespace Valdi {

RemoteModuleResources::RemoteModuleResources(FlatMap<StringBox, StringBox> resourceCacheUrls)
    : _resourceCacheUrls(std::move(resourceCacheUrls)) {}

RemoteModuleResources::RemoteModuleResources() = default;

std::optional<StringBox> RemoteModuleResources::getResourceCacheUrl(const StringBox& resourcePath) const {
    const auto& it = _resourceCacheUrls.find(resourcePath);
    if (it == _resourceCacheUrls.end()) {
        return std::nullopt;
    }

    return {it->second};
}

bool RemoteModuleResources::operator==(const RemoteModuleResources& other) const {
    return _resourceCacheUrls == other._resourceCacheUrls;
}

bool RemoteModuleResources::operator!=(const RemoteModuleResources& other) const {
    return !(*this == other);
}

const FlatMap<StringBox, StringBox>& RemoteModuleResources::getAllUrls() const {
    return _resourceCacheUrls;
}

VALDI_CLASS_IMPL(RemoteModuleResources)
} // namespace Valdi
