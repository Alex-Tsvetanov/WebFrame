//
//  JavaScriptAssetLoadObserver.cpp
//  valdi
//
//  Created by Simon Corsin on 08/20/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#include "valdi/runtime/JavaScript/JavaScriptAssetLoadObserver.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"

#include "valdi_core/cpp/Resources/Asset.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

JavaScriptAssetLoadObserver::JavaScriptAssetLoadObserver(const Ref<ValueFunction>& callback) : _callback(callback) {}
JavaScriptAssetLoadObserver::~JavaScriptAssetLoadObserver() = default;

void JavaScriptAssetLoadObserver::onLoad(const /*not-null*/ std::shared_ptr<snap::valdi_core::Asset>& /*asset*/,
                                         const Valdi::Value& loadedAsset,
                                         const std::optional<Valdi::StringBox>& error) {
    auto errorValue = error ? Value(error.value()) : Value::undefined();
    (*_callback)({loadedAsset, errorValue});
}

AssetLoadObserverUnsubscribeFunc::AssetLoadObserverUnsubscribeFunc(
    const Ref<Asset>& asset, const Shared<snap::valdi_core::AssetLoadObserver>& assetLoadObserver)
    : _asset(asset), _assetLoadObserver(assetLoadObserver) {}

AssetLoadObserverUnsubscribeFunc::~AssetLoadObserverUnsubscribeFunc() {
    unsubscribe();
}

const ReferenceInfo& AssetLoadObserverUnsubscribeFunc::getReferenceInfo() const {
    return _referenceInfo;
}

JSValueRef AssetLoadObserverUnsubscribeFunc::operator()(JSFunctionNativeCallContext& callContext) noexcept {
    unsubscribe();
    return callContext.getContext().newUndefined();
}

void AssetLoadObserverUnsubscribeFunc::unsubscribe() {
    auto asset = std::move(_asset);
    auto assetLoadObserver = std::move(_assetLoadObserver);
    if (asset != nullptr && assetLoadObserver != nullptr) {
        asset->removeLoadObserver(assetLoadObserver);
    }
}
} // namespace Valdi
