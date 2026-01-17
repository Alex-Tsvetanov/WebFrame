#include "snap_drawing/cpp/Drawing/Raster/RasterDamageResolver.hpp"
#include "snap_drawing/cpp/Drawing/Composition/CompositionState.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace snap::drawing {

struct ComputeDamageVisitor {
    explicit ComputeDamageVisitor(RasterDamageResolver& rasterDamageResolver, Scalar scaleX, Scalar scaleY)
        : _rasterDamageResolver(rasterDamageResolver) {
        Matrix baseMatrix;
        baseMatrix.setScaleX(scaleX);
        baseMatrix.setScaleY(scaleY);
        _contextStack.emplace_back(CompositionState(Path(), baseMatrix, 1.0), 0, false);
    }

    void visit(const Operations::PushContext& pushContext) {
        auto& topContext = getCurrentContext();

        _contextStack.emplace_back(topContext.compositionState.pushContext(pushContext.opacity, pushContext.matrix),
                                   pushContext.layerId,
                                   pushContext.hasUpdates);
    }

    void visit(const Operations::PopContext& /*popContext*/) {
        _contextStack.pop_back();
    }

    void visit(const Operations::ClipRect& clipRect) {
        auto& currentContext = getCurrentContext();

        currentContext.compositionState.clipRect(clipRect.width, clipRect.height);
    }

    void visit(const Operations::ClipRound& clipRound) {
        auto& currentContext = getCurrentContext();

        currentContext.compositionState.clipRound(clipRound.borderRadius, clipRound.width, clipRound.height);
    }

    void visit(const Operations::DrawPicture& drawPicture) {
        addDamageIfNeeded(fromSkValue<Rect>(drawPicture.picture->cullRect()));
    }

    void visit(const Operations::DrawExternalSurface& drawExternalSurface) {
        auto size = drawExternalSurface.externalSurfaceSnapshot->getExternalSurface()->getRelativeSize();

        addDamageIfNeeded(Rect::makeXYWH(0, 0, size.width, size.height));
    }

    void visit(const Operations::PrepareMask& prepareMask) {
        addDamageIfNeeded(prepareMask.mask->getBounds());
    }

    void visit(const Operations::ApplyMask& applyMask) {}

private:
    struct Context {
        CompositionState compositionState;
        uint64_t layerId = 0;
        bool hasUpdates = false;

        Context(CompositionState&& compositionState, uint64_t layerId, bool hasUpdates)
            : compositionState(std::move(compositionState)), layerId(layerId), hasUpdates(hasUpdates) {}
    };

    RasterDamageResolver& _rasterDamageResolver;
    Valdi::SmallVector<Context, 8> _contextStack;

    Context& getCurrentContext() {
        return _contextStack[_contextStack.size() - 1];
    }

    void addDamageIfNeeded(const Rect& bounds) {
        const auto& context = getCurrentContext();
        auto absoluteRect = context.compositionState.getAbsoluteClippedRect(bounds);
        _rasterDamageResolver.addNonTransparentLayerInRect(context.layerId,
                                                           absoluteRect,
                                                           context.compositionState.getAbsoluteMatrix(),
                                                           context.compositionState.getAbsoluteClipPath(),
                                                           context.compositionState.getAbsoluteOpacity(),
                                                           context.hasUpdates);
    }
};

RasterDamageResolver::RasterDamageResolver() = default;
RasterDamageResolver::~RasterDamageResolver() = default;

void RasterDamageResolver::beginUpdates(Scalar surfaceWidth, Scalar surfaceHeight) {
    auto changed = _width != surfaceWidth || _height != surfaceHeight;
    _width = surfaceWidth;
    _height = surfaceHeight;

    if (changed) {
        addDamageInRect(Rect::makeXYWH(0, 0, surfaceWidth, surfaceHeight));
    }
}

std::vector<Rect> RasterDamageResolver::endUpdates() {
    resolveDamage();

    std::swap(_previousLayerContents, _layerContents);
    _layerContents.clear();
    auto damageRects = std::move(_damageRects);
    _damageRects = std::vector<Rect>();

    return damageRects;
}

void RasterDamageResolver::resolveDamage() {
    // Iterate over all previous layer contents, and add damage if the layer was removed, or has updates of any kind.
    for (const auto& [layerId, layerContent] : _previousLayerContents) {
        const auto& it = _layerContents.find(layerId);
        if (it == _layerContents.end()) {
            // Layer no longer exists, add damage for the entire previous rect
            addDamageInRect(layerContent.absoluteRect);
            continue;
        }

        auto& newLayerContent = it->second;

        if (newLayerContent.hasUpdates || newLayerContent.absoluteMatrix != layerContent.absoluteMatrix ||
            newLayerContent.clipPath != layerContent.clipPath ||
            newLayerContent.absoluteRect != layerContent.absoluteRect ||
            newLayerContent.absoluteOpacity != layerContent.absoluteOpacity) {
            newLayerContent.hasUpdates = false;

            addDamageInRect(layerContent.absoluteRect);
            addDamageInRect(newLayerContent.absoluteRect);
        }
    }

    // Iterate over new layer contents, add damage for the new layer that were not processed in the previous loop.
    for (auto& [layerId, layerContent] : _layerContents) {
        if (layerContent.hasUpdates) {
            layerContent.hasUpdates = false;
            addDamageInRect(layerContent.absoluteRect);
        }
    }
}

void RasterDamageResolver::addDamageFromDisplayListUpdates(const DisplayList& displayList) {
    auto scaleX = _width / displayList.getSize().width;
    auto scaleY = _height / displayList.getSize().height;
    ComputeDamageVisitor computeDamageVisitor(*this, scaleX, scaleY);
    for (size_t i = 0; i < displayList.getPlanesCount(); i++) {
        displayList.visitOperations(i, computeDamageVisitor);
    }
}

void RasterDamageResolver::addDamageInRect(const Rect& rect) {
    auto damageToAdd = rect;

    /**
    TODO(simon): This is a simple and inefficient implementation which removes the
    intersected damage rects and adds a new damage rect that is the union of all
    the intersected damage rects.
     */
    auto it = _damageRects.begin();
    while (it != _damageRects.end()) {
        if (it->intersects(damageToAdd)) {
            damageToAdd.join(*it);
            _damageRects.erase(it);
            it = _damageRects.begin();
            break;
        } else {
            it++;
        }
    }

    _damageRects.emplace_back(damageToAdd);
}

void RasterDamageResolver::addNonTransparentLayerInRect(uint64_t layerId,
                                                        const Rect& rect,
                                                        const Matrix& absoluteMatrix,
                                                        const Path& clipPath,
                                                        Scalar absoluteOpacity,
                                                        bool hasUpdates) {
    auto& layerContent = _layerContents[layerId];
    layerContent.absoluteRect = rect;
    layerContent.absoluteMatrix = absoluteMatrix;
    layerContent.clipPath = clipPath;
    layerContent.absoluteOpacity = absoluteOpacity;
    layerContent.hasUpdates = hasUpdates;
}

} // namespace snap::drawing