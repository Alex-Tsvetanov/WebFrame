//
//  ValdiShapeLayer.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 3/4/22.
//

#include "valdi/snap_drawing/Layers/ValdiShapeLayer.hpp"
#include "valdi/snap_drawing/Utils/ValdiUtils.hpp"

namespace snap::drawing {

ValdiShapeLayer::ValdiShapeLayer(const Ref<Resources>& resources) : ShapeLayer(resources) {}
ValdiShapeLayer::~ValdiShapeLayer() = default;

void ValdiShapeLayer::onBoundsChanged() {
    ShapeLayer::onBoundsChanged();
    _pathDirty = true;
}

void ValdiShapeLayer::onDraw(DrawingContext& drawingContext) {
    if (_pathDirty) {
        _pathDirty = false;
        if (_pathData == nullptr) {
            setPath(Path());
        } else {
            setPath(pathFromValdiGeometricPath(
                _pathData->getBuffer(), drawingContext.drawBounds().width(), drawingContext.drawBounds().height()));
        }
    }

    ShapeLayer::onDraw(drawingContext);
}

void ValdiShapeLayer::setPathData(const Valdi::Ref<Valdi::ValueTypedArray>& pathData) {
    if (_pathData != pathData) {
        _pathData = pathData;
        _pathDirty = true;
        setNeedsDisplay();
    }
}

const Valdi::Ref<Valdi::ValueTypedArray>& ValdiShapeLayer::getPathData() const {
    return _pathData;
}

} // namespace snap::drawing
