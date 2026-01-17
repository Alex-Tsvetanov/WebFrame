//
//  AssetsManager.hpp
//  valdi
//
//  Created by Simon Corsin on 6/28/21.
//

#pragma once

#include "valdi_core/cpp/Resources/Asset.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi/runtime/Resources/AssetLoaderRequestHandlerListener.hpp"
#include "valdi/runtime/Resources/ManagedAsset.hpp"

#include "valdi_core/AssetOutputType.hpp"
#include "valdi_core/cpp/Resources/AssetLocation.hpp"

#include <deque>
#include <vector>

namespace Valdi {

class DispatchQueue;
class RemoteModuleResources;
class IResourceLoader;
class RemoteModuleManager;
class ObservableAsset;
class ILogger;
class AssetConsumer;
class AssetsManagerTransaction;
class AssetLoaderManager;
class LoadedAsset;
class MainThreadManager;
class Context;

class AssetsManagerListener {
public:
    virtual ~AssetsManagerListener() = default;

    virtual void onManagedAssetUpdated(const Ref<ManagedAsset>& managedAsset) = 0;
    virtual void onPerformedUpdates() = 0;
};

class AssetBytesStore;

class AssetsManager : public SharedPtrRefCountable, public AssetLoaderRequestHandlerListener {
public:
    AssetsManager(const Shared<IResourceLoader>& resourceLoader,
                  const Ref<RemoteModuleManager>& remoteModuleManager,
                  const Ref<AssetLoaderManager>& assetLoaderManager,
                  const Ref<DispatchQueue>& workerQueue,
                  MainThreadManager& mainThreadManager,
                  ILogger& logger);
    ~AssetsManager() override;

    Ref<Asset> getAsset(const AssetKey& assetKey);
    Ref<Asset> createAssetWithBytes(const BytesView& bytes);
    bool isAssetAlive(const AssetKey& assetKey) const;

    std::optional<AssetLocation> getResolvedAssetLocation(const AssetKey& assetKey);

    void setResolvedAssetLocation(const AssetKey& assetKey, const AssetLocation& assetLocation);

    void onAssetCatalogChanged(const Ref<Bundle>& bundle);

    void addAssetLoadObserver(const AssetKey& assetKey,
                              const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                              const Ref<Context>& context,
                              snap::valdi_core::AssetOutputType outputType,
                              int32_t preferredWidth,
                              int32_t preferredHeight,
                              const Value& attachedData);

    void removeAssetLoadObserver(const AssetKey& assetKey,
                                 const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer);

    void updateAssetLoadObserverPreferredSize(const AssetKey& assetKey,
                                              const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                                              int32_t preferredWidth,
                                              int32_t preferredHeight);

    static bool isAssetUrl(const StringBox& str);

    void onLoad(const Ref<AssetLoaderRequestHandler>& request, const Result<Ref<LoadedAsset>>& result) override;

    void setShouldRemoveUnusedLocalAssets(bool removeUnusedLocalAssets);

    void setListener(AssetsManagerListener* listener);

    void beginPauseUpdates();
    void endPauseUpdates();

    void flushUpdates();

private:
    Shared<IResourceLoader> _resourceLoader;
    Ref<RemoteModuleManager> _remoteModuleManager;
    Ref<AssetLoaderManager> _assetLoaderManager;
    Ref<DispatchQueue> _workerQueue;
    MainThreadManager& _mainThreadManager;
    [[maybe_unused]] ILogger& _logger;
    mutable std::recursive_mutex _mutex;
    FlatMap<AssetKey, Ref<ManagedAsset>> _assets;
    std::vector<AssetKey> _scheduledUpdates;
    std::deque<Ref<AssetLoaderRequestHandler>> _pendingLoadRequests;
    Ref<AssetBytesStore> _assetBytesStore;
    AssetsManagerListener* _listener = nullptr;
    int32_t _pauseUpdatesCount = 0;
    uint64_t _assetResolveIdSequence = 0;
    bool _pendingLoadRequestsScheduled = false;
    bool _removeUnusedLocalAssets = false;

    friend ObservableAsset;

    std::unique_lock<std::recursive_mutex> lock() const;

    Shared<ObservableAsset> createObservable(const AssetKey& assetKey);
    void onObservableDestroyed(const AssetKey& assetKey);

    Ref<Asset> lockFreeGetAsset(const AssetKey& assetKey);

    void updateAsset(AssetsManagerTransaction& transaction, const AssetKey& assetKey);

    void scheduleAssetUpdate(std::unique_lock<std::recursive_mutex> lock, const AssetKey& assetKey);
    void scheduleAssetUpdate(AssetsManagerTransaction& transaction, const AssetKey& assetKey);

    void resolveAssetLocation(AssetsManagerTransaction& transaction,
                              const AssetKey& assetKey,
                              const Ref<ManagedAsset>& managedAsset);

    void updateAssetConsumers(AssetsManagerTransaction& transaction,
                              const AssetKey& assetKey,
                              const Ref<ManagedAsset>& managedAsset);
    void doUpdateAssetConsumer(AssetsManagerTransaction& transaction,
                               const AssetKey& assetKey,
                               const Ref<ManagedAsset>& managedAsset,
                               const Ref<AssetConsumer>& consumerToUpdate);

    void loadAssetForConsumerAtResolvedLocation(AssetsManagerTransaction& transaction,
                                                const AssetKey& assetKey,
                                                const Ref<ManagedAsset>& managedAsset,
                                                const Ref<AssetConsumer>& assetConsumer,
                                                const AssetLocation& assetLocation);

    void notifyAssetConsumer(AssetsManagerTransaction& transaction,
                             const AssetKey& assetKey,
                             const Ref<ManagedAsset>& managedAsset,
                             const Ref<AssetConsumer>& assetConsumer,
                             const Ref<LoadedAsset>& loadedAsset,
                             const std::optional<Error>& error);

    void onLoadingRemoteResourcesCompleted(const AssetKey& assetKey,
                                           const Result<Ref<RemoteModuleResources>>& result,
                                           uint64_t resolveId);
    void resolveLocalAssetLocationAndUpdate(const AssetKey& assetKey, uint64_t resolveId);

    Result<AssetLocation> resolveRemoteAssetLocation(const AssetKey& assetKey,
                                                     const Result<Ref<RemoteModuleResources>>& result);
    Result<AssetLocation> resolveLocalAssetLocation(const AssetKey& assetKey);

    void updateAssetLocation(const AssetKey& assetKey,
                             const Ref<ManagedAsset>& managedAsset,
                             const Result<AssetLocation>& assetLocation);

    void performUpdates(std::unique_lock<std::recursive_mutex>&& lock);

    Ref<ManagedAsset> getManagedAsset(const AssetKey& assetKey) const;
    Ref<ManagedAsset> getOrCreateManagedAsset(const AssetKey& assetKey);

    void removeAssetConsumer(AssetsManagerTransaction& transaction,
                             const Ref<ManagedAsset>& managedAsset,
                             const Ref<AssetConsumer>& assetConsumer);

    bool removeManagedAssetIfNeeded(const AssetKey& assetKey, const Ref<ManagedAsset>& managedAsset);

    static void onConsumerLoad(const Ref<AssetConsumer>& assetConsumer, const Result<Ref<LoadedAsset>>& result);
    void updateConsumerRequestHandler(const Ref<AssetConsumer>& assetConsumer,
                                      const Ref<AssetLoaderRequestHandler>& request);

    void scheduleFlushLoadRequests();

    void flushLoadRequests();

    void schedulePerformUpdates();

    AssetKey lockFreeGenerateBytesAssetKey();
};

} // namespace Valdi
