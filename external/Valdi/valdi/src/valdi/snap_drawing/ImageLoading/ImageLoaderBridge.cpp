//
//  ImageLoaderBridge.cpp
//  valdi
//
//  Created by Simon Corsin on 7/12/21.
//

#include "valdi/snap_drawing/ImageLoading/ImageLoaderBridge.hpp"
#include "valdi/snap_drawing/ImageLoading/ImageLoader.hpp"

namespace snap::drawing {

ImageLoaderBridge::ImageLoaderBridge(const Ref<ImageLoader>& imageLoader,
                                     const Valdi::Ref<Valdi::IRemoteDownloader>& downloader,
                                     std::vector<Valdi::StringBox> supportedSchemes)
    : Valdi::AssetLoader(std::move(supportedSchemes)), _imageLoader(imageLoader), _downloader(downloader) {}

ImageLoaderBridge::~ImageLoaderBridge() = default;

snap::valdi_core::AssetOutputType ImageLoaderBridge::getOutputType() const {
    return snap::valdi_core::AssetOutputType::ImageSnapDrawing;
}

Valdi::Result<Valdi::Value> ImageLoaderBridge::requestPayloadFromURL(const Valdi::StringBox& url) {
    return Valdi::Value(url);
}

Valdi::Shared<snap::valdi_core::Cancelable> ImageLoaderBridge::loadAsset(
    const Valdi::Value& requestPayload,
    int32_t preferredWidth,
    int32_t preferredHeight,
    const Valdi::Value& associatedData,
    const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) {
    return _imageLoader->loadAsset(
        requestPayload.toStringBox(), preferredWidth, preferredHeight, associatedData, _downloader, completion);
}

} // namespace snap::drawing
