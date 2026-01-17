//
//  AnimationOptions.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/16/19.
//

#pragma once

#include "valdi_core/AnimationType.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

namespace Valdi {

struct AnimationOptions {
    std::vector<double> controlPoints;
    Value completionCallback;
    snap::valdi_core::AnimationType type = snap::valdi_core::AnimationType::EaseInOut;
    double duration = 0;
    bool beginFromCurrentState = false;
    bool crossfade = false;
    double stiffness = 0;
    double damping = 0;
    unsigned int cancelToken = 0;

    AnimationOptions();

    AnimationOptions(snap::valdi_core::AnimationType type,
                     std::vector<double> controlPoints,
                     double duration,
                     bool beginFromCurrentState,
                     Value completionCallback);

    ~AnimationOptions();

    Ref<ValueMap> toValueMap() const;
};

} // namespace Valdi
