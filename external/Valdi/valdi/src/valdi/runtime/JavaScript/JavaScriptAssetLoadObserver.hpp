//
//  JavaScriptAssetLoadObserver.hpp
//  valdi
//
//  Created by Simon Corsin on 08/20/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"
#include "valdi_core/AssetLoadObserver.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"

namespace Valdi {

class ValueFunction;
class Asset;

class JavaScriptAssetLoadObserver : public snap::valdi_core::AssetLoadObserver {
public:
    explicit JavaScriptAssetLoadObserver(const Ref<ValueFunction>& callback);
    ~JavaScriptAssetLoadObserver() override;

    void onLoad(const /*not-null*/ std::shared_ptr<snap::valdi_core::Asset>& /*asset*/,
                const Valdi::Value& loadedAsset,
                const std::optional<Valdi::StringBox>& error) override;

private:
    Ref<ValueFunction> _callback;
};

class AssetLoadObserverUnsubscribeFunc : public JSFunction {
public:
    AssetLoadObserverUnsubscribeFunc(const Ref<Asset>& asset,
                                     const Shared<snap::valdi_core::AssetLoadObserver>& assetLoadObserver);

    ~AssetLoadObserverUnsubscribeFunc() override;

    const ReferenceInfo& getReferenceInfo() const override;

    JSValueRef operator()(JSFunctionNativeCallContext& callContext) noexcept override;

private:
    Ref<Asset> _asset;
    Shared<snap::valdi_core::AssetLoadObserver> _assetLoadObserver;
    ReferenceInfo _referenceInfo;

    void unsubscribe();
};
} // namespace Valdi
