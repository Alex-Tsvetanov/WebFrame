//
//  JavaScriptPathResolver.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/8/18.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi_core/cpp/Resources/ResourceId.hpp"

namespace Valdi {

class JavaScriptPathResolver {
public:
    static Result<ResourceId> resolveResourceId(const StringBox& importString);
    static Result<ResourceId> resolveResourceId(const ResourceId& currentResourceId, const StringBox& importString);
};

} // namespace Valdi
