#include "snap_drawing/cpp/Layers/Mask/ScrollLayerFadingEdgesMaskLayer.hpp"
#include "snap_drawing/cpp/Drawing/Mask/CompositeMask.hpp"

namespace snap::drawing {

ScrollLayerFadingEdgesMaskLayer::ScrollLayerFadingEdgesMaskLayer() = default;
ScrollLayerFadingEdgesMaskLayer::~ScrollLayerFadingEdgesMaskLayer() = default;

bool ScrollLayerFadingEdgesMaskLayer::configure(bool isHorizontal,
                                                const Rect& scrollRect,
                                                Scalar leadingEdgeLength,
                                                Scalar trailingEdgeLength) {
    auto boundsChanged = _scrollRect != scrollRect;
    if (!boundsChanged && _isHorizontal == isHorizontal && _leadingEdgeLength == leadingEdgeLength &&
        _trailingEdgeLength == trailingEdgeLength) {
        return false;
    }
    _isHorizontal = isHorizontal;
    _scrollRect = scrollRect;
    LinearGradientOrientation orientation =
        isHorizontal ? LinearGradientOrientationLeftRight : LinearGradientOrientationTopBottom;
    if (leadingEdgeLength != _leadingEdgeLength || boundsChanged) {
        _leadingEdgeLength = leadingEdgeLength;
        if (leadingEdgeLength > 0 && _leadingEdgeMaskLayer == nullptr) {
            _leadingEdgeMaskLayer = Valdi::makeShared<GradientMaskLayer>();
            _leadingEdgeMaskLayer->setPositioning(getPositioning());
            std::vector<Color> colors{Color::black(), Color::transparent()};
            _leadingEdgeMaskLayer->getGradient().setColors(std::move(colors));
        }
        if (_leadingEdgeMaskLayer.get()) {
            _leadingEdgeMaskLayer->getGradient().setOrientation(orientation);
            auto rect = Rect::makeLTRB(0,
                                       0,
                                       isHorizontal ? leadingEdgeLength : scrollRect.right,
                                       isHorizontal ? scrollRect.bottom : leadingEdgeLength);
            _leadingEdgeMaskLayer->setRect(rect);
        }
    }
    if (trailingEdgeLength != _trailingEdgeLength || boundsChanged) {
        _trailingEdgeLength = trailingEdgeLength;
        if (trailingEdgeLength > 0 && _trailingEdgeMaskLayer == nullptr) {
            _trailingEdgeMaskLayer = Valdi::makeShared<GradientMaskLayer>();
            _trailingEdgeMaskLayer->setPositioning(getPositioning());
            std::vector<Color> colors{Color::transparent(), Color::black()};
            _trailingEdgeMaskLayer->getGradient().setColors(std::move(colors));
        }
        if (_trailingEdgeMaskLayer.get()) {
            _trailingEdgeMaskLayer->getGradient().setOrientation(orientation);
            auto rect = Rect::makeLTRB(isHorizontal ? scrollRect.right - trailingEdgeLength : 0,
                                       isHorizontal ? 0 : scrollRect.bottom - trailingEdgeLength,
                                       scrollRect.right,
                                       scrollRect.bottom);
            _trailingEdgeMaskLayer->setRect(rect);
        }
    }
    return true;
}

MaskLayerPositioning ScrollLayerFadingEdgesMaskLayer::getPositioning() {
    return MaskLayerPositioning::AboveBackground;
}

Ref<IMask> ScrollLayerFadingEdgesMaskLayer::createMask(const Rect& bounds) {
    std::vector<Ref<IMask>> masks;
    if (_leadingEdgeLength > 0) {
        auto mask = _leadingEdgeMaskLayer->createMask(bounds);
        if (mask != nullptr) {
            masks.emplace_back(mask);
        }
    }
    if (_trailingEdgeLength > 0) {
        auto mask = _trailingEdgeMaskLayer->createMask(bounds);
        if (mask != nullptr) {
            masks.emplace_back(mask);
        }
    }
    if (masks.empty()) {
        return nullptr;
    }
    auto composite = Valdi::makeShared<CompositeMask>(std::move(masks));
    return composite;
}

} // namespace snap::drawing
