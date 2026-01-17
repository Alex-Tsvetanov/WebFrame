//
//  AndroidScrollConstantsResolver.hpp
//  valdi-android
//
//  Created by Simon Corsin on 9/26/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {
class IDiskCache;
class ILogger;
} // namespace Valdi

namespace ValdiAndroid {

struct ScrollConstants {
    float gravity;
    float inflexion;
    float startTension;
    float endTension;
    float decelerationRate;

    constexpr ScrollConstants(
        float gravity, float inflexion, float startTension, float endTension, float decelerationRate)
        : gravity(gravity),
          inflexion(inflexion),
          startTension(startTension),
          endTension(endTension),
          decelerationRate(decelerationRate) {}

    static ScrollConstants resolve(const Valdi::Ref<Valdi::IDiskCache>& diskCache, Valdi::ILogger& logger);
};

} // namespace ValdiAndroid
