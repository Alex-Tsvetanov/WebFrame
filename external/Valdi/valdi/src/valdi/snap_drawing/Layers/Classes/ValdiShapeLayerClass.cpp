//
//  ValdiShapeLayerClass.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 3/4/22.
//

#include "valdi/snap_drawing/Layers/Classes/ValdiShapeLayerClass.hpp"
#include "valdi/snap_drawing/Animations/ValdiAnimator.hpp"
#include "valdi/snap_drawing/Utils/AttributesBinderUtils.hpp"

namespace snap::drawing {

ValdiShapeLayerClass::ValdiShapeLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass)
    : ILayerClass(resources, "SCValdiShapeView", "com.snap.valdi.views.ShapeView", parentClass, false) {}
ValdiShapeLayerClass::~ValdiShapeLayerClass() = default;

Valdi::Ref<Layer> ValdiShapeLayerClass::instantiate() {
    return makeLayer<ValdiShapeLayer>(getResources());
}

void ValdiShapeLayerClass::bindAttributes(Valdi::AttributesBindingContext& binder) {
    BIND_UNTYPED_ATTRIBUTE(ValdiShapeLayer, path, false);

    BIND_DOUBLE_ATTRIBUTE(ValdiShapeLayer, strokeWidth, false);
    BIND_COLOR_ATTRIBUTE(ValdiShapeLayer, strokeColor, false);
    BIND_COLOR_ATTRIBUTE(ValdiShapeLayer, fillColor, false);
    BIND_STRING_ATTRIBUTE(ValdiShapeLayer, strokeCap, false);
    BIND_STRING_ATTRIBUTE(ValdiShapeLayer, strokeJoin, false);
    BIND_DOUBLE_ATTRIBUTE(ValdiShapeLayer, strokeStart, false);
    BIND_DOUBLE_ATTRIBUTE(ValdiShapeLayer, strokeEnd, false);
}

IMPLEMENT_UNTYPED_ATTRIBUTE(
    ValdiShapeLayer,
    path,
    {
        view.setPathData(value.getTypedArrayRef());
        return Valdi::Void();
    },
    { view.setPathData(nullptr); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ValdiShapeLayer,
    strokeWidth,
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeWidth,
                                       &ValdiShapeLayer::setStrokeWidth,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       static_cast<Scalar>(value));
        return Valdi::Void();
    },
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeWidth,
                                       &ValdiShapeLayer::setStrokeWidth,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       1.0f);
    })

IMPLEMENT_COLOR_ATTRIBUTE(
    ValdiShapeLayer,
    strokeColor,
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeColor,
                                       &ValdiShapeLayer::setStrokeColor,
                                       MIN_VISIBLE_CHANGE_COLOR,
                                       snapDrawingColorFromValdiColor(value));
        return Valdi::Void();
    },
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeColor,
                                       &ValdiShapeLayer::setStrokeColor,
                                       MIN_VISIBLE_CHANGE_COLOR,
                                       Color::transparent());
    })

IMPLEMENT_COLOR_ATTRIBUTE(
    ValdiShapeLayer,
    fillColor,
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getFillColor,
                                       &ValdiShapeLayer::setFillColor,
                                       MIN_VISIBLE_CHANGE_COLOR,
                                       snapDrawingColorFromValdiColor(value));
        return Valdi::Void();
    },
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getFillColor,
                                       &ValdiShapeLayer::setFillColor,
                                       MIN_VISIBLE_CHANGE_COLOR,
                                       Color::transparent());
    })

IMPLEMENT_STRING_ATTRIBUTE(
    ValdiShapeLayer,
    strokeCap,
    {
        if (value == "butt") {
            view.setStrokeCap(PaintStrokeCapButt);
        } else if (value == "round") {
            view.setStrokeCap(PaintStrokeCapRound);
        } else if (value == "square") {
            view.setStrokeCap(PaintStrokeCapSquare);
        } else {
            return Valdi::Error("Invalid strokeCap");
        }
        return Valdi::Void();
    },
    { view.setStrokeCap(PaintStrokeCapButt); })

IMPLEMENT_STRING_ATTRIBUTE(
    ValdiShapeLayer,
    strokeJoin,
    {
        if (value == "bevel") {
            view.setStrokeJoin(PaintStrokeJoinBevel);
        } else if (value == "miter") {
            view.setStrokeJoin(PaintStrokeJoinMiter);
        } else if (value == "round") {
            view.setStrokeJoin(PaintStrokeJoinRound);
        } else {
            return Valdi::Error("Invalid strokeJoin");
        }
        return Valdi::Void();
    },
    { view.setStrokeCap(PaintStrokeCapButt); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ValdiShapeLayer,
    strokeStart,
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeStart,
                                       &ValdiShapeLayer::setStrokeStart,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       static_cast<Scalar>(value));
        return Valdi::Void();
    },
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeStart,
                                       &ValdiShapeLayer::setStrokeStart,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       0.0f);
    })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ValdiShapeLayer,
    strokeEnd,
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeEnd,
                                       &ValdiShapeLayer::setStrokeEnd,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       static_cast<Scalar>(value));
        return Valdi::Void();
    },
    {
        context.setAnimatableAttribute(static_cast<ShapeLayer&>(view),
                                       &ValdiShapeLayer::getStrokeEnd,
                                       &ValdiShapeLayer::setStrokeEnd,
                                       MIN_VISIBLE_CHANGE_PIXEL,
                                       1.0f);
    })

} // namespace snap::drawing
