//
//  SpinnerLayer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/15/20.
//

#pragma once

#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Utils/LazyPath.hpp"

namespace snap::drawing {

class SpinnerAnimation;

class SpinnerLayer : public Layer {
public:
    explicit SpinnerLayer(const Ref<Resources>& resources);
    ~SpinnerLayer() override;

    void setColor(Color color);
    void setLineThicknessRatio(Scalar lineThicknessRatio);
    void setRotationRatio(Scalar rotationRatio);

protected:
    void onRootChanged(ILayerRoot* root) override;
    void onDraw(DrawingContext& drawingContext) override;

private:
    Ref<SpinnerAnimation> _spinnerAnimation;
    Scalar _lineThicknessRatio = 0.0f;
    Scalar _rotationRatio = 0.0f;
    Color _color = Color::white();

    BorderRadius _circleBorder;

    Paint _outerCirclePaint;
    Paint _innerCirclePaint;
    LazyPath _outerCirclePath;
    LazyPath _innerCirclePath;
};

} // namespace snap::drawing
