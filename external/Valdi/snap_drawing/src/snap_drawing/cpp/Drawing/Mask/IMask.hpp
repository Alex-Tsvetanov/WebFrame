//
//  IMask.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

class SkCanvas;

namespace snap::drawing {

/**
 An IMask represents a single masking operation emitted at draw time.
 It can be created from an IMaskLayer instance.
 */
class IMask : public Valdi::SimpleRefCountable {
public:
    /**
     Returns the bounds covering the entire range of the mask.
     The bounds is used to know what area might be altered by the mask.
     */
    virtual Rect getBounds() const = 0;

    /**
     Prepare the canvas for applying the mask.
     */
    virtual void prepare(SkCanvas* canvas) = 0;

    /**
     Apply the mask, which should have been previously prepared through the prepare() call.
     The content drawn between the prepare and apply call should be masked after
     the apply call is made.
     */
    virtual void apply(SkCanvas* canvas) = 0;

    /**
     Return a description best matching the mask and its configured values.
     */
    virtual String getDescription() = 0;
};

} // namespace snap::drawing
