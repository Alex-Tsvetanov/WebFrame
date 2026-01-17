//
//  LinearGradient.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/8/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/LazyPath.hpp"

#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"

#include <vector>

namespace snap::drawing {

class BorderRadius;

enum LinearGradientOrientation {
    /** draw the gradient from the top to the bottom */
    LinearGradientOrientationTopBottom,
    /** draw the gradient from the top-right to the bottom-left */
    LinearGradientOrientationTopRightBottomLeft,
    /** draw the gradient from the right to the left */
    LinearGradientOrientationRightLeft,
    /** draw the gradient from the bottom-right to the top-left */
    LinearGradientOrientationBottomRightTopLeft,
    /** draw the gradient from the bottom to the top */
    LinearGradientOrientationBottomTop,
    /** draw the gradient from the bottom-left to the top-right */
    LinearGradientOrientationBottomLeftTopRight,
    /** draw the gradient from the left to the right */
    LinearGradientOrientationLeftRight,
    /** draw the gradient from the top-left to the bottom-right */
    LinearGradientOrientationTopLeftBottomRight,
};

class LinearGradient : public Valdi::SimpleRefCountable {
public:
    LinearGradient();
    ~LinearGradient() override;

    bool isDirty() const;
    bool isEmpty() const;

    void setLocations(std::vector<Scalar>&& locations);
    void setColors(std::vector<Color>&& colors);
    void setOrientation(LinearGradientOrientation orientation);

    void update(const Rect& bounds);

    void applyToPaint(Paint& paint) const;

    void draw(DrawingContext& drawingContext, const BorderRadius& borderRadius);

private:
    sk_sp<SkShader> _shader;
    std::vector<Scalar> _locations;
    std::vector<Color> _colors;
    LinearGradientOrientation _orientation = LinearGradientOrientationTopBottom;
    Rect _lastDrawBounds = Rect::makeEmpty();
    LazyPath _lazyPath;
    bool _dirty = true;

    void updateShader(const Rect& bounds);
};

} // namespace snap::drawing
