//
//  AssetConsumer.hpp
//  valdi
//
//  Created by Simon Corsin on 6/30/21.
//

#include "valdi_core/AssetOutputType.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace snap::valdi_core {
class AssetLoadObserver;
class Cancelable;
} // namespace snap::valdi_core

namespace Valdi {

class AssetLoaderCompletion;
class LoadedAsset;
class Context;

enum AssetConsumerState {
    AssetConsumerStateInitial = 0,
    AssetConsumerStateLoading,
    AssetConsumerStateFailed,
    AssetConsumerStateLoaded,
    AssetConsumerStateRemoved,
};

/**
 Represent a single consumer for an asset.
 Consumers are created whenever addLoadObserver
 is called on the ObservableAsset.
 */
class AssetConsumer : public SimpleRefCountable {
public:
    AssetConsumer();
    ~AssetConsumer() override;

    AssetConsumerState getState() const;
    void setState(AssetConsumerState state);

    void setNotified(bool notified);
    bool notified() const;

    void setPreferredWidth(int32_t preferredWidth);
    int32_t getPreferredWidth() const;

    void setPreferredHeight(int32_t preferredHeight);
    int32_t getPreferredHeight() const;

    void setAttachedData(const Value& attachedData);
    const Value& getAttachedData() const;

    const Ref<Context>& getContext() const;
    void setContext(const Ref<Context>& context);

    void setObserver(const Shared<snap::valdi_core::AssetLoadObserver>& observer);
    const Shared<snap::valdi_core::AssetLoadObserver>& getObserver() const;

    void setLoadedAsset(const Result<Ref<LoadedAsset>>& loadedAsset);
    const Result<Ref<LoadedAsset>>& getLoadedAsset() const;

    void setOutputType(snap::valdi_core::AssetOutputType outputType);
    snap::valdi_core::AssetOutputType getOutputType() const;

    const Ref<AssetLoaderCompletion>& getAssetLoaderCompletion() const;
    void setAssetLoaderCompletion(const Ref<AssetLoaderCompletion>& assetLoaderCompletion);

private:
    Ref<Context> _context;
    Shared<snap::valdi_core::AssetLoadObserver> _observer;
    AssetConsumerState _state = AssetConsumerStateInitial;
    snap::valdi_core::AssetOutputType _outputType;
    bool _notified = false;
    int32_t _preferredWidth = 0;
    int32_t _preferredHeight = 0;
    Result<Ref<LoadedAsset>> _loadedAsset;
    Ref<AssetLoaderCompletion> _assetLoaderCompletion;
    Value _attachedData;
};

} // namespace Valdi
