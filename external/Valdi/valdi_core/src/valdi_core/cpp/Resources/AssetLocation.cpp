//
//  AssetLocation.cpp
//  valdi
//
//  Created by Simon Corsin on 6/29/21.
//

#include "valdi_core/cpp/Resources/AssetLocation.hpp"

namespace Valdi {

AssetLocation::AssetLocation(const StringBox& url, bool isLocal) : URL(url), _isLocal(isLocal) {}

AssetLocation::~AssetLocation() = default;

bool AssetLocation::isLocal() const {
    return _isLocal;
}

bool AssetLocation::operator==(const AssetLocation& other) const {
    return static_cast<const URL&>(*this) == static_cast<const URL&>(other) && _isLocal == other._isLocal;
}

bool AssetLocation::operator!=(const AssetLocation& other) const {
    return !(*this == other);
}

} // namespace Valdi
