//
//  Compositor.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/1/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"

#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace Valdi {
class ILogger;
}

namespace snap::drawing {

class CompositorPlaneList;

/**
 The Compositor finds how many surfaces need to be used in order to draw a DisplayList.
 If the DisplayList has 1 or more external surfaces, it will split up the DisplayList
 into surfaces such that the external surfaces can be drawn at the right location and opacity
 by an external system. Regular surfaces are then laid out above or below the external
 surfaces depending on what they should drawn. The Compositor tries to limit the number
 of regular surfaces to use as much as possible. It does this by calculating the intersections
 of the draw commands to see if they intersects with external surfaces.
 */
class Compositor {
public:
    explicit Compositor(Valdi::ILogger& logger);

    Ref<DisplayList> performComposition(DisplayList& sourceDisplayList, CompositorPlaneList& planeList);

private:
    [[maybe_unused]] Valdi::ILogger& _logger;
};

} // namespace snap::drawing
