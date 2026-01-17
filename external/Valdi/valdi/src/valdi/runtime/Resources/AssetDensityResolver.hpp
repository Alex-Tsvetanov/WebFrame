//
//  AssetDensityResolver.hpp
//  valdi
//
//  Created by Simon Corsin on 2/9/22.
//

#pragma once

#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include <optional>

namespace Valdi {

class AssetDensityResolver {
public:
    AssetDensityResolver();
    ~AssetDensityResolver();

    void appendDensity(double density);

    std::optional<size_t> select(double density) const;

private:
    SmallVector<double, 8> _densities;
};

} // namespace Valdi
