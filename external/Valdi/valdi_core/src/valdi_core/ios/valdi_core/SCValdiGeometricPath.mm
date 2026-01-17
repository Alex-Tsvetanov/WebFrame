//
//  SCValdiGeometricPath.m
//  valdi_core-ios
//
//  Created by Simon Corsin on 3/4/22.
//

#import "valdi_core/SCValdiGeometricPath.h"
#import "valdi_core/cpp/Utils/GeometricPath.hpp"

namespace ValdiIOS {

struct PathToCGPathVisitor : public Valdi::GeometricPathVisitor {
public:
    CGMutablePathRef pathRef;

    PathToCGPathVisitor(CGMutablePathRef pathRef) : pathRef(pathRef) {}

    void moveTo(double x, double y) override {
        CGPathMoveToPoint(pathRef, nil, x, y);
    }

    void lineTo(double x, double y) override {
        CGPathAddLineToPoint(pathRef, nil, x, y);
    }

    void quadTo(double controlX, double controlY, double x, double y) override {
        CGPathAddQuadCurveToPoint(pathRef, nil, controlX, controlY, x, y);
    }

    void cubicTo(double controlX1, double controlY1, double controlX2, double controlY2, double x, double y) override {
        CGPathAddCurveToPoint(pathRef, nil, controlX1, controlY1, controlX2, controlY2, x, y);
    }

    void roundRectTo(double x, double y, double width, double height, double radiusX, double radiusY) override {
        CGPathAddRoundedRect(pathRef, nil, CGRectMake(x, y, width, height), radiusX, radiusY);
    }

    void arcTo(double centerX, double centerY, double radiusX, double radiusY, double startAngle, double sweepAngle) override {
        if (radiusX == radiusY) {
            CGPathAddRelativeArc(pathRef, nil, centerX, centerY, radiusX, startAngle, sweepAngle);
        } else {
            auto radius = std::max(radiusX, radiusY);
            auto scaleX = radiusX / radius;
            auto scaleY = radiusY / radius;
            CGAffineTransform transform = CGAffineTransformMakeScale(scaleX, scaleY);
            transform.tx = (radius - radiusX) / 2.0;
            transform.ty = (radius - radiusY) / 2.0;
            CGPathAddRelativeArc(pathRef, &transform, centerX, centerY, radius, startAngle, sweepAngle);
        }
    }

    void close() override {
        CGPathCloseSubpath(pathRef);
    }
};

};

FOUNDATION_EXPORT CGPathRef CGPathFromGeometricPathData(NSData *data, CGSize size) {
    if (!data) {
        return nil;
    }

    CGMutablePathRef pathRef = CGPathCreateMutable();
    ValdiIOS::PathToCGPathVisitor visitor(pathRef);

    Valdi::GeometricPath::visit(reinterpret_cast<const Valdi::Byte *>(data.bytes), data.length, size.width, size.height, visitor);

    return pathRef;
}
