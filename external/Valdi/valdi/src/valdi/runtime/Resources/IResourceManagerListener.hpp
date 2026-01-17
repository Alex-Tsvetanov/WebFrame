//
//  IResourceManagerListener.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {
class Bundle;

class IResourceManagerListener {
public:
    IResourceManagerListener() = default;
    virtual ~IResourceManagerListener() = default;

    virtual void bundleIsBeingUnloaded(const Shared<Bundle>& bundle) = 0;
};

} // namespace Valdi
