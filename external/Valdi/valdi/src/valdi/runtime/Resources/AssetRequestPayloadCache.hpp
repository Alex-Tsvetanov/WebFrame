//
//  AssetRequestPayloadCache.hpp
//  valdi
//
//  Created by Simon Corsin on 8/20/21.
//

#pragma once

#include "valdi_core/cpp/Utils/LinkedList.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class AssetLoader;

class AssetRequestPayloadCache : public LinkedListNode {
public:
    explicit AssetRequestPayloadCache(const Ref<AssetLoader>& assetLoader);
    ~AssetRequestPayloadCache() override;

    const Ref<AssetLoader>& getAssetLoader() const;

    const Result<Value>& getRequestPayload() const;
    void setRequestPayload(const Result<Value>& requestPayload);

private:
    Ref<AssetLoader> _assetLoader;
    Result<Value> _requestPayload;
};

} // namespace Valdi
