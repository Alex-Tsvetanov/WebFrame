//
//  BoxShadow.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/8/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/LazyPath.hpp"

#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"

namespace snap::drawing {

class BorderRadius;

class BoxShadow : public Valdi::SimpleRefCountable {
public:
    BoxShadow();
    ~BoxShadow() override;

    void setOffset(Size offset);
    void setColor(Color color);
    void setBlurAmount(Scalar blurAmount);

    void draw(DrawingContext& drawingContext, const BorderRadius& borderRadius);

private:
    Size _offset = Size::makeEmpty();
    Color _color = Color::transparent();
    Scalar _blurAmount = 0;
    Paint _paint;
    LazyPath _lazyPath;
};

} // namespace snap::drawing
