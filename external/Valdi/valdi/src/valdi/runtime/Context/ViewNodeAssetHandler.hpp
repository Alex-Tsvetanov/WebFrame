//
//  ViewNodeAssetHandler.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/11/21.
//

#pragma once

#include "valdi/runtime/Context/IViewNodeAssetHandler.hpp"
#include "valdi/runtime/Context/RawViewNodeId.hpp"
#include "valdi/runtime/Views/View.hpp"

#include "valdi_core/AssetLoadObserver.hpp"
#include "valdi_core/AssetOutputType.hpp"

namespace Valdi {

class Asset;
class ViewNodeTree;
class ViewTransactionScope;
class IViewNodesAssetTracker;

class ViewNodeAssetHandler : public IViewNodeAssetHandler, public snap::valdi_core::AssetLoadObserver {
public:
    ViewNodeAssetHandler(ViewNode& viewNode, snap::valdi_core::AssetOutputType assetOutputType);
    ~ViewNodeAssetHandler() override;

    void onContainerSizeChanged(float width, float height) override;

    constexpr const Ref<Asset>& getAsset() const {
        return _asset;
    }

    void setAsset(ViewTransactionScope& viewTransactionScope,
                  const Ref<View>& view,
                  const Ref<Asset>& asset,
                  const Ref<ValueFunction>& onLoadCallback,
                  const Value& attachedData,
                  bool flipOnRtl);

    void setLoadedAsset(ViewTransactionScope& viewTransactionScope,
                        const Ref<View>& view,
                        const Ref<LoadedAsset>& loadedAsset,
                        const Ref<ValueFunction>& onLoadCallback,
                        const Value& attachedData,
                        bool flipOnRtl);

    void onLoad(const std::shared_ptr<snap::valdi_core::Asset>& asset,
                const Valdi::Value& loadedAsset,
                const std::optional<Valdi::StringBox>& error) override;

    static std::pair<int32_t, int32_t> computeAssetSizeInPixelsForBounds(Asset& asset,
                                                                         float boundsWidth,
                                                                         float boundsHeight,
                                                                         float pointScale);

private:
    Weak<ViewNode> _viewNode;
    Weak<ViewNodeTree> _viewNodeTree;
    Ref<IViewNodesAssetTracker> _assetTracker;
    snap::valdi_core::AssetOutputType _assetOutputType;
    RawViewNodeId _viewNodeId;
    Ref<View> _view;
    Ref<Asset> _asset;
    Ref<Asset> _observingAsset;
    Ref<ValueFunction> _onLoadCallback;
    Ref<LoadedAsset> _loadedAsset;
    Value _attachedData;
    bool _loadCallbackCalled = false;
    bool _flipOnRtl = false;
    bool _viewNeedsUpdate = false;
    bool _needsObserverUpdate = false;
    int32_t _requestedImageWidth = 0;
    int32_t _requestedImageHeight = 0;

    void updateLoadObserver();

    void setAssetInner(ViewTransactionScope& viewTransactionScope,
                       const Ref<View>& view,
                       const Ref<Asset>& asset,
                       const Ref<LoadedAsset>& loadedAsset,
                       const Ref<ValueFunction>& onLoadCallback,
                       const Value& attachedData,
                       bool flipOnRtl);

    std::pair<int32_t, int32_t> resolveImageSize(ViewNode& viewNode) const;

    void notifyView();
};

} // namespace Valdi
