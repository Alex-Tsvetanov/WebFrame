//
//  AssetCatalog.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/14/20.
//

#include "valdi/runtime/Resources/AssetCatalog.hpp"
#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Utils/Parser.hpp"

namespace Valdi {

struct AssetDataLegacy {
    uint32_t width;
    uint32_t height;
};

struct AssetData {
    uint32_t width;
    uint32_t height;
    uint32_t type;
};

static constexpr uint32_t kAssetTypeImage = 1;
static constexpr uint32_t kAssetTypeFont = 2;

AssetSpecs::AssetSpecs(const StringBox& name, int width, int height, AssetSpecsType type)
    : _name(name), _width(width), _height(height), _type(type) {}

const StringBox& AssetSpecs::getName() const {
    return _name;
}

int AssetSpecs::getWidth() const {
    return _width;
}

int AssetSpecs::getHeight() const {
    return _height;
}

AssetSpecsType AssetSpecs::getType() const {
    return _type;
}

AssetCatalog::AssetCatalog(std::vector<AssetSpecs> assets) : _assets(std::move(assets)) {
    for (size_t i = 0; i < _assets.size(); i++) {
        _assetIndexByName[_assets[i].getName()] = i;
    }
}

std::optional<AssetSpecs> AssetCatalog::getAssetSpecsForName(const StringBox& name) const {
    const auto& it = _assetIndexByName.find(name);
    if (it == _assetIndexByName.end()) {
        return std::nullopt;
    }

    return {_assets[it->second]};
}

Result<Ref<AssetCatalog>> AssetCatalog::parse(const Byte* data, size_t length) {
    ValdiArchive archive(data, data + length);

    auto entries = archive.getEntries();
    if (!entries) {
        return entries.moveError();
    }

    std::vector<AssetSpecs> assets;

    for (const auto& entry : entries.value()) {
        auto parser = Parser(entry.data, entry.data + entry.dataLength);

        AssetData assetData;
        if (entry.dataLength >= sizeof(AssetData)) {
            auto result = parser.parseStruct<AssetData>();
            if (!result) {
                return result.moveError();
            }
            std::memcpy(&assetData, reinterpret_cast<const void*>(result.value()), sizeof(AssetData));
        } else {
            auto result = parser.parseStruct<AssetDataLegacy>();
            if (!result) {
                return result.moveError();
            }
            std::memcpy(&assetData, reinterpret_cast<const void*>(result.value()), sizeof(AssetDataLegacy));
            assetData.type = kAssetTypeImage;
        }

        auto type = AssetSpecsType::UNKNOWN;
        if (assetData.type == kAssetTypeImage) {
            type = AssetSpecsType::IMAGE;
        } else if (assetData.type == kAssetTypeFont) {
            type = AssetSpecsType::FONT;
        }

        assets.emplace_back(
            entry.filePath, static_cast<int>(assetData.width), static_cast<int>(assetData.height), type);
    }

    return Valdi::makeShared<AssetCatalog>(std::move(assets));
}

const std::vector<AssetSpecs>& AssetCatalog::getAssets() const {
    return _assets;
}

} // namespace Valdi
