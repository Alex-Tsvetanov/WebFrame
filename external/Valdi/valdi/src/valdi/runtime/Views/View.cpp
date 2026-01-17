//
//  View.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 6/26/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/Views/View.hpp"

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

namespace Valdi {

ViewTrace::ViewTrace(StringBox bundleName, StringBox documentPath, StringBox nodeId)
    : bundleName(std::move(bundleName)), documentPath(std::move(documentPath)), nodeId(std::move(nodeId)) {}

View::View() = default;

View::~View() = default;

Result<Void> View::rasterInto(
    const Ref<IBitmap>& bitmap, const Frame& frame, const Matrix& transform, float rasterScaleX, float rasterScaleY) {
    return Error("rasterInto() not implemented");
}

bool View::onTouchEvent(long offsetSinceSourceMs,
                        int touchEventType,
                        float absoluteX,
                        float absoluteY,
                        float x,
                        float y,
                        float dx,
                        float dy,
                        bool isTouchOwner,
                        const Ref<RefCountable>& source) {
    return false;
}

ViewHitTestResult View::hitTest(float x, float y) {
    return ViewHitTestResultUseDefault;
}

bool View::canBeReused() const {
    return _canBeReused;
}

void View::setCanBeReused(bool viewCanBeReused) {
    _canBeReused = viewCanBeReused;
}

} // namespace Valdi
