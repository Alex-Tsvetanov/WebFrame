//
//  ViewNodeAssetHandler.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/11/21.
//

#include "valdi/runtime/Context/ViewNodeAssetHandler.hpp"
#include "valdi/runtime/Context/IViewNodesAssetTracker.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi/runtime/Interfaces/IViewTransaction.hpp"

#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

#include "valdi_core/cpp/Resources/Asset.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"

#include "valdi_core/Platform.hpp"

namespace Valdi {

ViewNodeAssetHandler::ViewNodeAssetHandler(ViewNode& viewNode, snap::valdi_core::AssetOutputType assetOutputType)

    : _viewNode(weakRef(&viewNode)),
      _viewNodeTree(weakRef(viewNode.getViewNodeTree())),
      _assetTracker(viewNode.getViewNodeTree() != nullptr ? viewNode.getViewNodeTree()->getAssetTracker() : nullptr),
      _assetOutputType(assetOutputType),
      _viewNodeId(viewNode.getRawId()) {}

ViewNodeAssetHandler::~ViewNodeAssetHandler() = default;

void ViewNodeAssetHandler::onContainerSizeChanged(float width, float height) {
    if (_observingAsset != nullptr) {
        auto viewNode = _viewNode.lock();
        if (viewNode != nullptr) {
            auto imageSize = resolveImageSize(*viewNode);
            if (_requestedImageWidth != imageSize.first || _requestedImageHeight != imageSize.second) {
                _requestedImageWidth = imageSize.first;
                _requestedImageHeight = imageSize.second;
                _observingAsset->updateLoadObserverPreferredSize(strongRef(this), imageSize.first, imageSize.second);
            }
        }
    }
}

void ViewNodeAssetHandler::setAsset(ViewTransactionScope& viewTransactionScope,
                                    const Ref<View>& view,
                                    const Ref<Asset>& asset,
                                    const Ref<ValueFunction>& onLoadCallback,
                                    const Value& attachedData,
                                    bool flipOnRtl) {
    setAssetInner(viewTransactionScope, view, asset, nullptr, onLoadCallback, attachedData, flipOnRtl);
}

void ViewNodeAssetHandler::setLoadedAsset(ViewTransactionScope& viewTransactionScope,
                                          const Ref<View>& view,
                                          const Ref<LoadedAsset>& loadedAsset,
                                          const Ref<ValueFunction>& onLoadCallback,
                                          const Value& attachedData,
                                          bool flipOnRtl) {
    setAssetInner(viewTransactionScope, view, nullptr, loadedAsset, onLoadCallback, attachedData, flipOnRtl);
}

void ViewNodeAssetHandler::setAssetInner(ViewTransactionScope& viewTransactionScope,
                                         const Ref<View>& view,
                                         const Ref<Asset>& asset,
                                         const Ref<LoadedAsset>& loadedAsset,
                                         const Ref<ValueFunction>& onLoadCallback,
                                         const Value& attachedData,
                                         bool flipOnRtl) {
    bool hasAnyAsset = asset != nullptr || loadedAsset != nullptr;

    if (!hasAnyAsset) {
        if (_view != nullptr) {
            // Immediately unset asset to the view if we had a view attached
            viewTransactionScope.transaction().setViewLoadedAsset(_view, nullptr, /* shouldFlip */ false);
            _view = nullptr;
        }
    } else {
        if (_view != view) {
            if (_view != nullptr) {
                // If we are swapping views, remove the asset from the previously attached view
                viewTransactionScope.transaction().setViewLoadedAsset(_view, nullptr, /* shouldFlip */ false);
            }
            _view = view;
            _viewNeedsUpdate = true;
        }
    }

    _asset = asset;
    _loadedAsset = loadedAsset;
    _onLoadCallback = onLoadCallback;

    if (_flipOnRtl != flipOnRtl) {
        _flipOnRtl = flipOnRtl;
        _viewNeedsUpdate = true;
    }

    if (_attachedData != attachedData) {
        _attachedData = attachedData;
        _needsObserverUpdate = true;
    }

    updateLoadObserver();

    if (_viewNeedsUpdate) {
        notifyView();
    }
}

void ViewNodeAssetHandler::updateLoadObserver() {
    if (_observingAsset != _asset || _needsObserverUpdate) {
        if (_loadedAsset != nullptr) {
            _viewNeedsUpdate = true;
            _loadedAsset = nullptr;
        }
        _needsObserverUpdate = false;

        auto self = strongRef(this);
        auto observingAsset = std::move(_observingAsset);

        if (observingAsset != nullptr) {
            if (_assetTracker != nullptr) {
                _assetTracker->onEndRequestingLoadedAsset(_viewNodeId, observingAsset);
            }

            observingAsset->removeLoadObserver(self);
        }

        auto viewNode = _viewNode.lock();

        if (viewNode != nullptr && !viewNode->isMeasurerPlaceholder() && _asset != nullptr) {
            _observingAsset = _asset;
            _loadCallbackCalled = false;

            auto imageSize = resolveImageSize(*viewNode);
            _requestedImageWidth = imageSize.first;
            _requestedImageHeight = imageSize.second;

            if (_assetTracker != nullptr) {
                _assetTracker->onBeganRequestingLoadedAsset(_viewNodeId, _observingAsset);
            }

            _observingAsset->addLoadObserver(self, _assetOutputType, imageSize.first, imageSize.second, _attachedData);
        }
    }
}

std::pair<int32_t, int32_t> ViewNodeAssetHandler::computeAssetSizeInPixelsForBounds(Asset& asset,
                                                                                    float boundsWidth,
                                                                                    float boundsHeight,
                                                                                    float pointScale) {
    auto resolvedImageWidth = asset.getWidth();
    auto resolvedImageHeight = asset.getHeight();

    if (resolvedImageWidth == 0) {
        resolvedImageWidth = boundsWidth;
    }
    if (resolvedImageHeight == 0) {
        resolvedImageHeight = boundsHeight;
    }

    if (boundsWidth != 0 && boundsHeight != 0) {
        auto wRatio = boundsWidth / resolvedImageWidth;
        auto hRatio = boundsHeight / resolvedImageHeight;

        auto ratio = std::max(wRatio, hRatio);

        resolvedImageWidth *= ratio;
        resolvedImageHeight *= ratio;
    }

    return std::make_pair(pointsToPixels(resolvedImageWidth, pointScale),
                          pointsToPixels(resolvedImageHeight, pointScale));
}

std::pair<int32_t, int32_t> ViewNodeAssetHandler::resolveImageSize(ViewNode& viewNode) const {
    const auto& frame = viewNode.getCalculatedFrame();
    return computeAssetSizeInPixelsForBounds(*_observingAsset, frame.width, frame.height, viewNode.getPointScale());
}

void ViewNodeAssetHandler::onLoad(const std::shared_ptr<snap::valdi_core::Asset>& asset,
                                  const Valdi::Value& loadedAsset,
                                  const std::optional<Valdi::StringBox>& error) {
    auto viewNodeTree = _viewNodeTree.lock();
    if (viewNodeTree == nullptr) {
        return;
    }

    viewNodeTree->withLock([&]() {
        if (asset.get() != _observingAsset.get()) {
            return;
        }

        _loadedAsset = castOrNull<LoadedAsset>(loadedAsset.getValdiObject());

        notifyView();

        if (_assetTracker != nullptr) {
            _assetTracker->onLoadedAssetChanged(_viewNodeId, _observingAsset, error);
        }

        if (!_loadCallbackCalled && _onLoadCallback != nullptr) {
            _loadCallbackCalled = true;

            if (_onLoadCallback != nullptr) {
                if (error) {
                    (*_onLoadCallback)({Value(false), Value(error.value())});
                } else {
                    (*_onLoadCallback)({Value(true)});
                }
            }
        }
    });
}

void ViewNodeAssetHandler::notifyView() {
    if (_view == nullptr) {
        return;
    }

    auto viewNodeTree = _viewNodeTree.lock();
    if (viewNodeTree == nullptr) {
        return;
    }

    _viewNeedsUpdate = false;

    auto viewNode = _viewNode.lock();
    bool shouldFlip = false;
    if (_flipOnRtl && viewNode != nullptr && viewNode->isRightToLeft()) {
        shouldFlip = true;
    }

    viewNodeTree->getCurrentViewTransactionScope().transaction().setViewLoadedAsset(_view, _loadedAsset, shouldFlip);
}

} // namespace Valdi
