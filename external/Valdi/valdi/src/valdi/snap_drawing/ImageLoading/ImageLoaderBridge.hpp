//
//  ImageLoaderBridge.hpp
//  valdi
//
//  Created by Simon Corsin on 7/12/21.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace snap::drawing {

class ImageLoader;

class ImageLoaderBridge : public Valdi::AssetLoader {
public:
    ImageLoaderBridge(const Ref<ImageLoader>& imageLoader,
                      const Valdi::Ref<Valdi::IRemoteDownloader>& downloader,
                      std::vector<Valdi::StringBox> supportedSchemes);
    ~ImageLoaderBridge() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Result<Valdi::Value> requestPayloadFromURL(const Valdi::StringBox& url) override;

    Valdi::Shared<snap::valdi_core::Cancelable> loadAsset(
        const Valdi::Value& requestPayload,
        int32_t preferredWidth,
        int32_t preferredHeight,
        const Valdi::Value& associatedData,
        const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) override;

private:
    Ref<ImageLoader> _imageLoader;
    Valdi::Ref<Valdi::IRemoteDownloader> _downloader;
};

} // namespace snap::drawing
