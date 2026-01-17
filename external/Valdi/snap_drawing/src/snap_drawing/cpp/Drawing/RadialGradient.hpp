//
//  RadialGradient.hpp
//  snap_drawing
//
//  Created by Brandon Francis on 3/7/23.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/LazyPath.hpp"

#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"

#include <vector>

namespace snap::drawing {

class BorderRadius;

class RadialGradient : public Valdi::SimpleRefCountable {
public:
    RadialGradient();
    ~RadialGradient() override;

    bool isDirty() const;
    bool isEmpty() const;

    void setLocations(std::vector<Scalar>&& locations);
    void setColors(std::vector<Color>&& colors);

    void update(const Rect& bounds);

    void applyToPaint(Paint& paint) const;

    void draw(DrawingContext& drawingContext, const BorderRadius& borderRadius);

private:
    sk_sp<SkShader> _shader;
    std::vector<Scalar> _locations;
    std::vector<Color> _colors;
    Rect _lastDrawBounds = Rect::makeEmpty();
    LazyPath _lazyPath;
    bool _dirty = true;

    void updateShader(const Rect& bounds);
};

} // namespace snap::drawing
