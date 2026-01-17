//
//  PaintMaskLayer.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Paint.hpp"
#include "snap_drawing/cpp/Layers/Mask/IMaskLayer.hpp"
#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

#include "include/core/SkBlendMode.h"

namespace snap::drawing {

class Paint;

/**
 A PaintMaskLayer is an IMaskLayer implementation that emits PaintMask.
 When using the default DstOut blendMode, this will cause a given region
 provided through a rect or path to be drawn with a different opacity.
 */
class PaintMaskLayer : public IMaskLayer {
public:
    PaintMaskLayer();
    ~PaintMaskLayer() override;

    void setRect(const Rect& rect);
    void setPath(const Path& path);

    /**
     Set the color that should be used to draw the region.
     */
    void setColor(Color color);

    /**
     Get the color that should be used to draw the region
     */
    Color getColor() const;

    void setPositioning(MaskLayerPositioning positioning);

    /**
     Set the blend mode that should be used to draw the region.
     */
    void setBlendMode(SkBlendMode blendMode);

    /**
     Returns the bounds that represents the entire region configured
     through either a rect or path.
     */
    Rect getBounds() const;

    MaskLayerPositioning getPositioning() override;
    Ref<IMask> createMask(const Rect& bounds) override;

protected:
    virtual void onConfigurePaint(Paint& paint, const Rect& bounds);

private:
    Rect _rect;
    Path _path;
    Color _color = Color::black();
    SkBlendMode _blendMode = SkBlendMode::kDstOut;
    MaskLayerPositioning _positioning = MaskLayerPositioning::BelowBackground;
};

} // namespace snap::drawing
