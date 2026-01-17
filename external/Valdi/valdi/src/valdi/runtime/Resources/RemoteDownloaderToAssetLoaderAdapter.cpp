#include "valdi/runtime/Resources/RemoteDownloaderToAssetLoaderAdapter.hpp"
#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/runtime/Resources/BytesAsset.hpp"

namespace Valdi {

RemoteDownloaderToAssetLoaderAdapter::RemoteDownloaderToAssetLoaderAdapter(
    const Ref<IRemoteDownloader>& remoteDownloader, std::vector<StringBox> supportedSchemes)
    : AssetLoader(std::move(supportedSchemes)), _remoteDownloader(remoteDownloader) {}
RemoteDownloaderToAssetLoaderAdapter::~RemoteDownloaderToAssetLoaderAdapter() = default;

snap::valdi_core::AssetOutputType RemoteDownloaderToAssetLoaderAdapter::getOutputType() const {
    return snap::valdi_core::AssetOutputType::Bytes;
}

Result<Value> RemoteDownloaderToAssetLoaderAdapter::requestPayloadFromURL(const StringBox& url) {
    return Value(url);
}

Shared<snap::valdi_core::Cancelable> RemoteDownloaderToAssetLoaderAdapter::loadAsset(
    const Value& requestPayload,
    int32_t preferredWidth,
    int32_t preferredHeight,
    const Value& associatedData,
    const Ref<AssetLoaderCompletion>& completion) {
    return _remoteDownloader->downloadItem(requestPayload.toStringBox(), [completion](auto result) {
        if (result) {
            Ref<LoadedAsset> asset = makeShared<BytesAsset>(result.value());
            completion->onLoadComplete(asset);
        } else {
            completion->onLoadComplete(result.moveError());
        }
    });
}

} // namespace Valdi
