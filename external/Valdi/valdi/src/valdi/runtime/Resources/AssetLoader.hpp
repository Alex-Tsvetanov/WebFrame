//
//  AssetLoader.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/12/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi_core/AssetOutputType.hpp"
#include "valdi_core/Cancelable.hpp"

#include <vector>

namespace Valdi {

class LoadedAsset;
class AssetLoaderCompletion;

class AssetLoader : public SharedPtrRefCountable {
public:
    explicit AssetLoader(std::vector<StringBox> supportedSchemes);
    ~AssetLoader() override;

    virtual snap::valdi_core::AssetOutputType getOutputType() const = 0;

    /**
     * If true, if the asset criteria are identical, the assets are shared among different consumers.
     * Otherwise, a unique asset is created for each consumer.
     */
    virtual bool canReuseLoadedAssets() const {
        return true;
    }

    constexpr const std::vector<StringBox>& getSupportedSchemes() const {
        return _supportedSchemes;
    }

    virtual Result<Value> requestPayloadFromURL(const StringBox& url) = 0;

    /**
     * Load the asset with the given requestPayload, preferred output size, and associatedData.
     * The associatedData is an arbitrary object that can be used to help loading the asset.
     * It can be a filter when loading an image, or the font manager when loading a Lottie scene.
     */
    virtual Shared<snap::valdi_core::Cancelable> loadAsset(const Value& requestPayload,
                                                           int32_t preferredWidth,
                                                           int32_t preferredHeight,
                                                           const Value& associatedData,
                                                           const Ref<AssetLoaderCompletion>& completion) = 0;

private:
    std::vector<StringBox> _supportedSchemes;
};

} // namespace Valdi
