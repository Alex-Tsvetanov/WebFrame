//
//  AssetResolver.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/25/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi_core/cpp/Resources/Asset.hpp"

namespace Valdi {

class ViewNode;
class ResourceManager;

class AssetResolver {
public:
    /**
     Resolves an Asset from a ViewNode and an attribute value.
     */
    static Result<Ref<Asset>> resolve(ViewNode& viewNode, const Value& value);

    /**
     Resolves an Asset from a ResourceManager object and an attribute value.
     Return null if the asset needs the ViewNode to be resolved.
     */
    static Ref<Asset> resolve(ResourceManager& resourceManager, const Value& value);
};

} // namespace Valdi
