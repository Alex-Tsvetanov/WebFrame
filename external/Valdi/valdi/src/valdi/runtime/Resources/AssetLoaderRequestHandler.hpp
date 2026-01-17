//
//  AssetLoaderRequestHandler.hpp
//  valdi
//
//  Created by Simon Corsin on 8/19/21.
//

#pragma once

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/runtime/Resources/AssetLoaderRequestHandlerListener.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace snap::valdi_core {
class Cancelable;
}

namespace Valdi {

class AssetRequestPayloadCache;
class Context;

class AssetLoaderRequestHandler : public AssetLoaderCompletion {
public:
    AssetLoaderRequestHandler(Weak<AssetLoaderRequestHandlerListener> listener,
                              const Ref<Context>& context,
                              const AssetKey& assetKey,
                              const Ref<AssetRequestPayloadCache>& payloadCache,
                              const StringBox& url,
                              int32_t requestedWidth,
                              int32_t requestedHeight,
                              const Value& associatedData);
    ~AssetLoaderRequestHandler() override;

    void onLoadComplete(const Result<Ref<LoadedAsset>>& result) override;

    const Ref<Context>& getContext() const;

    void startLoadIfNeeded();

    void cancel();

    bool scheduledForCancelation() const;

    void setScheduledForCancelation();

    bool scheduledForLoad() const;

    void setScheduledForLoad();

    void incrementConsumersCount();
    size_t decrementConsumersCount();

    int32_t getRequestedWidth() const;

    int32_t getRequestedHeight() const;

    const Value& getAttachedData() const;

    const AssetKey& getAssetKey() const;

    const StringBox& getUrl() const;

    const Result<Ref<LoadedAsset>>& getLastLoadResult() const;

    void setLastLoadResult(const Result<Ref<LoadedAsset>>& lastLoadResult);

private:
    Weak<AssetLoaderRequestHandlerListener> _listener;
    Ref<Context> _context;
    AssetKey _assetKey;
    Ref<AssetRequestPayloadCache> _payloadCache;
    StringBox _url;
    int32_t _requestedWidth;
    int32_t _requestedHeight;
    size_t _consumersCount = 0;
    Shared<snap::valdi_core::Cancelable> _cancelable;
    Result<Ref<LoadedAsset>> _lastLoadResult;
    Value _associatedData;
    bool _scheduledForCancelation = false;
    bool _scheduledForLoad = false;
    bool _loadStarted = false;
};

} // namespace Valdi
