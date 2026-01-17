//
//  BaseScrollLayerAnimation.hpp
//  snap_drawing-ios
//
//  Created by Simon Corsin on 11/18/22.
//

#pragma once

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

namespace snap::drawing {

class ScrollLayer;

class BaseScrollLayerAnimation : public IAnimation {
public:
    BaseScrollLayerAnimation();
    ~BaseScrollLayerAnimation() override;

    bool run(Layer& layer, Duration delta) override;
    void cancel(Layer& layer) override;
    void complete(Layer& layer) override;
    void addCompletion(AnimationCompletion&& completion) override;

protected:
    virtual bool update(ScrollLayer& scrollLayer, Duration delta) = 0;

    static Point clampContentOffset(ScrollLayer& scrollLayer, Point contentOffset);
    static Vector applyContentOffset(ScrollLayer& scrollLayer, Point contentOffset);

private:
    bool _started = false;
};

} // namespace snap::drawing
