//
//  Resource.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/16/19.
//

#pragma once

#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include <vector>

namespace Valdi {

struct Resource {
    ResourceId resourceId;
    BytesView data;

    Resource(ResourceId resourceId, BytesView data);
};

} // namespace Valdi
