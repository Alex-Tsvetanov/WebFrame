//
//  IOSScroller.hpp
//  snap_drawing-ios
//
//  Created by Simon Corsin on 11/18/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/Scroll/IScroller.hpp"

namespace snap::drawing {

class IOSFlingAnimation;
class IOSContentOffsetAnimation;

class IOSScroller : public IScroller {
public:
    IOSScroller();
    ~IOSScroller() override;

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
    Vector _carriedVelocity;
    TimePoint _dragStartTime;

    friend IOSFlingAnimation;
    friend IOSContentOffsetAnimation;

    void updateCarriedVelocity(Vector currentVelocity);

    static Scalar getDecelerationCoef(bool fast);
    static Scalar getDecelerationRate(bool fast);
    static Vector computeDecelerationInitialVelocity(const Point& initialOffset, const Point& finalOffset, bool fast);
    static Duration computeDecelerationDuration(const Vector& velocity, bool fast);

    static Point computeDecelerationOffset(const Point& contentOffset,
                                           const Vector& velocity,
                                           Duration elapsedTime,
                                           bool fast);

    static std::optional<Duration> computeDecelerationTimeAtTargetOffset(const Point& contentOffset,
                                                                         const Vector& velocity,
                                                                         const Point& targetContentOffset,
                                                                         bool fast);

    static std::optional<Duration> computeDecelerationTimeAtTargetOffset(Scalar sourceOffset,
                                                                         Scalar velocity,
                                                                         Scalar targetOffset,
                                                                         bool fast);
};

} // namespace snap::drawing
