#include "CompositionState.hpp"

namespace snap::drawing {

CompositionState::CompositionState() = default;

CompositionState::CompositionState(const Path& clipPath, const Matrix& resolvedMatrix, Scalar resolvedOpacity)
    : _absoluteClipPath(clipPath), _absoluteMatrix(resolvedMatrix), _absoluteOpacity(resolvedOpacity) {}

CompositionState::~CompositionState() = default;

CompositionState CompositionState::pushContext(Scalar opacity, const Matrix& matrix) {
    auto newOpacity = _absoluteOpacity * opacity;
    auto newMatrix = _absoluteMatrix;
    newMatrix.preConcat(matrix);

    return CompositionState(_absoluteClipPath, newMatrix, newOpacity);
}

void CompositionState::clipRect(Scalar width, Scalar height) {
    Path clipPath;
    clipPath.addRect(Rect::makeLTRB(0, 0, width, height), true);
    updateClip(clipPath);
}

void CompositionState::clipRound(const BorderRadius& borderRadius, Scalar width, Scalar height) {
    auto clipPath = borderRadius.getPath(Rect::makeLTRB(0, 0, width, height));
    updateClip(clipPath);
}

void CompositionState::updateClip(Path& path) {
    path.transform(_absoluteMatrix);

    if (_absoluteClipPath.isEmpty()) {
        _absoluteClipPath = std::move(path);
    } else {
        _absoluteClipPath = _absoluteClipPath.intersection(path);
    }
}

SkScalar CompositionState::getAbsoluteOpacity() const {
    return _absoluteOpacity;
}

const Matrix& CompositionState::getAbsoluteMatrix() const {
    return _absoluteMatrix;
}

const Path& CompositionState::getAbsoluteClipPath() const {
    return _absoluteClipPath;
}

Rect CompositionState::getAbsoluteRect(const Rect& localRect) const {
    return _absoluteMatrix.mapRect(localRect);
}

Rect CompositionState::getAbsoluteClippedRect(const Rect& localRect) const {
    auto absoluteRect = getAbsoluteRect(localRect);
    auto clipBounds = _absoluteClipPath.getBounds();

    if (clipBounds) {
        return clipBounds.value().intersection(absoluteRect);
    } else {
        return absoluteRect;
    }
}

} // namespace snap::drawing