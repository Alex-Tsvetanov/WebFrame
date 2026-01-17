//
//  Resource.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/16/19.
//

#include "valdi/runtime/Resources/Resource.hpp"

namespace Valdi {

Resource::Resource(ResourceId resourceId, BytesView data) : resourceId(std::move(resourceId)), data(std::move(data)) {}

} // namespace Valdi
