
#pragma once

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi/runtime/Resources/AssetLoaderFactory.hpp"

namespace ValdiTest {

struct MockedAssetLoaderWithDownloader : public Valdi::AssetLoader {
    snap::valdi_core::AssetOutputType outputType;
    Valdi::Ref<Valdi::IRemoteDownloader> downloader;

    MockedAssetLoaderWithDownloader(snap::valdi_core::AssetOutputType outputType,
                                    std::vector<Valdi::StringBox> urlSchemes,
                                    const Valdi::Ref<Valdi::IRemoteDownloader>& downloader);

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Result<Valdi::Value> requestPayloadFromURL(const Valdi::StringBox& url) override;

    Valdi::Shared<snap::valdi_core::Cancelable> loadAsset(
        const Valdi::Value& requestPayload,
        int32_t preferredWidth,
        int32_t preferredHeight,
        const Valdi::Value& filter,
        const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) override;
};

struct MockAssetLoaderFactory : public Valdi::AssetLoaderFactory {
    snap::valdi_core::AssetOutputType outputType;

    MockAssetLoaderFactory(snap::valdi_core::AssetOutputType outputType);
    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Ref<Valdi::AssetLoader> createAssetLoader(const std::vector<Valdi::StringBox>& urlSchemes,
                                                     const Valdi::Ref<Valdi::IRemoteDownloader>& downloader) override;
};

} // namespace ValdiTest
