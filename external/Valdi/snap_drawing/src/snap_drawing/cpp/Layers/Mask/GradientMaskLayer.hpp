//
//  GradientMaskLayer.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#include "snap_drawing/cpp/Drawing/LinearGradient.hpp"
#include "snap_drawing/cpp/Layers/Mask/PaintMaskLayer.hpp"

namespace snap::drawing {

class Paint;

/**
 A GradientMaskLayer is a specialization of PaintMaskLayer which
 draws the region using a given gradient.
 */
class GradientMaskLayer : public PaintMaskLayer {
public:
    GradientMaskLayer();
    ~GradientMaskLayer() override;

    const LinearGradient& getGradient() const;
    LinearGradient& getGradient();

protected:
    void onConfigurePaint(Paint& paint, const Rect& bounds) override;

private:
    LinearGradient _gradient;
};

} // namespace snap::drawing
