//
//  IScroller.hpp
//  snap_drawing-ios
//
//  Created by Simon Corsin on 11/18/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/Scroll/BaseScrollLayerAnimation.hpp"
#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

namespace snap::drawing {

/**
 A Scroller is responsible for implementing the underlying scroll physics of a ScrollLayer.
 */
class IScroller : public Valdi::SimpleRefCountable {
public:
    /**
     Reset the state of the scroller.
     Called when the ScrollLayer is removed from the hierarchy.
     */
    virtual void reset() = 0;

    /**
     Called on drag events. The scroller can use this to update its underlying
     carried velocity.
     */
    virtual void onDrag(GestureRecognizerState state, const DragEvent& event) = 0;

    /**
     Computes the target deceleration offset for the given contentOffset at the given velocity.
     This is used on paginated scrolling to find the target range.
     */
    virtual Point computeDecelerationFinalOffset(const Point& contentOffset,
                                                 const Vector& velocity,
                                                 const Size& pageSize,
                                                 bool fast) = 0;

    /**
     Return an animation that will fling the scroll layer from the given source contentOffset with the given
     velocity. The scroller should use its the accumulated velocity if it has any for the fling animation.
     */
    virtual Ref<BaseScrollLayerAnimation> fling(const Point& sourceContentOffset,
                                                const Vector& velocity,
                                                bool fast) = 0;

    /**
     Animate a scroll between the given source contentOffset to the given target contentOffset.
     The scroller should not use its accumulated velocity.
     */
    virtual Ref<BaseScrollLayerAnimation> animate(const Point& sourceContentOffset,
                                                  const Point& targetContentOffset,
                                                  bool fast) = 0;
};

} // namespace snap::drawing
