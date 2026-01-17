//
//  Matrix.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include <fmt/format.h>

namespace snap::drawing {

void Matrix::setIdentity() {
    getSkValue().setIdentity();
}

Scalar Matrix::getTranslateX() const {
    return getSkValue().getTranslateX();
}

void Matrix::setTranslateX(Scalar translateX) {
    getSkValue().setTranslateX(translateX);
}

Scalar Matrix::getTranslateY() const {
    return getSkValue().getTranslateY();
}

void Matrix::setTranslateY(Scalar translateY) {
    getSkValue().setTranslateY(translateY);
}

Scalar Matrix::getScaleX() const {
    return getSkValue().getScaleX();
}

void Matrix::setScaleX(Scalar scaleX) {
    getSkValue().setScaleX(scaleX);
}

Scalar Matrix::getScaleY() const {
    return getSkValue().getScaleY();
}

void Matrix::setScaleY(Scalar scaleY) {
    getSkValue().setScaleY(scaleY);
}

Scalar Matrix::getSkewX() const {
    return getSkValue().getSkewX();
}

void Matrix::setSkewX(Scalar skewX) {
    getSkValue().setSkewX(skewX);
}

Scalar Matrix::getSkewY() const {
    return getSkValue().getSkewY();
}

void Matrix::setSkewY(Scalar skewY) {
    getSkValue().setSkewY(skewY);
}

void Matrix::postRotate(Scalar radians, Scalar px, Scalar py) {
    postRotateDegrees(SkRadiansToDegrees(radians), px, py);
}

void Matrix::postRotateDegrees(Scalar degrees, Scalar px, Scalar py) {
    getSkValue().postRotate(degrees, px, py);
}

void Matrix::preConcat(const Matrix& other) {
    getSkValue().preConcat(other.getSkValue());
}

void Matrix::postConcat(const Matrix& other) {
    getSkValue().postConcat(other.getSkValue());
}

void Matrix::preScale(Scalar sx, Scalar sy) {
    getSkValue().preScale(sx, sy);
}

void Matrix::postScale(Scalar sx, Scalar sy) {
    getSkValue().postScale(sx, sy);
}

Rect Matrix::mapRect(const Rect& rect) const {
    return fromSkValue<Rect>(getSkValue().mapRect(rect.getSkValue()));
}

bool Matrix::isIdentity() const {
    return getSkValue().isIdentity();
}

bool Matrix::isIdentityOrTranslate() const {
    return getSkValue().isTranslate();
}

Matrix Matrix::makeScaleTranslate(Scalar sx, Scalar sy, Scalar tx, Scalar ty) {
    Matrix matrix;
    matrix.getSkValue().setScaleTranslate(sx, sy, tx, ty);
    return matrix;
}

Matrix Matrix::makeScale(Scalar sx, Scalar sy) {
    Matrix matrix;
    matrix.getSkValue().setScale(sx, sy);
    return matrix;
}

bool Matrix::operator==(const Matrix& other) const {
    return getSkValue() == other.getSkValue();
}

bool Matrix::operator!=(const Matrix& other) const {
    return getSkValue() != other.getSkValue();
}

bool Matrix::toAffine(Scalar* affineValues) const {
    const auto& mat = getSkValue();

    return mat.asAffine(reinterpret_cast<SkScalar*>(affineValues));
}

void Matrix::getAll(Scalar* values) const {
    getSkValue().get9(reinterpret_cast<SkScalar*>(values));
}

std::string Matrix::toString() const {
    const auto& skMatrix = getSkValue();
    return fmt::format("[{}, {}, {}, {}, {}, {}, {}, {}, {}]",
                       skMatrix.get(0),
                       skMatrix.get(1),
                       skMatrix.get(2),
                       skMatrix.get(3),
                       skMatrix.get(4),
                       skMatrix.get(5),
                       skMatrix.get(6),
                       skMatrix.get(7),
                       skMatrix.get(8));
}

std::ostream& operator<<(std::ostream& os, const Matrix& matrix) {
    return os << matrix.toString();
}

} // namespace snap::drawing
