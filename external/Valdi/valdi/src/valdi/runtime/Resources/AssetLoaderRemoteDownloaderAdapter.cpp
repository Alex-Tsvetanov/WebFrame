//
//  AssetLoaderRemoteDownloaderAdapter.cpp
//  valdi
//
//  Created by Simon Corsin on 7/19/22.
//

#include "valdi/runtime/Resources/AssetLoaderRemoteDownloaderAdapter.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

namespace Valdi {

class AssetLoaderCompletionBridge : public AssetLoaderCompletion {
public:
    explicit AssetLoaderCompletionBridge(Function<void(const Result<BytesView>&)>&& completion)
        : _completion(std::move(completion)) {}
    ~AssetLoaderCompletionBridge() override = default;

    void onLoadComplete(const Result<Ref<LoadedAsset>>& result) override {
        if (result) {
            auto content = result.value()->getBytesContent();
            if (!content) {
                _completion(content.error());
                return;
            }

            _completion(content.value());
        } else {
            _completion(result.error());
        }
    }

private:
    Function<void(const Result<BytesView>&)> _completion;
};

AssetLoaderRemoteDownloaderAdapter::AssetLoaderRemoteDownloaderAdapter(const Ref<AssetLoader>& assetLoader)
    : _assetLoader(assetLoader) {}

AssetLoaderRemoteDownloaderAdapter::~AssetLoaderRemoteDownloaderAdapter() = default;

const Ref<AssetLoader>& AssetLoaderRemoteDownloaderAdapter::getAssetLoader() const {
    return _assetLoader;
}

Shared<snap::valdi_core::Cancelable> AssetLoaderRemoteDownloaderAdapter::downloadItem(
    const StringBox& url, Function<void(const Result<BytesView>&)> completion) {
    auto requestPayload = _assetLoader->requestPayloadFromURL(url);
    if (!requestPayload) {
        completion(requestPayload.error());
        return nullptr;
    }

    auto assetLoadCompletion = makeShared<AssetLoaderCompletionBridge>(std::move(completion));
    return _assetLoader->loadAsset(requestPayload.value(), 0, 0, Value(), assetLoadCompletion);
}

} // namespace Valdi
