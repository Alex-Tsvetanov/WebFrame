//
//  StandaloneAssetLoader.cpp
//  valdi-standalone_runtime
//
//  Created by Simon Corsin on 7/12/21.
//

#include "valdi/standalone_runtime/StandaloneAssetLoader.hpp"

#include "valdi/runtime/Interfaces/IDiskCache.hpp"

#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/standalone_runtime/StandaloneLoadedAsset.hpp"

#include "valdi_core/Platform.hpp"

#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/URL.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class StandaloneAssetLoaderRequestPayload : public ValdiObject {
public:
    explicit StandaloneAssetLoaderRequestPayload(const URL& url) : _url(url) {}

    ~StandaloneAssetLoaderRequestPayload() override = default;

    const URL& getURL() const {
        return _url;
    }

    VALDI_CLASS_HEADER_IMPL(StandaloneAssetLoaderRequestPayload)

private:
    URL _url;
};

StandaloneAssetLoader::StandaloneAssetLoader(IDiskCache& diskCache)
    : AssetLoader({STRING_LITERAL("file")}), _diskCache(diskCache) {}
StandaloneAssetLoader::~StandaloneAssetLoader() = default;

snap::valdi_core::AssetOutputType StandaloneAssetLoader::getOutputType() const {
    return snap::valdi_core::AssetOutputType::Dummy;
}

Valdi::Result<Valdi::Value> StandaloneAssetLoader::requestPayloadFromURL(const Valdi::StringBox& url) {
    URL parsedURL(url);

    return Valdi::Value(makeShared<StandaloneAssetLoaderRequestPayload>(std::move(parsedURL)));
}

Valdi::Shared<snap::valdi_core::Cancelable> StandaloneAssetLoader::loadAsset(
    const Value& requestPayload,
    int32_t preferredWidth,
    int32_t preferredHeight,
    const Value& /*associatedData*/,
    const Ref<AssetLoaderCompletion>& completion) {
    auto payload = requestPayload.getTypedRef<StandaloneAssetLoaderRequestPayload>();

    auto result = _diskCache.load(Path(payload->getURL().getPath()));

    if (!result) {
        completion->onLoadComplete(result.error());
        return nullptr;
    }

    auto asset = makeShared<StandaloneLoadedAsset>(result.value(), preferredWidth, preferredHeight);

    completion->onLoadComplete(Ref<LoadedAsset>(asset));

    return nullptr;
}

} // namespace Valdi
