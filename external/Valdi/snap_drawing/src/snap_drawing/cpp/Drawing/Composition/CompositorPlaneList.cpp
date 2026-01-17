//
//  CompositorPlaneList.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/Composition/CompositorPlaneList.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"

namespace snap::drawing {

CompositorPlaneList::CompositorPlaneList() = default;

CompositorPlaneList::~CompositorPlaneList() = default;

size_t CompositorPlaneList::getPlanesCount() const {
    return _planes.size();
}

const CompositorPlane& CompositorPlaneList::getPlaneAtIndex(size_t index) const {
    return _planes[index];
}

CompositorPlane& CompositorPlaneList::getPlaneAtIndex(size_t index) {
    return _planes[index];
}

void CompositorPlaneList::appendPlane(CompositorPlane&& plane) {
    _planes.emplace_back(plane);
}

void CompositorPlaneList::appendDrawableSurface() {
    appendPlane(CompositorPlane(nullptr));
}

void CompositorPlaneList::insertPlane(CompositorPlane&& plane, size_t index) {
    _planes.insert(_planes.begin() + index, std::move(plane));
}

void CompositorPlaneList::removePlaneAtIndex(size_t index) {
    _planes.erase(_planes.begin() + index);
}

void CompositorPlaneList::clear() {
    _planes.clear();
}

const CompositorPlane* CompositorPlaneList::begin() const {
    return _planes.data();
}

const CompositorPlane* CompositorPlaneList::end() const {
    return _planes.data() + _planes.size();
}

CompositorPlane* CompositorPlaneList::begin() {
    return _planes.data();
}

CompositorPlane* CompositorPlaneList::end() {
    return _planes.data() + _planes.size();
}

} // namespace snap::drawing
