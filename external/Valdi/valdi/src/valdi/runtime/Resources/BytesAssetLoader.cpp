//
//  BytesAssetLoader.cpp
//  valdi
//
//  Created by Ramzy Jaber on 3/18/22.
//

#include "valdi/runtime/Resources/BytesAssetLoader.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/runtime/Resources/BytesAsset.hpp"
#include "valdi/runtime/Resources/Remote/RemoteDownloader.hpp"
#include "valdi/runtime/Utils/BytesUtils.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "valdi_core/cpp/Utils/SimpleAtomicCancelable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

BytesAssetLoader::BytesAssetLoader(const Ref<IDiskCache>& diskCache,
                                   const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                                   const Ref<DispatchQueue>& workerQueue,
                                   ILogger& logger)
    : AssetLoader({
          STRING_LITERAL("http"),
          STRING_LITERAL("https"),
          STRING_LITERAL("file"),
          STRING_LITERAL("data"),
      }),
      _downloader(makeShared<RemoteDownloader>(diskCache, requestManager, workerQueue, logger)) {}

BytesAssetLoader::~BytesAssetLoader() = default;

snap::valdi_core::AssetOutputType BytesAssetLoader::getOutputType() const {
    return snap::valdi_core::AssetOutputType::Bytes;
}

Result<Value> BytesAssetLoader::requestPayloadFromURL(const StringBox& url) {
    return Value(url);
}

static StringBox makeLocalFilename(const StringBox& url) {
    auto schemeIndex = url.find("://");
    if (schemeIndex) {
        return url.substring(schemeIndex.value() + 3);
    }

    if (url.hasPrefix("data:image/")) {
        size_t extensionStart = std::string_view("data:image/").size();
        auto extensionEnd = url.indexOf(';');

        StringBox extension;
        if (extensionEnd) {
            extension = url.substring(extensionStart, extensionEnd.value());
        }

        auto url_hash = BytesUtils::sha256String(reinterpret_cast<const Byte*>(url.getCStr()), url.length());
        return STRING_FORMAT("{}.{}", url_hash, extension);
    }

    return url;
}

Shared<snap::valdi_core::Cancelable> BytesAssetLoader::loadAsset(const Value& requestPayload,
                                                                 int32_t preferredWidth,
                                                                 int32_t preferredHeight,
                                                                 const Value& associatedData,
                                                                 const Ref<AssetLoaderCompletion>& completion) {
    class BytesAssetTransfomer : public IRemoteDownloaderItemHandler {
    public:
        ~BytesAssetTransfomer() override = default;

        Result<Value> transform(const StringBox& localFilename, const BytesView& data) const override {
            return Value(Valdi::makeShared<BytesAsset>(data));
        }

        std::string_view getItemTypeDescription() const override {
            return "bytes asset";
        }
    };

    static BytesAssetTransfomer transformer;

    auto url = requestPayload.toStringBox();

    StringBox fileName = makeLocalFilename(url);

    auto cancel = Valdi::makeShared<SimpleAtomicCancelable>();

    _downloader->enqueue(fileName,
                         url,
                         transformer,
                         BytesView(),
                         [url, completion, weakThis = Valdi::weakRef(this), cancel](auto result, auto /*loadSource*/) {
                             Result<Ref<LoadedAsset>> retval;

                             if (result.failure()) {
                                 retval = result.error();
                             } else {
                                 retval = result.value().template getTypedRef<LoadedAsset>();
                                 if (auto strongThis = weakThis.lock()) {
                                     strongThis->_downloader->removeItem(url);
                                 }
                             }

                             if (!cancel->wasCanceled()) {
                                 completion->onLoadComplete(retval);
                             }
                         });

    return cancel.toShared();
}

} // namespace Valdi
