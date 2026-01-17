//
//  SpinnerLayer.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/15/20.
//

#include "snap_drawing/cpp/Layers/SpinnerLayer.hpp"
#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace snap::drawing {

constexpr TimeInterval kLineThicknessAnimation = 0.5;
constexpr TimeInterval kRotationAnimation = 1.0;
constexpr Scalar kLineThickness = 1.5;
constexpr Scalar kLineSweepRatio = 0.6;
constexpr Scalar kInnerCircleOffset = 3.0;
constexpr Scalar kInnerCircleRotationOffset = 0.5;

class SpinnerAnimation : public IAnimation {
public:
    SpinnerAnimation() = default;
    ~SpinnerAnimation() override = default;

    bool run(Layer& layer, Duration delta) override {
        if (!_started) {
            _started = true;
            _animationTime = 0;
        } else {
            _animationTime += delta.seconds();
        }

        auto lineThicknessCompletion = std::min(1.0, _animationTime / kLineThicknessAnimation);
        auto rotationAnimation = (fmod(_animationTime, kRotationAnimation)) / kRotationAnimation;

        update(layer, lineThicknessCompletion, rotationAnimation);

        // The animation never completes
        return false;
    }

    void cancel(Layer& layer) override {
        _started = false;
        update(layer, 0.0, 0.0);
    }

    void complete(Layer& layer) override {
        _started = false;
        update(layer, 0.0, 0.0);
    }

    void addCompletion(AnimationCompletion&& completion) override {}

private:
    bool _started = false;
    TimeInterval _animationTime = 0;

    static void update(Layer& layer, double lineThicknessRatio, double rotationRatio) {
        auto& spinnerLayer = dynamic_cast<SpinnerLayer&>(layer);

        spinnerLayer.setLineThicknessRatio(static_cast<Scalar>(lineThicknessRatio));
        spinnerLayer.setRotationRatio(static_cast<Scalar>(rotationRatio));
    }
};

SpinnerLayer::SpinnerLayer(const Ref<Resources>& resources)
    : Layer(resources),
      _spinnerAnimation(Valdi::makeShared<SpinnerAnimation>()),
      _circleBorder(BorderRadius::makeCircle()) {
    _outerCirclePaint.setStroke(true);
    _outerCirclePaint.setColor(_color);
    _outerCirclePaint.setAntiAlias(true);

    _innerCirclePaint.setStroke(true);
    _innerCirclePaint.setColor(_color);
    _innerCirclePaint.setAntiAlias(true);
}

SpinnerLayer::~SpinnerLayer() = default;

static void drawArc(
    DrawingContext& drawingContext, const Rect& arcBounds, const Paint& paint, LazyPath& lazyPath, Scalar rotation) {
    auto drawWidth = drawingContext.drawBounds().width();
    auto drawHeight = drawingContext.drawBounds().height();

    auto saveCount = drawingContext.save();

    Matrix matrix;
    matrix.postRotate(rotation, drawWidth / 2.0f, drawHeight / 2.0f);
    drawingContext.concat(matrix);

    if (lazyPath.update(drawWidth, drawHeight)) {
        lazyPath.path().arcTo(arcBounds, 360.0f, 360.0f * kLineSweepRatio);
    }

    drawingContext.drawPaint(paint, lazyPath.path());

    drawingContext.restore(saveCount);
}

void SpinnerLayer::onDraw(DrawingContext& drawingContext) {
    Layer::onDraw(drawingContext);

    auto pi2 = M_PI * 2.0f;

    drawArc(drawingContext, drawingContext.drawBounds(), _outerCirclePaint, _outerCirclePath, pi2 * _rotationRatio);

    auto innerCircleBounds = drawingContext.drawBounds();

    innerCircleBounds.offset(kInnerCircleOffset, kInnerCircleOffset);
    innerCircleBounds.right -= kInnerCircleOffset * 2;
    innerCircleBounds.bottom -= kInnerCircleOffset * 2;

    if (!innerCircleBounds.isEmpty()) {
        drawArc(drawingContext,
                innerCircleBounds,
                _innerCirclePaint,
                _innerCirclePath,
                pi2 - fmod(pi2 * (_rotationRatio + kInnerCircleRotationOffset), pi2));
    }
}

void SpinnerLayer::onRootChanged(ILayerRoot* root) {
    Layer::onRootChanged(root);

    static auto kAnimationKey = STRING_LITERAL("spinner");

    if (root != nullptr && !hasAnimation(kAnimationKey)) {
        addAnimation(kAnimationKey, _spinnerAnimation);
    }
}

void SpinnerLayer::setLineThicknessRatio(Scalar lineThicknessRatio) {
    if (_lineThicknessRatio != lineThicknessRatio) {
        _lineThicknessRatio = lineThicknessRatio;

        _innerCirclePaint.setStrokeWidth(kLineThickness * lineThicknessRatio);
        _outerCirclePaint.setStrokeWidth(kLineThickness * lineThicknessRatio);
        setNeedsDisplay();
    }
}

void SpinnerLayer::setRotationRatio(Scalar rotationRatio) {
    if (_rotationRatio != rotationRatio) {
        _rotationRatio = rotationRatio;
        setNeedsDisplay();
    }
}

void SpinnerLayer::setColor(Color color) {
    if (_color != color) {
        _color = color;
        _innerCirclePaint.setColor(color);
        _outerCirclePaint.setColor(color);

        setNeedsDisplay();
    }
}

} // namespace snap::drawing
