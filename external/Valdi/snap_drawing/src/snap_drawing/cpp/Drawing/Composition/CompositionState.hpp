#pragma once

#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"

namespace snap::drawing {

/**
CompositionState helps with resolving absolute opacity, transform matrix and clip path inside a DisplayList.
Since each pushContext operation within the DisplayList is relative to the parent context, CompositionState helps
with resolving the absolute values for properties that are relevant for composition.
 */
class CompositionState {
public:
    CompositionState();
    CompositionState(const Path& clipPath, const Matrix& resolvedMatrix, Scalar resolvedOpacity);
    ~CompositionState();

    CompositionState pushContext(Scalar opacity, const Matrix& matrix);

    void clipRect(Scalar width, Scalar height);
    void clipRound(const BorderRadius& borderRadius, Scalar width, Scalar height);

    SkScalar getAbsoluteOpacity() const;
    const Matrix& getAbsoluteMatrix() const;
    const Path& getAbsoluteClipPath() const;

    Rect getAbsoluteRect(const Rect& localRect) const;
    Rect getAbsoluteClippedRect(const Rect& localRect) const;

private:
    // The absolute clip path in this context
    Path _absoluteClipPath;
    // The absolute matrix which can be used to transform from relative coordinates
    // from within this context into absolute.
    Matrix _absoluteMatrix;
    // The absolute opacity, taking in account the opacity of this context and
    // all contexts above it.
    Scalar _absoluteOpacity = 1.0;

    void updateClip(Path& path);
};

} // namespace snap::drawing