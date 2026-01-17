//
//  AssetLoaderRequestHandlerListener.hpp
//  valdi
//
//  Created by Simon Corsin on 8/19/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class AssetLoaderRequestHandler;
class LoadedAsset;

class AssetLoaderRequestHandlerListener {
public:
    virtual ~AssetLoaderRequestHandlerListener() = default;

    virtual void onLoad(const Ref<AssetLoaderRequestHandler>& requestHandler,
                        const Result<Ref<LoadedAsset>>& result) = 0;
};

} // namespace Valdi
