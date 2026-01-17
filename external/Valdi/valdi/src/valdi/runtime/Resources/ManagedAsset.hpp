//
//  ManagedAsset.hpp
//  valdi
//
//  Created by Simon Corsin on 6/29/21.
//

#pragma once

#include "valdi_core/Asset.hpp"
#include "valdi_core/cpp/Utils/LinkedList.hpp"
#include "valdi_core/cpp/Utils/PlatformResult.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi/runtime/Resources/AssetConsumer.hpp"
#include "valdi_core/cpp/Resources/AssetLocation.hpp"

namespace Valdi {

class AssetLoaderCompletion;

enum AssetState {
    AssetStateInitial = 0,
    AssetStateResolvingLocation,
    AssetStateFailedPermanently,
    AssetStateFailedRetryable,
    AssetStateReady,
};

class ObservableAsset;
class AssetLoader;

class AssetRequestPayloadCache;

/**
 A ManagedAsset contains the state of a single asset inside the AssetsManager.
 It holds the resolved asset location as well as a list of consumers, which will
 be notified whenever the asset has finished loading.
 */
class ManagedAsset : public SimpleRefCountable {
public:
    ManagedAsset();
    ~ManagedAsset() override;

    Shared<ObservableAsset> getObservable() const;
    void setObservable(const Shared<ObservableAsset>& observable);

    AssetState getState() const;
    void setState(AssetState assetState);

    Ref<AssetConsumer> addConsumer();
    void removeConsumer(const Ref<AssetConsumer>& consumer);

    std::optional<size_t> getConsumerIndex(const Ref<AssetConsumer>& consumer) const;

    const Ref<AssetConsumer>& getConsumer(size_t index) const;

    size_t getConsumersSize() const;
    bool hasConsumers() const;

    const Result<AssetLocation>& getResolvedAssetLocation() const;
    void setResolvedAssetLocation(const Result<AssetLocation>& resolvedAssetLocation);

    uint64_t getResolveId() const;
    void setResolveId(uint64_t resolveId);

    Ref<AssetRequestPayloadCache> getPayloadCacheForAssetLoader(const Ref<AssetLoader>& assetLoader);
    void clearPayloadCache();

private:
    std::vector<Ref<AssetConsumer>> _consumers;
    Weak<ObservableAsset> _observable;
    AssetState _assetState = AssetStateInitial;
    Result<AssetLocation> _assetLocation;
    LinkedList<AssetRequestPayloadCache> _payloadCache;
    uint64_t _resolveId = 0;
};

std::ostream& operator<<(std::ostream& os, const AssetState& k) noexcept;

} // namespace Valdi
