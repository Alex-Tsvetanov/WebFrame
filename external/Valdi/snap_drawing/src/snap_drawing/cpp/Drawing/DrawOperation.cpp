//
//  DrawOperation.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 2/16/22.
//

#include "snap_drawing/cpp/Drawing/DrawOperation.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterManager.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace snap::drawing {

DrawOperation::DrawOperation(const Ref<DisplayList>& displayList,
                             const Ref<SurfacePresenterManager>& surfacePresenterManager,
                             SurfacePresenterList&& surfacePresenters)
    : _displayList(displayList),
      _surfacePresenterManager(surfacePresenterManager),
      _surfacePresenters(std::move(surfacePresenters)),
      _current(_surfacePresenters.begin()) {
    advance();
}

DrawOperation::~DrawOperation() = default;

bool DrawOperation::drawForPresenterId(SurfacePresenterId presenterId, DrawableSurfaceCanvas& canvas) {
    auto* presenter = _surfacePresenters.getForId(presenterId);
    if (_displayList == nullptr || presenter == nullptr) {
        return false;
    }

    _displayList->draw(canvas, presenter->getDisplayListPlaneIndex(), /* shouldClearCanvas */ true);

    return true;
}

Valdi::Result<snap::drawing::GraphicsContext*> DrawOperation::drawNext() {
    const auto& surfacePresenter = *_current;
    _current++;

    advance();

    auto drawableSurface = Ref<DrawableSurface>(surfacePresenter.getDrawableSurface());

    if (drawableSurface == nullptr) {
        return nullptr;
    }

    VALDI_TRACE("SnapDrawing.drawSurface");

    auto canvasResult = drawableSurface->prepareCanvas();

    if (!canvasResult) {
        return canvasResult.error().rethrow("Failed to prepare canvas on GraphicsContext");
    }

    auto displayListSize = _displayList->getSize();
    auto scaleX = static_cast<Scalar>(canvasResult.value().getWidth()) / displayListSize.width;
    auto scaleY = static_cast<Scalar>(canvasResult.value().getHeight()) / displayListSize.height;

    _displayList->draw(canvasResult.value(),
                       surfacePresenter.getDisplayListPlaneIndex(),
                       scaleX,
                       scaleY,
                       /* shouldClearCanvas */ true);

    drawableSurface->flush();

    _surfacePresenterManager->onDrawableSurfacePresenterUpdated(surfacePresenter.getId());

    return drawableSurface->getGraphicsContext();
}

bool DrawOperation::hasNext() {
    return _current != _surfacePresenters.end();
}

void DrawOperation::advance() {
    while (_current != _surfacePresenters.end() && !_current->isDrawable()) {
        _current++;
    }
}

} // namespace snap::drawing
