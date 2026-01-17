//
//  Path.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#include "snap_drawing/cpp/Utils/Path.hpp"
#include "include/core/SkStream.h"
#include "include/pathops/SkPathOps.h"
#include "snap_drawing/cpp/Utils/Matrix.hpp"

namespace snap::drawing {

Path::Path() = default;
Path::Path(Path&& other) noexcept : WrappedSkValue<SkPath>(std::move(other.getSkValue())) {}

Path::Path(const Path& other) noexcept : WrappedSkValue<SkPath>(other.getSkValue()) {}

Path::~Path() = default;

static SkPathDirection getPathDirection(bool clockwise) {
    return clockwise ? SkPathDirection::kCW : SkPathDirection::kCCW;
}

Path& Path::operator=(Path&& other) noexcept {
    if (&other != this) {
        getSkValue() = std::move(other.getSkValue());
    }

    return *this;
}

Path& Path::operator=(const Path& other) noexcept {
    getSkValue() = other.getSkValue();
    return *this;
}

bool Path::operator==(const Path& other) const {
    return getSkValue() == other.getSkValue();
}

bool Path::operator!=(const Path& other) const {
    return getSkValue() != other.getSkValue();
}

void Path::addRoundRect(const Rect& bounds, Scalar radii[8], bool clockwise) {
    getSkValue().addRoundRect(bounds.getSkValue(), &radii[0], getPathDirection(clockwise));
}

void Path::addRoundRect(const Rect& bounds, Scalar rx, Scalar ry, bool clockwise) {
    getSkValue().addRoundRect(bounds.getSkValue(), rx, ry, getPathDirection(clockwise));
}

bool Path::isEmpty() const {
    return getSkValue().isEmpty();
}

void Path::reset() {
    getSkValue().reset();
}

void Path::moveTo(Scalar x, Scalar y) {
    getSkValue().moveTo(x, y);
}

void Path::lineTo(Scalar x, Scalar y) {
    getSkValue().lineTo(x, y);
}

void Path::quadTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1) {
    getSkValue().quadTo(x0, y0, x1, y1);
}

void Path::conicTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1, Scalar weight) {
    getSkValue().conicTo(x0, y0, x1, y1, weight);
}

void Path::cubicTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1, Scalar x2, Scalar y2) {
    getSkValue().cubicTo(x0, y0, x1, y1, x2, y2);
}

void Path::close() {
    getSkValue().close();
}

void Path::addRect(const Rect& bounds, bool clockwise) {
    getSkValue().addRect(bounds.getSkValue(), getPathDirection(clockwise));
}

void Path::addOval(const Rect& bounds, bool clockwise) {
    getSkValue().addOval(bounds.getSkValue(), getPathDirection(clockwise));
}

void Path::arcTo(const Rect& oval, Scalar startAngle, Scalar sweepAngle) {
    getSkValue().addArc(oval.getSkValue(), startAngle, sweepAngle);
}

void Path::addPath(const Path& path) {
    getSkValue().addPath(path.getSkValue());
}

void Path::transform(const Matrix& matrix) {
    getSkValue().transform(matrix.getSkValue());
}

Path Path::intersection(const Path& other) const {
    Path output;
    Op(getSkValue(), other.getSkValue(), kIntersect_SkPathOp, &output.getSkValue());

    output.getSkValue().setFillType(getSkValue().getFillType());
    return output;
}

std::optional<Rect> Path::getBounds() const {
    if (isEmpty()) {
        return std::nullopt;
    }
    return {fromSkValue<Rect>(getSkValue().getBounds())};
}

std::string Path::toString() const {
    SkDynamicMemoryWStream stream;
    getSkValue().dump(&stream, true);

    auto bytes = stream.bytesWritten();

    std::string output;
    output.resize(bytes);

    stream.copyTo(output.data());

    return output;
}

void Path::visit(PathVisitor& visitor) const {
    SkPath::Iter it(getSkValue(), false);
    SkPoint points[4];
    SkPoint quads[5];

    for (;;) {
        auto verb = it.next(points);

        switch (verb) {
            case SkPath::kMove_Verb:
                visitor.move(fromSkValue<Point>(points[0]));
                break;
            case SkPath::kLine_Verb:
                visitor.line(fromSkValue<Point>(points[0]), fromSkValue<Point>(points[1]));
                break;
            case SkPath::kQuad_Verb:
                visitor.quad(
                    fromSkValue<Point>(points[0]), fromSkValue<Point>(points[1]), fromSkValue<Point>(points[2]));
                break;
            case SkPath::kConic_Verb: {
                if (visitor.shouldConvertConicToQuad()) {
                    SkPath::ConvertConicToQuads(points[0], points[1], points[2], it.conicWeight(), quads, 1);

                    visitor.quad(
                        fromSkValue<Point>(quads[0]), fromSkValue<Point>(quads[1]), fromSkValue<Point>(quads[2]));
                    visitor.quad(
                        fromSkValue<Point>(quads[2]), fromSkValue<Point>(quads[3]), fromSkValue<Point>(quads[4]));
                } else {
                    visitor.conic(fromSkValue<Point>(points[0]),
                                  fromSkValue<Point>(points[1]),
                                  fromSkValue<Point>(points[2]),
                                  it.conicWeight());
                }
            } break;
            case SkPath::kCubic_Verb:
                visitor.cubic(fromSkValue<Point>(points[0]),
                              fromSkValue<Point>(points[1]),
                              fromSkValue<Point>(points[2]),
                              fromSkValue<Point>(points[3]));
                break;
            case SkPath::kClose_Verb:
                visitor.close();
                break;
            case SkPath::kDone_Verb:
                return;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const Path& path) {
    return os << path.toString();
}

} // namespace snap::drawing
