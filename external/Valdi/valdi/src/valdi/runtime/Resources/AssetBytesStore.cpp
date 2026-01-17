
#include "valdi/runtime/Resources/AssetBytesStore.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

AssetBytesStore::AssetBytesStore() = default;

AssetBytesStore::~AssetBytesStore() = default;

static const StringBox& getAssetBytesStoreUrlPrefix() {
    static auto kPrefix = STRING_LITERAL("valdi-byteasset://");
    return kPrefix;
}

StringBox AssetBytesStore::registerAssetBytes(const BytesView& bytes) {
    std::lock_guard<Mutex> lock(_mutex);
    auto id = ++_assetKeyBytesIdSequence;

    auto url = STRING_FORMAT("{}{}", getAssetBytesStoreUrlPrefix(), id);
    _bytesByUrl[url] = bytes;
    return url;
}

void AssetBytesStore::unregisterAssetBytes(const StringBox& url) {
    std::lock_guard<Mutex> lock(_mutex);
    auto it = _bytesByUrl.find(url);
    if (it != _bytesByUrl.end()) {
        _bytesByUrl.erase(it);
    }
}

Shared<snap::valdi_core::Cancelable> AssetBytesStore::downloadItem(
    const StringBox& url, Function<void(const Result<BytesView>&)> completion) {
    Result<BytesView> result;
    {
        std::lock_guard<Mutex> lock(_mutex);
        const auto& it = _bytesByUrl.find(url);
        if (it != _bytesByUrl.end()) {
            result = Result<BytesView>(it->second);
        } else {
            result = Result<BytesView>(Error("Did not find associated bytes with url"));
        }
    }
    completion(result);
    return nullptr;
}

StringBox AssetBytesStore::getUrlScheme() {
    const auto& prefix = getAssetBytesStoreUrlPrefix();
    return prefix.substring(0, prefix.length() - 3); // Remove :// trailing
}

bool AssetBytesStore::isAssetBytesUrl(const StringBox& url) {
    return url.hasPrefix(getAssetBytesStoreUrlPrefix());
}
} // namespace Valdi
