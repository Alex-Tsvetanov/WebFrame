//
//  BaseScrollLayerAnimation.cpp
//  snap_drawing-ios
//
//  Created by Simon Corsin on 11/18/22.
//

#include "snap_drawing/cpp/Layers/Scroll/BaseScrollLayerAnimation.hpp"
#include "snap_drawing/cpp/Layers/ScrollLayer.hpp"

namespace snap::drawing {

BaseScrollLayerAnimation::BaseScrollLayerAnimation() = default;
BaseScrollLayerAnimation::~BaseScrollLayerAnimation() = default;

bool BaseScrollLayerAnimation::run(Layer& layer, Duration delta) {
    auto& scrollLayer = dynamic_cast<ScrollLayer&>(layer);

    if (!_started) {
        _started = true;
        // delta is currently not always correct on the first animation frame
        // TODO(simon): Fix this inside the FrameScheduler.
        return false;
    }

    return update(scrollLayer, delta);
}

void BaseScrollLayerAnimation::cancel(Layer& layer) {
    auto& scrollLayer = dynamic_cast<ScrollLayer&>(layer);
    scrollLayer.onScrollAnimationEnded();
}

void BaseScrollLayerAnimation::complete(Layer& layer) {
    auto& scrollLayer = dynamic_cast<ScrollLayer&>(layer);
    scrollLayer.onScrollAnimationEnded();
}

void BaseScrollLayerAnimation::addCompletion(AnimationCompletion&& /*completion*/) {
    // not implemented. Not needed for now
}

Point BaseScrollLayerAnimation::clampContentOffset(ScrollLayer& scrollLayer, Point contentOffset) {
    auto clampedOffsetX = scrollLayer.clampContentOffsetX(contentOffset.x);
    auto clamedOpffsetY = scrollLayer.clampContentOffsetY(contentOffset.y);

    return Point::make(clampedOffsetX, clamedOpffsetY);
}

Vector BaseScrollLayerAnimation::applyContentOffset(ScrollLayer& scrollLayer, Point contentOffset) {
    return scrollLayer.applyContentOffset(contentOffset.x, contentOffset.y, Vector::makeEmpty());
}

} // namespace snap::drawing
