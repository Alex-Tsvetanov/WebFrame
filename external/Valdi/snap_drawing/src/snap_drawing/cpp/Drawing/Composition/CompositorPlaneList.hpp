//
//  CompositorPlaneList.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Composition/CompositorPlane.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace snap::drawing {

class CompositorPlaneList {
public:
    CompositorPlaneList();
    ~CompositorPlaneList();

    size_t getPlanesCount() const;
    const CompositorPlane& getPlaneAtIndex(size_t index) const;
    CompositorPlane& getPlaneAtIndex(size_t index);

    void appendPlane(CompositorPlane&& plane);
    void appendDrawableSurface();
    void insertPlane(CompositorPlane&& plane, size_t index);
    void removePlaneAtIndex(size_t index);

    void clear();

    const CompositorPlane* begin() const;
    const CompositorPlane* end() const;
    CompositorPlane* begin();
    CompositorPlane* end();

private:
    Valdi::SmallVector<CompositorPlane, 2> _planes;
};

} // namespace snap::drawing
