//
//  AssetRequestPayloadCache.cpp
//  valdi
//
//  Created by Simon Corsin on 8/20/21.
//

#include "valdi/runtime/Resources/AssetRequestPayloadCache.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace Valdi {

AssetRequestPayloadCache::AssetRequestPayloadCache(const Ref<AssetLoader>& assetLoader) : _assetLoader(assetLoader) {}
AssetRequestPayloadCache::~AssetRequestPayloadCache() = default;

const Ref<AssetLoader>& AssetRequestPayloadCache::getAssetLoader() const {
    return _assetLoader;
}

const Result<Value>& AssetRequestPayloadCache::getRequestPayload() const {
    return _requestPayload;
}

void AssetRequestPayloadCache::setRequestPayload(const Result<Value>& requestPayload) {
    _requestPayload = requestPayload;
}

} // namespace Valdi
