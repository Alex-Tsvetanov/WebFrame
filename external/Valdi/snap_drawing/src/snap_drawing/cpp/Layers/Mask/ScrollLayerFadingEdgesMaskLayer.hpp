#include "snap_drawing/cpp/Layers/Mask/GradientMaskLayer.hpp"

namespace snap::drawing {

class Paint;

class ScrollLayerFadingEdgesMaskLayer : public IMaskLayer {
public:
    ScrollLayerFadingEdgesMaskLayer();
    ~ScrollLayerFadingEdgesMaskLayer() override;

    bool configure(bool isHorizontal, const Rect& scrollRect, Scalar leadingEdgeLength, Scalar trailingEdgeLength);

    MaskLayerPositioning getPositioning() override;
    Ref<IMask> createMask(const Rect& bounds) override;

protected:
private:
    bool _isHorizontal = false;
    Rect _scrollRect;
    Scalar _leadingEdgeLength = 0;
    Scalar _trailingEdgeLength = 0;

    Ref<GradientMaskLayer> _leadingEdgeMaskLayer;
    Ref<GradientMaskLayer> _trailingEdgeMaskLayer;
};

} // namespace snap::drawing
