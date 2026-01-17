//
//  AssetLocation.hpp
//  valdi
//
//  Created by Simon Corsin on 6/29/21.
//

#pragma once

#include "valdi_core/cpp/Utils/URL.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

/**
 Represents the resolved physical location of an asset
 */
class AssetLocation : public URL {
public:
    AssetLocation(const StringBox& url, bool isLocal);
    ~AssetLocation();

    bool isLocal() const;

    bool operator==(const AssetLocation& other) const;
    bool operator!=(const AssetLocation& other) const;

private:
    bool _isLocal;
};

} // namespace Valdi
