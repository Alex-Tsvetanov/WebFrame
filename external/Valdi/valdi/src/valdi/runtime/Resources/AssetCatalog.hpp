//
//  AssetCatalog.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/14/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <optional>

namespace Valdi {

enum class AssetSpecsType {
    IMAGE,
    FONT,
    UNKNOWN,
};

class AssetSpecs {
public:
    AssetSpecs(const StringBox& name, int width, int height, AssetSpecsType type);

    const StringBox& getName() const;
    int getWidth() const;
    int getHeight() const;

    AssetSpecsType getType() const;

private:
    StringBox _name;
    int _width;
    int _height;
    AssetSpecsType _type;
};

class AssetCatalog : public SimpleRefCountable {
public:
    explicit AssetCatalog(std::vector<AssetSpecs> assets);

    const std::vector<AssetSpecs>& getAssets() const;

    std::optional<AssetSpecs> getAssetSpecsForName(const StringBox& name) const;

    static Result<Ref<AssetCatalog>> parse(const Byte* data, size_t length);

private:
    std::vector<AssetSpecs> _assets;
    FlatMap<StringBox, size_t> _assetIndexByName;
};

} // namespace Valdi
