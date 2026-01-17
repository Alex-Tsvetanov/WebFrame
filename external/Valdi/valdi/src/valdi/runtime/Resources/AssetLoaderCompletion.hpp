//
//  AssetLoaderCompletion.hpp
//  valdi
//
//  Created by Simon Corsin on 8/12/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

class LoadedAsset;

class AssetLoaderCompletion : public SharedPtrRefCountable {
public:
    virtual void onLoadComplete(const Result<Ref<LoadedAsset>>& result) = 0;
};

} // namespace Valdi
