//
//  ResourceReloaderThrottler.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/7/18.
//

#include "valdi/runtime/Resources/ResourceReloaderThrottler.hpp"

namespace Valdi {

ResourceReloaderThrottler::ResourceReloaderThrottler() = default;

std::size_t hashVector(const std::vector<uint8_t>& resource) {
    std::string_view arrayView(reinterpret_cast<const char*>(resource.data()), resource.size());
    return std::hash<std::string_view>()(arrayView);
}

bool ResourceReloaderThrottler::receivedResource(const std::vector<uint8_t>& resource, const ResourceId& resourceId) {
    auto now = std::chrono::steady_clock::now();

    auto hash = hashVector(resource);

    const auto& it = _receivedResources.find(resourceId);
    if (it != _receivedResources.end() && it->second.hash == hash &&
        it->second.lastReloadTime + std::chrono::seconds(1) > now) {
        // If we received a resource with the same hash and same resource id less than a second ago, we throttle it.
        return false;
    }

    _receivedResources[resourceId] = {.hash = hash, .lastReloadTime = now};

    return true;
}

} // namespace Valdi
