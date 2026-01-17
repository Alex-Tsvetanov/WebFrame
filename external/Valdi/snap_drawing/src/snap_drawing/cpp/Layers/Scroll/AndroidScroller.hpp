//
//  AndroidScroller.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/21/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/Scroll/IScroller.hpp"

namespace snap::drawing {

class AndroidFlingAnimation;
class AndroidContentOffsetAnimation;
class Resources;

/**
 An IScroller implementation that replicates Android's scroll physics.
 */
class AndroidScroller : public IScroller {
public:
    explicit AndroidScroller(const Ref<Resources>& resources);
    ~AndroidScroller() override;

    void reset() override;

    void onDrag(GestureRecognizerState state, const DragEvent& event) override;
    Point computeDecelerationFinalOffset(const Point& contentOffset,
                                         const Vector& velocity,
                                         const Size& pageSize,
                                         bool fast) override;

    Ref<BaseScrollLayerAnimation> fling(const Point& sourceContentOffset, const Vector& velocity, bool fast) override;
    Ref<BaseScrollLayerAnimation> animate(const Point& sourceContentOffset,
                                          const Point& targetContentOffset,
                                          bool fast) override;

private:
    friend AndroidFlingAnimation;
    friend AndroidContentOffsetAnimation;

    Vector _carriedVelocity;
    Scalar _scrollFriction = 0.0f;

    void updateCarriedVelocity(Vector currentVelocity);
};

} // namespace snap::drawing
