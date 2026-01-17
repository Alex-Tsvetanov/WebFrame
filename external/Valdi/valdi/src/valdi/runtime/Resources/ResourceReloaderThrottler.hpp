//
//  ResourceReloaderThrottler.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/7/18.
//

#pragma once

#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace Valdi {

struct ReceivedResource {
    std::size_t hash;
    std::chrono::steady_clock::time_point lastReloadTime;
};

class ResourceReloaderThrottler {
public:
    ResourceReloaderThrottler();

    /**
     Notifies that a resource has been received.
     Returns whether the resource should be reloaded or not.
     */
    bool receivedResource(const std::vector<uint8_t>& resource, const ResourceId& resourceId);

private:
    FlatMap<ResourceId, ReceivedResource> _receivedResources;
};

} // namespace Valdi
