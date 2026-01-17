//
//  SpringFlingScrollLayerAnimation.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/28/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/Scroll/BaseScrollLayerAnimation.hpp"
#include "snap_drawing/cpp/Layers/Scroll/SpringScrollPhysics.hpp"

#include <optional>

namespace snap::drawing {

class SpringFlingScrollLayerAnimation : public BaseScrollLayerAnimation {
public:
    SpringFlingScrollLayerAnimation();
    ~SpringFlingScrollLayerAnimation() override;

protected:
    bool update(ScrollLayer& scrollLayer, Duration delta) final;

    virtual bool onDecelerate(ScrollLayer& scrollLayer, Duration elapsed) = 0;
    virtual void updateCarriedVelocity(const Vector& velocity) = 0;

    bool startBouncing(ScrollLayer& scrollLayer,
                       const Vector& velocity,
                       const Point& sourceContentOffset,
                       const Point& targetContentOffset,
                       const Duration& startTime,
                       const SpringScrollPhysicsConfiguration* scrollPhysicsConfiguration);

private:
    Duration _elapsed;
    std::optional<SpringScrollPhysics> _springScrollPhysics;
    Point _targetContentOffset;

    bool onBounce(ScrollLayer& scrollLayer);
};

} // namespace snap::drawing
