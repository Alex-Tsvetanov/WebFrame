//
//  CoreGraphicsUtils.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/14/22.
//

#include "snap_drawing/apple/CoreGraphicsUtils.hpp"
#include <optional>

namespace snap::drawing {

struct PathToCGPathVisitor : public PathVisitor {
    CGMutablePathRef pathRef;

    PathToCGPathVisitor(CGMutablePathRef pathRef) : pathRef(pathRef) {}

    bool shouldConvertConicToQuad() const override {
        return true;
    }

    void move(const Point& point) override {
        CGPathMoveToPoint(pathRef, nil, point.x, point.y);
    }

    void line(const Point& from, const Point& to) override {
        CGPathAddLineToPoint(pathRef, nil, to.x, to.y);
    }

    void quad(const Point& p1, const Point& p2, const Point& p3) override {
        CGPathAddQuadCurveToPoint(pathRef, nil, p2.x, p2.y, p3.x, p3.y);
    }

    void conic(const Point& p1, const Point& p2, const Point& p3, Scalar weight) override {}

    void cubic(const Point& p1, const Point& p2, const Point& p3, const Point& p4) override {
        // TODO(simon): This is untested
        CGPathAddCurveToPoint(pathRef, nil, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
    }

    void close() override {}
};

CGPathRef cgPathFromPath(const Path& path) {
    CGMutablePathRef clipPathRef = CGPathCreateMutable();

    PathToCGPathVisitor visitor(clipPathRef);
    path.visit(visitor);

    return static_cast<CGPathRef>(clipPathRef);
}

CGAffineTransform cgAffineTransformFromMatrix(const Matrix& matrix) {
    Scalar affines[6];
    if (!matrix.toAffine(affines)) {
        return CGAffineTransformIdentity;
    }

    return CGAffineTransformMake(affines[0], affines[1], affines[2], affines[3], affines[4], affines[5]);
}

CATransform3D caTransform3DFromMatrix(const Matrix& matrix) {
    CGAffineTransform affineTransform = cgAffineTransformFromMatrix(matrix);
    return CATransform3DMakeAffineTransform(affineTransform);
}

} // namespace snap::drawing
