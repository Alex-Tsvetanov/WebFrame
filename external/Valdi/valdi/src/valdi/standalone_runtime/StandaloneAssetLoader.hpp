//
//  StandaloneAssetLoader.hpp
//  valdi-standalone_runtime
//
//  Created by Simon Corsin on 7/12/21.
//

#pragma once

#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace Valdi {

class IDiskCache;

class StandaloneAssetLoader : public AssetLoader {
public:
    explicit StandaloneAssetLoader(IDiskCache& diskCache);
    ~StandaloneAssetLoader() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Result<Valdi::Value> requestPayloadFromURL(const Valdi::StringBox& url) override;

    Shared<snap::valdi_core::Cancelable> loadAsset(const Value& requestPayload,
                                                   int32_t preferredWidth,
                                                   int32_t preferredHeight,
                                                   const Value& associatedData,
                                                   const Ref<AssetLoaderCompletion>& completion) override;

private:
    IDiskCache& _diskCache;
};

} // namespace Valdi
