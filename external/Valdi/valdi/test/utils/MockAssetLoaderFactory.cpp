#include "MockAssetLoaderFactory.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/standalone_runtime/StandaloneLoadedAsset.hpp"

using namespace Valdi;

namespace ValdiTest {

MockedAssetLoaderWithDownloader::MockedAssetLoaderWithDownloader(snap::valdi_core::AssetOutputType outputType,
                                                                 std::vector<StringBox> urlSchemes,
                                                                 const Ref<IRemoteDownloader>& downloader)
    : AssetLoader(std::move(urlSchemes)), outputType(outputType), downloader(downloader) {}

snap::valdi_core::AssetOutputType MockedAssetLoaderWithDownloader::getOutputType() const {
    return outputType;
}

Result<Value> MockedAssetLoaderWithDownloader::requestPayloadFromURL(const StringBox& url) {
    return Value(url);
}

Shared<snap::valdi_core::Cancelable> MockedAssetLoaderWithDownloader::loadAsset(
    const Value& requestPayload,
    int32_t preferredWidth,
    int32_t preferredHeight,
    const Value& filter,
    const Ref<AssetLoaderCompletion>& completion) {
    return downloader->downloadItem(requestPayload.toStringBox(), [completion](auto result) {
        if (result) {
            Ref<LoadedAsset> asset = makeShared<StandaloneLoadedAsset>(result.value(), 0, 0);
            completion->onLoadComplete(asset);
        } else {
            completion->onLoadComplete(result.moveError());
        }
    });
}

MockAssetLoaderFactory::MockAssetLoaderFactory(snap::valdi_core::AssetOutputType outputType) : outputType(outputType) {}

snap::valdi_core::AssetOutputType MockAssetLoaderFactory::getOutputType() const {
    return outputType;
}

Ref<AssetLoader> MockAssetLoaderFactory::createAssetLoader(const std::vector<StringBox>& urlSchemes,
                                                           const Ref<IRemoteDownloader>& downloader) {
    return makeShared<MockedAssetLoaderWithDownloader>(outputType, urlSchemes, downloader);
}

} // namespace ValdiTest
