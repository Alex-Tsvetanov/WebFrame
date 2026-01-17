//
//  AnimationOptions.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/16/19.
//

#include "valdi/runtime/Rendering/AnimationOptions.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

AnimationOptions::AnimationOptions() = default;

AnimationOptions::AnimationOptions(snap::valdi_core::AnimationType type,
                                   std::vector<double> controlPoints,
                                   double duration,
                                   bool beginFromCurrentState,
                                   Value completionCallback)
    : controlPoints(std::move(controlPoints)),
      completionCallback(std::move(completionCallback)),
      type(type),
      duration(duration),
      beginFromCurrentState(beginFromCurrentState) {}

AnimationOptions::~AnimationOptions() = default;

Ref<ValueMap> AnimationOptions::toValueMap() const {
    auto out = makeShared<ValueMap>();

    static auto linear = STRING_LITERAL("linear");
    static auto easeIn = STRING_LITERAL("easeIn");
    static auto easeOut = STRING_LITERAL("easeOut");
    static auto easeInOut = STRING_LITERAL("easeInOut");

    auto typeStr = linear;
    switch (type) {
        case snap::valdi_core::AnimationType::Linear:
            typeStr = linear;
            break;
        case snap::valdi_core::AnimationType::EaseIn:
            typeStr = easeIn;
            break;
        case snap::valdi_core::AnimationType::EaseOut:
            typeStr = easeOut;
            break;
        case snap::valdi_core::AnimationType::EaseInOut:
            typeStr = easeInOut;
            break;
    }

    auto outControlPoints = ValueArray::make(controlPoints.size());

    size_t controlPointsIndex = 0;
    for (const auto& controlPoint : controlPoints) {
        outControlPoints->emplace(controlPointsIndex++, Value(controlPoint));
    }

    static auto type = STRING_LITERAL("type");
    static auto duration = STRING_LITERAL("duration");
    static auto controlPoints = STRING_LITERAL("controlPoints");
    static auto beginFromCurrentState = STRING_LITERAL("beginFromCurrentState");
    static auto crossfade = STRING_LITERAL("crossfade");
    static auto completion = STRING_LITERAL("completion");
    static auto stiffness = STRING_LITERAL("stiffness");
    static auto damping = STRING_LITERAL("damping");

    out->try_emplace(type, typeStr);
    out->try_emplace(duration, this->duration);
    out->try_emplace(controlPoints, outControlPoints);
    out->try_emplace(beginFromCurrentState, this->beginFromCurrentState);
    out->try_emplace(crossfade, this->crossfade);
    out->try_emplace(completion, completionCallback);
    out->try_emplace(stiffness, this->stiffness);
    out->try_emplace(damping, this->damping);

    return out;
}

} // namespace Valdi
