//
//  AssetResolver.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/25/19.
//

#include "valdi/runtime/Resources/AssetResolver.hpp"

#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi/runtime/Context/ViewManagerContext.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi/runtime/Resources/AssetsManager.hpp"

#include "valdi_core/cpp/Resources/Asset.hpp"

#include "valdi/runtime/Resources/ResourceManager.hpp"
#include "valdi/runtime/Runtime.hpp"

namespace Valdi {

StringBox static sanitizeAssetName(const StringBox& assetName) {
    return assetName.trimmed().replacing('-', '_');
}

static Ref<Asset> resolveFromModuleAndAssetName(ResourceManager& resourceManager,
                                                const Ref<AssetsManager>& assetsManager,
                                                const StringBox& moduleName,
                                                const StringBox& assetName) {
    auto module = resourceManager.getBundle(moduleName);

    return assetsManager->getAsset(AssetKey(module, sanitizeAssetName(assetName)));
}

Ref<Asset> AssetResolver::resolve(ResourceManager& resourceManager, const Value& value) {
    if (value.isValdiObject()) {
        return castOrNull<Asset>(value.getValdiObject());
    }

    auto str = value.toStringBox();

    const auto& assetsManager = resourceManager.getAssetsManager();

    if (AssetsManager::isAssetUrl(str)) {
        return assetsManager->getAsset(AssetKey(str));
    }

    auto moduleSeparator = str.indexOf(':');
    if (moduleSeparator) {
        // Module is explicitly specified
        auto pair = str.split(*moduleSeparator);
        return resolveFromModuleAndAssetName(resourceManager, assetsManager, pair.first.trimmed(), pair.second);
    }

    return nullptr;
}

Result<Ref<Asset>> AssetResolver::resolve(ViewNode& viewNode, const Value& value) {
    auto* viewNodeTree = viewNode.getViewNodeTree();
    if (viewNodeTree == nullptr) {
        return Error("Cannot resolve asset without an attached ViewNodeTree");
    }

    auto& resourceManager = viewNodeTree->getRuntime()->getResourceManager();
    auto asset = resolve(resourceManager, value);
    if (asset != nullptr) {
        return asset;
    }

    auto cssDocument = viewNode.getCSSDocument();
    if (cssDocument == nullptr) {
        return Error("Cannot resolve relative asset without a document");
    }

    return resolveFromModuleAndAssetName(resourceManager,
                                         resourceManager.getAssetsManager(),
                                         cssDocument->getResourceId().bundleName,
                                         value.toStringBox());
}

} // namespace Valdi
