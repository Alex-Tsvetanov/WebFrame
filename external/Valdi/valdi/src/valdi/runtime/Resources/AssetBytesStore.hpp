#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"

namespace Valdi {

class AssetBytesStore : public IRemoteDownloader {
public:
    AssetBytesStore();
    ~AssetBytesStore() override;

    StringBox registerAssetBytes(const BytesView& bytes);
    void unregisterAssetBytes(const StringBox& url);

    Shared<snap::valdi_core::Cancelable> downloadItem(const StringBox& url,
                                                      Function<void(const Result<BytesView>&)> completion) override;

    static StringBox getUrlScheme();
    static bool isAssetBytesUrl(const StringBox& url);

private:
    Mutex _mutex;
    uint64_t _assetKeyBytesIdSequence = 0;
    FlatMap<StringBox, BytesView> _bytesByUrl;
};

} // namespace Valdi
