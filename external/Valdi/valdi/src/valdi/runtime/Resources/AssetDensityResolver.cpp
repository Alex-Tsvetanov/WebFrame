//
//  AssetDensityResolver.cpp
//  valdi
//
//  Created by Simon Corsin on 2/9/22.
//

#include "valdi/runtime/Resources/AssetDensityResolver.hpp"
#include <limits>

namespace Valdi {

AssetDensityResolver::AssetDensityResolver() = default;
AssetDensityResolver::~AssetDensityResolver() = default;

void AssetDensityResolver::appendDensity(double density) {
    _densities.emplace_back(density);
}

std::optional<size_t> AssetDensityResolver::select(double desiredDensity) const {
    double bestDensity = 0;
    double bestDensityDistance = std::numeric_limits<double>::max();
    std::optional<size_t> bestIndex;

    for (size_t index = 0; index < _densities.size(); index++) {
        auto density = _densities[index];
        auto densityDistance = std::abs(desiredDensity - density);

        // We take the density that matches the most closely our device screen density.
        // If two densities are equally distant to the optimal distance, we take the smallest one.
        if (!bestIndex || densityDistance < bestDensityDistance ||
            (densityDistance == bestDensityDistance && density < bestDensity)) {
            bestDensity = density;
            bestDensityDistance = densityDistance;
            bestIndex = {index};
        }
    }

    return bestIndex;
}

} // namespace Valdi
